
Virtual 9-Track Tape Drive (v9ttd) lets you decode digital images of PE/NRZI
9-track tapes. The idea behind the tool is that you can read tapes which are
otherwise unreadable by the drive you have access to due to tape damage
or format incompatibility.

Another reason for using it is that in case of old precious tapes you may want to
minimize the tape wear by making a single "raw" readout and then doing multiple offline
software analysis passes until the data is successfully read.

# Getting the input data

To get the input data you need a tape drive and a 16-bit logic analyzer that allows dumping the data
stream in real time to a file.

First you need to locate earliest point in your drive's signal chain where head signals are available
in digital form. For example, in Qualstar 1052 drive that would be the 9-pin connector named ZCD.
Notice, that there
may also be another digital output with clock or "data ready" signals - this is not what you want.

Following ASCII drawing shows a general idea of where you need to tap in:

            analog          analog   .----------.    .--- LOGIC ANALYZER
            signals         signals  |  FLUX    |    |
                                 .-->| CHANGES  |----+-----> ...
    .------.     .------------.  |   | DETECTOR |
    |      |     |            |  |   `----------'  digital
    | HEAD |---->| AMPLIFIERS |--+   .----------.  signals
    |      |     |            |  |   |  CLOCK   |
    `------'     `------------'  `-->| FORMING  |----------> ...
                                     `----------'

Connect the logic analyzer to all 9 signal pins - the order doesn't matter, tracks can be later reordered with
tool's '-t' option. Now you need to put the drive in a "free-running" mode, which lets the tape
to move over the heads with a constant speed and start recording logic analyzer output.

Note that this is not the only way of doing it. You may as well sample the analog signals,
digitize it on your own and prepare an image file readable by v9ttd.

## Sampling frequency

Quality of the input data plays a great role in how successful the readout is.
In general, the higher sampling frequency the beter, but you need to consider final image size too.

For example:

1600bpi PE tape has a maximum od 3200 flux changes per inch.
This translates to max 160kHz signal for a tape running at 50in/s.
1MS/s would be the lowest "safe" practical sampling frequency, at which resulting dump file
is 1GiB in size.

# Building v9ttd

To build v9ttd from sources you'll need:

* cmake
* GNU make

```
mkdir build
cd build
cmake ..
make
make install
```

# Analyzing tape images

v9ttd input is a 16-bit LE binary file, where each word contains all 9 bits
of track signal in a given point in time. In other words: each 16-bit value is a
tape row sample. This is a format used by Saleae software when exporting data to a binary file:

http://support.saleae.com/hc/en-us/articles/208667306-Binary-data-export-format-description

## Program parameters summary

* **-h** - Print help
* **-i input** - Input tape image file name
* **-p len** - Base pulse length (>1)
* **-c chlist** - Input channel list specified as: p,7,6,5,4,3,2,1,0 (p=parity track, 0=LSB track). Default is: 8,7,6,5,4,3,2,1,0
* **-d ratio** - Downsample input data by n>1
* **-m margin** - Max allowed base pulse margin (0.0 - 0.5)
* **-s skew** - Max allowed inter-track skew (0.0 - 0.5)
* **-S** - Calculate and print pulse statistics

## Running v9ttd

v9ttd can run in one of two modes:

* pulse length statistical analysis (selected by specifying **-S** switch at the commandline)
* tape contents analysis (used otherwise)

First mode can help with finding out parameters for running v9ttd in the second mode.
It does not analyze the signal for data, it just lets you see what are the pulse lenghts
in your sampled signal (for both positive and negative pulses, polarity doesn't matter).

Depending on the mode, different options should/can be used:

* pulse analysis: `v9ttd -i input -S [-c chlist] [-d downsample]`
* content analysis: `v9ttd -i input -p pulse_len [-c chlist] [-m pulse_margin] [-d downsample] [-s skew]`

### Input file name (-i)

This parameter is required for both modes of operation. It specifies the input file that v9ttd works on.

### Channel to track mapping (-c)

This parameter is optional for both modes of operation.

By default v9ttd assumes that logic analyzer probes were connected to the drive so that
logic channels 0-7 carry signals for data bits 2^0...2^7, and channel 8 carry the parity track:

* channel 0 -> bit 2^0
* channel 1 -> bit 2^1
* channel 2 -> bit 2^2
* channel 3 -> bit 2^3
* channel 4 -> bit 2^4
* channel 5 -> bit 2^5
* channel 6 -> bit 2^6
* channel 7 -> bit 2^7
* channel 8 -> parity

I you follow this setup, there is no need to use **-c** option,
otherwise you need to provide proper channel to track mapping.

Numbers in the **-c** map list are logic analyzer channel numbers.
Position in the list denotes the bit significance as follows:

    parity, 2^7, 2^6, 2^5, 2^4, 2^3, 2^2, 2^1, 2^0

For example, if logic analyzer channels are connected the following way:

* channel 0 -> parity
* channel 1 -> bit 2^0
* channel 2 -> bit 2^1
* channel 3 -> bit 2^2
* channel 4 -> bit 2^3
* channel 5 -> bit 2^4
* channel 6 -> bit 2^5
* channel 7 -> bit 2^6
* channel 8 -> bit 2^7

you would need to remap them with:

    v9ttd -i input.bin -c 0,8,7,6,5,4,3,2,1

### Downsampling (-d)

This parameter is optional for both modes of operation.

If your input is sampled with high frequency, for example 10MS/s, you may want to downsample it
before analysis to lower the memory usage and speed up the proces. To downsample a signal
by 5 (to 2MS/s), use:

    v9ttd -i input.bin -d 5

### Signal statistics (-S)

This switch selects the pulse analysis mode, which lets you see a histogram of all pulses found
on the tape image. For example:


```
Pulse length histogram:
   1 : ####
   2 : #################################################
   3 : ##############################################
   4 : #############################
   5 : #######################################################
   6 : ##################################################################################
   7 : ##################################################################
   8 : ###############
   9 : ####
  10 : ##
  11 : ###############
  12 : ###################
  13 : #######
  14 : ##
```

From the above you can guess that the base pulse length (or the BPL, or the minimum flux transition distance) is 6 samples -
this is the most frequent pulse length. There is also another, last peek at 12 samples (double the BPL),
which indicates, that this tape is PE-encoded.
Pulses of length around 1-4 seem to be a digital noise "created" between blocks by tape drive's digital stage drivers.

### Base pulse length and margin (-p and -m)

Now that you know what the BPL is, you can set parameters for the tape content analysis. BPL is set with **-p**
option, amd **-m** allows you to set the +- margin (as a ratio of BPL). For example, the following command:

    v9ttd -i input.bin -b 62 -m 0.3

sets the base pulse length to 62 with margin of 0.3 BPL, allowing it to be anywhere between 38 and 86 (inclusive).

Following table shows base pulse length values for various tape formats and speeds sampled at 10MS/s:

| Tape    |  800 BPI | 1600 BPI | 3200 BPI |
| Speed   |   NRZI   |    PE    |    PE    |
|---------|----------|----------|----------|
| 25 in/s |   500    |   125    |    62    |
| 50 in/s |   250    |    62    |    31    |
|100 in/s |   125    |    31    |    15    |

### Inter-track skew (-s)

Due to tape heads magnetic gap misalignment and inaccuracies in tape vs. head aligment
tracks may become skewed. '-s' option allows you to specify maximum allowed skew
as a ratio of BPL.



