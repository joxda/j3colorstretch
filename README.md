# j3colorstretch
Stretches astronomical images while preserving the colors.

The algorithms are based on Roger N. Clark's rnc-color-stretch, which can be obtained from https://clarkvision.com/articles/astrophotography.software/rnc-color-stretch/ , and which is licenced under the GPL.

There are minor changes and updates to the algorithms, but the major difference is that this program here does not rely on davinci. It is coded with c++ using OpenCV and is significantly faster.

# Requirements

- OpenCV (probably version 3 or later)
- cmake (probably 3 or later)

# Compilation

Once the requirements are fulfilled, the code should compile simply by the following sequence

```
cmake .
make
```

This create the executable `j3colorstretch`. It can be installed to `/usr/local/bin/` by running `make install`.

# Usage

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
		desired zero point on sky, green channel
	--zeroskygreen (value:4096.0)
		desired zero point on sky, green channel
	--zeroskyred (value:4096.0)
```

To stretch an image the image name needs to be given as argument with any of the optional parameters listet above.

```
./j3colorstretch [parameters] IMAGEFILENAME
```

# ToDos
- do the installation thing
- illustrate?
- release? compiled versions?
- show histograms?
- mono versions?
- add log scale?
- CI action?
