# j3colorstretch

Under construction...


[![Donate](https://img.shields.io/badge/Donate-PayPal-900000.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=AVHSY5ZEGB482)


## Usage

Running the executable without any parameters prints out the following help message:

```
  Usage: j3colorstretch [params]

	--ccf, --color (value:1.0)
		default enhancement value
	-h, --help, --usage
		print this message
	--ncc, --nocolorcorrect
		turn off color correction
	--no-display, -x
		no display
	-o, --obase, --output (value:output)
		output image
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
	--zeroskyblue (value:4096.0)
		desired zero point on sky, blue channel
	--zeroskygreen (value:4096.0)
		desired zero point on sky, green channel
	--zeroskyred (value:4096.0)
		desired zero point on sky, red channel
```

To stretch an image the image name needs to be given as argument with any of the optional parameters listet above.

```shell
j3colorstretch [parameters] IMAGEFILENAME
```

The software should work with any file format that is understood by OpenCV, but in the most common usage case it will be a 16bit per channel RGB tiff file.

## Examples

The photos in the following examples were taken by Jerry Lodriguss and downloaded from [https://www.astropix.com/html/i_astrop/practice_files.html](https://www.astropix.com/html/i_astr\
op/practice_files.html). For the purpose of this illustration the raw images were stacked (without any bias/dark subtraction and without flatfielding) and the coadded images processed with `j3colorstretch`. The raw verions were saved as JPGs with `j3colorstretch input.tif--rootiter=0 --output=out.jpg`. From there the only step is to run `j3colorstretch`.

### M 42
Raw image
![M42_raw](/images/M42_raw.jpg)
A simple strong root stretch with
`j3colorstretch M42.tif --rootpower=300 --output=M42_rp300.jpg
![M42_j3colorstretch](/images/M42_rp300.jpg)                                                                                             


### M 45
Raw image
![M42_raw](/images/M45_raw.jpg)
Two iterations of root stretches with different powers with `j3colorstretch M45.tif --rootpower=100 --rootiter=2 --rootpower2=5 --output=M45_rp100_5.jpg`
![M42_j3colorstretch](/images/M45_rp100_5.jpg)                                                             

### M 31
Raw image
![M42_raw](/images/M31_raw.jpg)
Two iterations of root stretches, two iterations of s-curves and color enhanced with `j3colorstretch -rp=15 -ri=2 --rp2=3 -si=2 -ccf=1.5 -o=M31_rp15_3_s2_c1.5.jpg`
![M42_j3colorstretch](/images/M31_rp15_3_s2_c1.5.jpg)

## Compilation and Installation
See the [github pages](https://github.com/joxda/j3colorstretch).


[![ko-fi](https://www.ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/H2H5250BJ)
