Nine Track Lab lets you decode digital images of NRZ1
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
in digital form. For example, in Qualstar 1052 drive that would be the 9-pin connectors named ZCD and ENV.
Both sets of signals are viable for tape dumping, although there are some limitations.

Following ASCII drawing shows a general idea of where you need to tap in:

            analog          analog   .----------.            .--- LOGIC ANALYZER
            signals         signals  |  FLUX    |            |
                                 .-->| CHANGES  |------------+-----> ...
    .------.     .------------.  |   | DETECTOR |
    |      |     |            |  |   `----------'  digital
    | HEAD |---->| AMPLIFIERS |--+   .----------.  signals   .--- LOGIC ANALYZER
    |      |     |            |  |   |  CLOCK   |            |
    `------'     `------------'  `-->| FORMING  |------------+-----> ...
                                     `----------'

Connect the logic analyzer to all 9 signal pins - the order doesn't matter, tracks can be later reordered with
tool's '-t' option. Now you need to put the drive in a "free-running" mode, which lets the tape
to move over the heads with a constant speed and start recording logic analyzer output.

Note that this is not the only way of doing it. You may as well sample the analog signals,
digitize it on your own and prepare an image file readable by the software.

## Sampling frequency

Quality of the input data plays a great role in how successful the readout is.
In general, the higher sampling frequency the beter, but you need to consider final image size too.

For example:

1600bpi PE tape has a maximum od 3200 flux changes per inch.
This translates to max 160kHz signal for a tape running at 50in/s.
1MS/s would be the lowest "safe" practical sampling frequency, at which resulting dump file
is 1GiB in size.

# Building Nine Track Lab

To build Nine Track Lab from sources you'll need:

* Qt >= 4
* qmake
* GNU make

```
mkdir build
cd build
qmake ../ninetracklab
make
make install
```

# Analyzing tape images

Input is a 16-bit LE binary file, where each word contains all 9 bits
of track signal in a given point in time. In other words: each 16-bit value is a
tape row sample. This is a format used by Saleae software when exporting data to a binary file:

http://support.saleae.com/hc/en-us/articles/208667306-Binary-data-export-format-description

## Running Nine Track Lab

...

## Using Nine Track Lab

...

### Channel to track mapping

By default Nine Track Lab assumes that logic analyzer probes were connected to the drive so that
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

I you follow this setup, there is no need to change anything,
otherwise you need to provide proper channel to track mapping.

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

...

### Signal statistics

...

### Base pulse length and margin

Following table shows base pulse length values for various tape formats and speeds sampled at 10MS/s:

|  Tape Speed   |  800BPI NRZ1 | 1600BPI PE | 3200BPI PE |
| ------------- | ------------ | ---------- | ---------- |
|    25 in/s    |     500      |    125     |     62     |
|    50 in/s    |     250      |     62     |     31     |
|   100 in/s    |     125      |     31     |     15     |

### Inter-track skew

...
