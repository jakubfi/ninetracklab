
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
* **-c**/**--chlist CHLIST** - Input channel position list specified as: P,7,6,5,4,3,2,1,0 (P=parity, 0=LSB). Default is: 8,7,6,5,4,3,2,1,0
* **-d**/**--downsample DOWNSAMPLE** - Downsample input data by n > 1
* **-p**/**--pulselen PULSELEN** - Base pulse length (guessed if none specified)
* **-m**/**--pulsemargin PULSEMARGIN** -  Max allowed base pulse margin (0.0 - 0.5)
* **-s**/**--skew SKEW** - Max allowed track skew (0.0 - 0.5)
* **-S**/**--stats** - Print track stats and exit
* **-G**/**--gstats** - Draw track stats and exit (requires matplotlib)

## Running v9ttd


