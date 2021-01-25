.. j3colorstretch documentation master file, created by
   sphinx-quickstart on Sun Jan 24 12:47:42 2021.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Documentation for j3colorstretch
==========================================

.. toctree::
   :maxdepth: 2
   :caption: Contents:


The code uses internally images represented in pixels with 32bit floating point values in the range from 0...1.
Any input values (e.g. target sky value etc) are referenced to 16bit integer values as they would be in 16bit TIFF
images (i.e. in the range from 0 to 65535), the most common type of input image. 

The main source code in ``j3colorstretch.cpp`` implements the command line parsing and image IO and calls the
image processing functions (implemented in ``j3clrstrch.cpp``) as needed. The following steps are run through in the
given order, all but the input output steps are optional and can be switched on/off and specified with parameters
per command line options: ::

	- read image
	|
	------ Apply tone curve 
	|      
	------ Sky subtraction
	|
	------ Root stretch (iterations)
	|      |
	|      ----- Sky subtraction
	| 
	------ S-curve (iterations)
	|      |
	|      ----- Sky subtraction
	|
	------ Set minimum reasonable pixel value
	| 
	------ Color correction
	|      |
	|      ----- Sky subtraction
	|
	- write image


Image processing functions in j3clrstrtch
-----------------------------------------
.. doxygenfile:: j3clrstrtch.hpp



Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
