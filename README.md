# j3colorstretch
Stretches astronomical images while preserving the colors.

https://joxda.github.io/j3colorstretch

![M31](https://joxda.github.io/j3colorstretch/images/M31_rp15_3_s2_c1.5.jpg)
(Raw images from Jerry Lodriguss, https://www.astropix.com/html/i_astrop/practice_files.html )

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=AVHSY5ZEGB482)

The algorithms are based on Roger N. Clark's rnc-color-stretch, which can be obtained from https://clarkvision.com/articles/astrophotography.software/rnc-color-stretch/ , and which is licenced under the GPL.

There are minor changes and updates to the algorithms, but the major difference is that this program here does not rely on davinci. It is coded with c++ using OpenCV and is significantly faster.


# How to obtain

You can try the binaries provided [here](https://github.com/joxda/j3colorstretch/releases/)
These were compiled on several Ubuntu Desktop and macOS systems and statically linked to the OpenCV libraries, so that there should not be any additional requirements. Once you downloaded the tar archive, you can unpack it (`tar xzvf j3colorstretch*tgz`) and move the batch script and j3colorstretch binary to your PATH. Alternatively you can download the source code from here and follow the instructions for compiling and installing below.

# Development

Developers can get a quick overview over the code in the documentation [here](https://joxda.github.io/j3colorstretch/doc/).

# Requirements for compilation

- OpenCV (version 3.0.0 or later)
- cmake (2.8.12 or later)

For example, on a fresh installation of Ubuntu 18.04 Desktop the following installs the requirements:
```
sudo apt install build-essential cmake libopencv-dev
```
On a macos with homebrew the following command should work:
```
brew install cmake opencv
```

# Compilation

Once the requirements are fulfilled, the code should compile simply by the following sequence

```shell
cmake .
make
```

If cmake fails to find OpenCV even though it is installed, it should help to modify the `OpenCV_DIR` path in the file `CMakeList.txt` to the path where the file `OpenCVConfig.cmake` can be found.
The `make` command creates the executable `j3colorstretch`, if successful. It can be installed by running `sudo make install`.

# Usage

Running the executable without any parameters prints out the following help message:

```
  Usage: j3colorstretch [params]

	--ccf, --color (value:1.0)
		default enhancement value
	-f
		force to overwrite output file
	-h, --help, --usage
		print this message
	--min
		set minimum in all channels (in 16bit)
	--minb
		set minimum b (in 16bit)
	--ming
		set minimum g (in 16bit)
	--minr
		set minimum r (in 16bit)
	--ncc, --nocolorcorrect
		turn off color correction
	--no-display, -x
		no display
	-o, --output
		output image (without the result will be displayed, supports jpg and tif)
	--ri, --rootiter (value:1)
		number of iterations on applying rootpower - sky
	--rootpower, --rp (value:6.0)
		power factor: 1/rootpower
	--rootpower2, --rp2
		use this power on iteration 2
	--sc, --scurvepower (value:5.0)
		scurve power odd iterations
	--sc2, --scurvepower2 (value:3.0)
		scurve power  even iterations
	--scurveiter, --si (value:0)
		number of iterations on applying scurve - sky
	--scurveoffset, --so (value:0.42)
		scurve offset odd iterations
	--scurveoffset2, --so2 (value:0.22)
		scurve offset even iterations
	--skylevelfactor, --sl (value:0.06)
		sky level relative to the histogram peak
	--tc, --tonecurve
		application of a tone curve
	-v, --verbose
		print some progress information
	--zerosky (value:4096.0)
		desired zero point on sky, sets all channels
	--zeroskyblue
		desired zero point on sky, bue channel
	--zeroskygreen
		desired zero point on sky, green channel
	--zeroskyred
		desired zero point on sky, red channel
```

To stretch an image the image name needs to be given as argument with any of the optional parameters listet above.

```shell
j3colorstretch [parameters] IMAGEFILENAME
```

The software should work with any file format that is understood by OpenCV, but in the most common usage case it will be a 16bit per channel RGB tiff file.

# Batch processing

A bash script ```batch-stretch``` is provided for batch processing. It includes an option to convert raw images with ```dcraw``` before running ```j3colorstretch```. Its call sequence is:

```
batch-stretch dir ext (tif or jpg) [dcraw] [j3colorstretch parameters]
```
It searches images with the extension ```ext``` in the directory ```dir``` and runs (```dcraw``` if the optional dcraw parameter is given and) ```j3colorstretch``` on them with the given optional parameters and saves the outputs as jpg or tif in the same directory as the original images.

[![ko-fi](https://www.ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/H2H5250BJ)
