
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
This translates to 160kHz signal for a tape running at 50in/s.
1MS/s would be the lowest "safe" sampling frequency, at which resulting dump file
is 1GiB in size.

# Analyzing images

v9ttd input is a 16-bit LE binary file, where each word contains all 9 bits
of track signal in a given point in time. In other words: each 16-bit value is a
tape row sample. This format is used by Saleae software when exporting data to a binary file:

http://support.saleae.com/hc/en-us/articles/208667306-Binary-data-export-format-description

## Software requirements

v9ttd requires the following to work:

* python3
* numpy
* matplotlib (optional)

## v9ttd options

* **-h**/**--help** - Print help
* **-i**/**--input INPUT** - Input tape image
* **-o**/**--output OUTPUT** - Output file name or prefix
* **-O**/**--otype TYPE** - Output type (blocks, files, emimg, print)
* **-c**/**--chlist CHLIST** - Input channel list specified as: p,7,6,5,4,3,2,1,0 (p=parity track, 0=LSB track). Default is: 8,7,6,5,4,3,2,1,0
* **-d**/**--downsample DOWNSAMPLE** - Downsample input data by n > 1
* **-p**/**--pulselen PULSELEN** - Base pulse length (guessed if none specified)
* **-m**/**--pulsemargin PULSEMARGIN** -  Max allowed base pulse margin (0.0 - 0.5)
* **-s**/**--skew SKEW** - Max allowed track skew (0.0 - 0.5)
* **-S**/**--stats** - Print track stats and exit
* **-G**/**--gstats** - Draw track stats and exit (requires matplotlib)

## Running v9ttd

### Channel to track mapping

By default v9ttd assumes that logic analyzer probes were connected to the drive so that
logic channels 0-7 carried signals for data bits 2^0...2^7, and channel 8 carried the parity track:

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

### Downsampling

If your input is sampled with high frequency, for example 10MS/s, you may want to downsample it
before analysis to lower the memory usage and speed up the proces. To downsample a signal
by 5 (to 2MS/s), use:

    v9ttd -i input.bin -d 5

### Base pulse length and margin

### Tape skew

### Signal statistics

