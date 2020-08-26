/*******************************************************************************
  Copyright(c) 2020 Joachim Janz. All rights reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.

*******************************************************************************/

#include "opencv2/core.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

#include <algorithm>
#include <iostream>

#include "j3colorstretch.hpp"


inline std::string trim(const std::string& s)
{
    auto wsfront = std::find_if_not(s.begin(), s.end(), ::isspace);
    return std::string(wsfront,
        std::find_if_not(
            s.rbegin(), std::string::const_reverse_iterator(wsfront), ::isspace)
            .base());
}

struct CustomCLP2
{
    cv::CommandLineParser _clp;
    std::vector<std::string> pos_args;
    
  public:
    CustomCLP2(int argc, const char* const argv[], const cv::String& keys)
        : _clp(argc, argv, keys)
    {
        for (int i = 1; i < argc; ++i)
        {
            std::string s(argv[i]);
            s = trim(s);
            if (s[0] == '-')
                continue;

            pos_args.push_back(s);
        }
        _clp.about("\nThis program demonstrates the use of calcHist() -- histogram creation.\n"); // TBD

    }
    
    bool check() const
    {
        return _clp.check();
    }
    bool has(const cv::String& name) const
    {
        return _clp.has(name);
    }

    template <typename T>
    T get(const cv::String& name, bool space_delete = true) const
    {
        return _clp.get<T>(name, space_delete);
    }

    template <typename T>
    T get(int index, bool space_delete = true) const
    {
        std::stringstream ss;
        ss << pos_args[index];
        T t;
        ss >> t;
        return t;
    }

    void errorCheck() {
        if (!_clp.check())
        {
            _clp.printErrors();
        }
    }

    void printMessage()
    {
        _clp.printMessage();
    }

    cv::String get(int index, bool space_delete) const
    {
        return cv::String(pos_args[index]);
    }

    unsigned long n_positional_args() const
    {
        return pos_args.size();
    }
};


int writeTif(const char* ofile, cv::Mat output)
{
    cv::Mat out;
    if (output.channels() == 3)
    {
        output.convertTo(out, CV_16UC3, 65535.);
    }
    if (output.channels() == 1)
    {
        output.convertTo(
            out, CV_16UC1, 1, 65535.); // TBD reasonable values, log scaling?
    }

    cv::imwrite(ofile, out);
    return 0;
}

int writeJpg(const char* ofile, cv::Mat output)
{
    cv::Mat out;
    if (output.channels() == 3)
    {
        output.convertTo(out, CV_16UC3, 255.);
    }
    if (output.channels() == 1)
    {
        output.convertTo(
            out, CV_16UC1, 1, 255.); // TBD reasonable values, log scaling?
    }

    cv::imwrite(ofile, out);
    return 0;
}

 //if (cv::haveImageReader(file)) // TBD!! not in v3
int readImage(const char* file, cv::Mat& image) {
    image = cv::imread(file, cv::IMREAD_COLOR | cv::IMREAD_ANYDEPTH);
    if (image.empty())
        return -1;

    if (image.channels() == 1)
        {
            image.convertTo(image, CV_32FC1);
            cv::normalize(image, image, 0.0, 1.0, cv::NORM_MINMAX);
            // 1 / 65535.0); // TBD FACTOR... AND #channels TBD: FACTOR ALSO
            // APPLIES TO WEIGHT IMAGE...
        }
        else
        {
            image.convertTo(image, CV_32FC3); // TBD FACTOR...
            cv::normalize(image, image, 0.0, 1.0, cv::NORM_MINMAX);

        }
    return 0;
}

// TBD image scaling?
// CHECK output file exists...
// set parameters -- easiest with defaults from clp?
// plot histograms etc?

int main(int argc, char** argv)
{
    cv::String keys = "{help h usage   |        | print this message   }"
                      "{o output obase | output | output image}"
                      "{tc tonecurve   |        | application of a tone curve}"
                      "{sl skylevelfactor | 0.06 | sky level relative to the histogram peak  }"
                      "{ri rootiter    | 1      | number of iterations on applying rootpower - sky }"
                      "{rp rootpower   | 6.0    | power factor: 1/rootpower}"
                      "{rp2 rootpower2 |        | use this power on iteration 2}"
                      "{si scurveiter    | 0      | number of iterations on applying scurve - sky }"
                      "{sc scurvepower   | 5.0    | scurve power odd iterations}"
                      "{so scurveoffset  | 0.42    | scurve offset odd iterations}"
                      "{sc2 scurvepower2 | 3.0    | scurve power  even iterations}"
                      "{so2 scurveoffset2| 0.22   | scurve offset even iterations}"  
                      "{ccf color      | 1.0    | default enhancement value }" // PUT TOGETHER WITH COLOR FACTOR OF 1.2!!
                      "{ncc nocolorcorrect |     | turn off color correction }"
                      "{x no-display    |        | no display}";
                      
       /*
       pcntclip     = 0.005    # default percent clip level = total pixels * pcntclip/100

colorcorrect = 1        # default is to docolor correction.  (turn off to see the difference)

colorenhance = 1.0      # default enhancement value.
tonecurve    = 0        # no application odf a tone curve
specprhist   = 0        # no output histogram to specpr file
cumstretch   = 0        # do not do cumulative histogram stretch after rootpower
skylevelfactor = 0.06   # sky level relative to the histogram peak  (was 0.03 v0.88 and before)
rgbskylevel   = 1024.0    # desired  on a 16-bit  0 to 65535 scale
rgbskylevelrs = 4096.0    # desired  on output root stretched image 16-bit  0 to 65535 scale
zeroskyred    = 4096.0    # desired zero point on sky, red   channel
zeroskygreen  = 4096.0    # desired zero point on sky, green channel
zeroskyblue   = 4096.0    # desired zero point on sky, blue  channel
scurve        = 0         # no s-curve application
setmin        = 0         # no modification of the minimum
setminr = 0.0  # minimum for red
setming = 0.0  # minimum for green
setminb = 0.0  # minimum for blue
idisplay = 0   # do not display the final image
jpegonly = 0   # do not do jpeg only (will do jpeg + 16-bit png)
saveinputminussky  = 0  #  save input image - sky

debuga  = 0  # set to 1 for debugging, or 0 for none
doplots = 0  # show plots of histograms*/


/*  	printf ("      ERROR: need command line arguments\n")
	printf ("form:\n")
	printf ("      rnc-color-stretch   input_16bit_file [-rootpower x.x]  [-percent_clip p.p]\n")
	printf ("                                   -obase filename_noextension] [-enhance factor] [-tone] [-debug]\n")
	printf ("                                    [-specpr spfile] [-cumstretch] [-skylevelfactor x.x]\n")
	#printf ("                                    [-rgbskylevel x.x]\n")
	printf ("                                    [-rgbskyzero x.x x.x x.x]\n")
	printf ("                                    [-setmin R G B]\n")
	printf ("                                    [-nocolorcoerect]\n")
	printf ("                                    [-scurve1|-scurv2|scurve3|scurve4]\n")
	printf ("                                    [-jpegonly]  [-display]\n")
	printf ("\n")
        printf ("      -obase   outoput base file name\n")
	printf ("      -rootpower x.x  values 1.0 to 599.0, default =%f\n", rootpower)
	printf ("                     Use higher number to bring up the faint end more\n")
	printf ("      -rootpower2 x.x  Use this power on iteration 2 if the user sets it >1\n")
	printf ("      -rootiter   Number of iterations to apploy rootpower - sky, default =1\n")
	printf ("      -percent_clip p.p  allow no more than this percent of pixels to be clipped at zero.  default = %f%%\n", pcntclip)
	printf ("                       Note: zeros are ignored\n")
	printf ("      -enhance factor is >= 0.1. Values belwo 1 desaturate, above saturate.  default = %6.2f\n", colorenhance)
	printf ("      -specpr spfile  create specpr file and write histogram 0-65535 DN\n")
	printf ("               If spfile does not exist, it will be created\n")
	printf ("               note: sp_stamp program must be on the system\n")
	printf ("      -skylevelfactor  the sky histogram alignment level relative to peak histogram (0.000001 to 0.5)\n")
	printf ("                       skylevelfactor default = %f\n", skylevelfactor)
	#printf ("      -rgbskylevel  output image, neutralized dark sky DN level.  default= %f (OBSOLETE)\n", rgbskylevel)
	#printf ("                    NOTE: rgbskylevel gets overridden by rgbskyzero so is obsolete\n")
	printf ("      -rgbskyzero  final image zero point level.  default RGB= %8.1f %8.1f %8.1f\n", zeroskyred, zeroskygreen, zeroskyblue)
	printf ("                   In the core of the Milky Way, it might be something like the color of interstellar dust.\n")
	printf ("                   This might be something like 2500 4000 5500 (0 to 65535, 16-bit range)\n")
	printf ("      -cumulstretch do cumulative stretch after rootpower (not trecommended)\n")
	printf ("      -scurve1  apply an s-curve stretch as last step\n")
	printf ("      -scurve2  apply a stronger s-curve stretch as last step\n")
	printf ("      -scurve3  apply s-curve1 then 2, then 1 again as last step\n")
	printf ("      -scurve4  apply s-curve1 then 2, then 1, then 2 again as last step\n")
	printf ("      -setmin R G B set minimum levels to the 3 (floating point) values for red, green, blue\n")
	printf ("                    typical is about 20 on a 0 - 255 scale for darkest sky\n")
	printf ("                                        = 65535 * 20/255 = 5140 on 16-bit scale\n")
	printf ("      -jpegonly   do jpeg output only, no 16-bt png\n")
	printf ("      -display    display the final image\n")
	printf ("      -plots      display plots of the RGB histograms at each stage (needs davinci with gnuplot)\n")
	printf ("      -debug          turn on debug info and write debug images\n")
	printf ("      -nocolorcoerect turn off color correction to see the detrimental effect of stretching\n")
      printf ("\n  Example:\n")
	printf ("      rnc-color-stretch trestfile.tif -rootpower 6.0  -obase testresult4  -scurve1\n")
   
   
   
   printf ("\nFull command line:\n")
printf ("rnc-color-stretch ")
for (j = 1; j <= $argc; j = j +1 ) {

	printf (" %s", $argv[j])
}
printf ("\n\n")

if ( $argc > 1 ) {
   for (j = 2; j <= $argc; j = j +1 ) {

	if ($argv[j] == "-rootpower" || $argv[j] == "rootpower")   { 
		j = j +1
		rx = atof($argv[j])
		if (rx >= 0.9999 && rx <= 599.001) {
			rootpower  = rx
			rootpower1 = rx
			printf ("rootpower = %f\n", rootpower)
		} else {

			printf ("rootpower out of range.  Should be 1.0 to 599.0\n")
			printf ("exit 1\n")
			exit (1)
		}
	} else if ($argv[j] == "-rootiter" || $argv[j] == "rootiter") {
		j = j +1
		rootiter = atoi($argv[j])
		if (rootiter < 1 || rootiter > 2) {
			printf ("rootiter out of range.  Should be 1 to 2\n")
			printf ("exit 1\n")
			exit (1)
		} else {
			printf ("roopower-sky iteration= %d\n", rootiter)
		}
	} else if ($argv[j] == "-rootpower2" || $argv[j] == "rootpower2") {
		j = j +1
		rx = atof($argv[j])
		if (rx >= 1.0    && rx <= 599.001) {
			rootpower2 = rx
			printf ("rootpower2 = %f\n", rootpower2)
			if ( rootiter < 2) {
				rootiter = 2
				printf ("rootiter now set to %d\n", rootiter)
			}
		} else {

			printf ("rootpower2 out of range.  Should be >1.0 to 599.0\n")
			printf ("exit 1\n")
			exit (1)
		}
	} else if ($argv[j] == "-percent_clip" || $argv[j] == "percent_clip") {
		j = j +1
		px = atof($argv[j])    # 
		if (px >= 0.0 && px < 90.0) {
			pcntclip = px
			printf ("percent clip = %f\n", pcntclip)
		} else {

			printf ("pcntclip out of range.  Should be 0.0 to 90.0\n")
			printf ("exit 1\n")
			exit (1)
		}
	} else if ($argv[j] == "-obase" || $argv[j] == "obase") {
		j = j +1
		obase=$argv[j]
		printf ("output base file name = %s\n", obase)
	} else if ($argv[j] == "-enhance" || $argv[j] == "enhance") {
		j = j +1
		xcolor =atof($argv[j])
		colorenhance = xcolor
		printf ("color enhancement factor= %f\n", xcolor)
	} else if ($argv[j] == "-setmin" || $argv[j] == "setmin") {
		setminr = 0.0  # minimum for red
		setming = 0.0  # minimum for green
		setminb = 0.0  # minimum for blue
		setmin = 1
		j = j +1
		setminr =atof($argv[j])
		j = j +1
		setming =atof($argv[j])
		j = j +1
		setminb =atof($argv[j])
		if ( setminr < 0.0 || setminr > 20000.0 ||     \
			setming < 0.0 || setming > 20000.0 ||  \
			setminb < 0.0 || setminb > 20000.0 ) {
				printf ("ERROR: minimum values out of range.  Must be within 0 to 20000, typical is 5000\n")
				printf ("minimum RGB levels on output= %f %f %f\n", setminr, setming, setminb)
			printf ("exit 1\n")
			exit (1)
		} else {
			printf ("minimum RGB levels on output= %f %f %f\n", setminr, setming, setminb)
		}
	} else if ($argv[j] == "-skylevelfactor" || $argv[j] == "skylevelfactor") {
		j = j +1
		skylevelfactor =atof($argv[j])
		printf ("Histogram sky level fraction from histogram peak= %f\n", skylevelfactor)
		if (skylevelfactor < 0.000001 || skylevelfactor > 0.7) {

			printf ("skylevelfactor is out of range.  Range is 0.000001 to 0.7\n")
			printf ("exit 1\n")
			exit (1)
		} else {
			printf ("skylevelfactor= %f\n", skylevelfactor)
		}
	} else if ($argv[j] == "-rgbskylevel" || $argv[j] == "rgbskylevel") {
		j = j +1
		rgbskylevel =atof($argv[j])
		printf ("Output sky level = %f\n", rgbskylevel)
		if (rgbskylevel < 0.1 || rgbskylevel > 20000.0) {

			printf ("rgbskylevel is out of range.  Range is 001 to 20000.0\n")
			printf ("exit 1\n")
			exit (1)
		} else {
			printf ("rgbskylevel= %8.1f DN\n", rgbskylevel)
		}
	} else if ($argv[j] == "-rgbskyzero" || $argv[j] == "rgbskyzero") {
		j = j +1
		zeroskyred   =atof($argv[j])
		j = j +1
		zeroskygreen =atof($argv[j])
		j = j +1
		zeroskyblue  =atof($argv[j])
		if (zeroskyred > 0 && zeroskyred < 25000 && zeroskygreen > 0 && zeroskygreen < 25000 && \
			zeroskyblue > 0 && zeroskyblue < 25000) {
			printf ("target image zero point level before stretch RGB= %8.1f %8.1f %8.1f\n", zeroskyred, zeroskygreen, zeroskyblue)
		} else { 
			printf ("ERROR: -rgbskyzero: out of range (0 to 25000): = %8.1f %8.1f %8.1f\n", zeroskyred, zeroskygreen, zeroskyblue)
			printf ("exit 1\n")
			exit (1)
		}
	} else if ($argv[j] == "-cumulstretch" || $argv[j] == "cumulstretch") {
		cumstretch   = 1
	} else if ($argv[j] == "-debug" || $argv[j] == "debug") {
		debuga   = 1
		printf ("debuging on\n")
	} else if ($argv[j] == "-display" || $argv[j] == "display") {
		idisplay = 1
		printf ("will display final image\n")
	} else if ($argv[j] == "-plots" || $argv[j] == "plots") {
		doplots = 1
		printf ("will plot histograms\n")
	} else if ($argv[j] == "-jpegonly" || $argv[j] == "jpegonly") {
		jpegonly = 1
		printf ("will output jpeg image only\n")
	} else if ($argv[j] == "-nocolorcoerect" || $argv[j] == "nocolorcoerect") {
		colorcorrect = 0
		printf ("color correction turned off\n")
	} else if ($argv[j] == "-tone" || $argv[j] == "tone") {
		tonecurve = 1  # apply a tone curve to the data
		printf ("Will apply a tone curve\n")
	} else if ($argv[j] == "-scurve1" || $argv[j] == "scurve1") {
		scurve = 1
		printf ("Will apply an s-curve curve 1\n")
	} else if ($argv[j] == "-scurve2" || $argv[j] == "scurve2") {
		scurve = 2
		printf ("Will apply a strong s-curves 1 and 2\n")
	} else if ($argv[j] == "-scurve3" || $argv[j] == "scurve3") {
		scurve = 3
		printf ("Will apply a s-curves 1, then 2, then 1 again\n")
	} else if ($argv[j] == "-scurve4" || $argv[j] == "scurve4") {
		scurve = 4
		printf ("Will apply a s-curves 1, then 2, then 1, then1 again\n")
	} else if ($argv[j] == "-saveinputminussky" || $argv[j] == "saveinputminussky") {
		saveinputminussky = 1
		printf ("Will save input image - sky\m")
	} else if ($argv[j] == "-specpr" || $argv[j] == "specpr") {
		j = j +1
		if ( hasvalue($argv[j]) == 1 ) {
			spfile = $argv[j]
			spe= fexists(spfile)
			if ( spe == 0 ) {
				printf ("  creating specpr file %s\n", spfile)
				s=sprintf ("cp /dev/null %s", spfile)
				printf ("%s\n", s)
				system (s)
				s=sprintf ("sp_stamp %s", spfile)
				printf ("%s\n", s)
				system (s)
			}
			printf ("Will write histograms to specpr file %s\n", spfile)
			specprhist = 1  # output histogram to specpr file
		} else {
			printf ("ERROR: no specpr file name after -specpr\n")
			printf ("exit 1\n")
			exit (1)
		}
	} else  {
		printf ("ERROR: option not recognized: %s\n", $argv[j])
		printf ("exit 1\n")
		exit (1)

	}
   }
}

ofe = fexists(filename=$1)    # test if input file exists
if ( ofe == 0 ) {  # file does not exist
	printf ("ERROR: can not find input file  %s\n", $1)
	printf ("EXITING\n")
	exit(2)
} */


    CustomCLP2 clp(argc, argv, keys);

   if (clp.get<bool>("help"))
   {
       std::cout << "Help" << std::endl;
       clp.printMessage();
       return 0;
   }

    // TBD add later to the outfile names...
    cv::String outf = clp.get<cv::String>("o");

    float skylevelfactor = clp.get<float>("sl");
    std::cout << "Skylevelfactor: " << skylevelfactor << std::endl;

    long int N = clp.n_positional_args();


    if (N == 0)
        {
            std::cout << "NO ARGUMENTS" << std::endl;
            return -1;
        }

    //clp.errorCheck();

    cv::Mat thisIma; 
    readImage(clp.pos_args[0].c_str(),thisIma);
    
    cv::Mat output_norm;
    cv::normalize(thisIma, output_norm, 1., 0., cv::NORM_MINMAX); // TBD

    if (clp.has("tc"))
    {
        toneCurve(output_norm,output_norm);
    }

    CVskysub(output_norm, output_norm, skylevelfactor);
    cv::Mat colref;
    if (~clp.has("ncc") && output_norm.channels()==3)
    {   
        colref = output_norm.clone();
    }

// TBD write some outputs...

    float rootpower = clp.get<float>("rp");
    float rootpower2 = rootpower;
    if(clp.has("rp2")) {
        rootpower2 = clp.get<float>("rp2");
    }
    float rtpwr;
    for(int i=0; i < clp.get<int>("ri"); i++) {
        rtpwr = i != 1 ? rootpower : rootpower2;
        stretching(output_norm, output_norm, rootpower);
        CVskysub(output_norm, output_norm, skylevelfactor);
    }
    
    float spwr, soff;
    float scurvepower1 = clp.get<float>("sc");
    float scurvepower2 = clp.get<float>("sc2");
    float scurveoff1 = clp.get<float>("so");
    float scurveoff2 = clp.get<float>("so2");
 
    for(int i=0; i < clp.get<int>("si"); i++) {
        spwr = i % 2 == 0 ? scurvepower1 : scurvepower2;
        soff = i % 2 == 0 ? scurveoff1 : scurveoff2;
        scurve(output_norm, output_norm, spwr, soff);
        CVskysub(output_norm, output_norm, skylevelfactor);
    }
 
    float colorcorrectionfactor = clp.get<float>("ccf");

    if (~clp.has("ncc") && output_norm.channels()==3)
    {
        colorcorr(output_norm, output_norm, colref, colorcorrectionfactor);
        CVskysub(output_norm, output_norm, skylevelfactor);
    }

/*if ( setmin > 0) {   # this makes sure there are no really dark pixels.
			# this happens typically from noise and color matrix
			# application (in the raw converter) around stars showing
			# chromatic aberration.
	printf ("applying set minimum after color correction\n")
	printf ("minimum RGB levels on output= %f %f %f\n", setminr, setming, setminb)
	cr=c[,,1]  # red
	cg=c[,,2]  # green
	cb=c[,,3]  # blue

	zx = 0.2  # keep some of the low level, which is noise, so it looks more natural.

	cr[ where ( cr < setminr )] = setminr + zx * cr   # minimum for red
	cg[ where ( cg < setming )] = setming + zx * cg   # minimum for green
	cb[ where ( cb < setminb )] = setminb + zx * cg   # minimum for blue

	# now put back together

	c=cat(cr,cg, axis=z)
	c=cat(c, cb, axis=z)
	c=bip(c)

	cr = 0 # clear memory
	cg = 0 # clear memory
	cb = 0 # clear memory
}*/


    writeTif("out.tif", output_norm);
    
    cv::Mat c3;
    output_norm.convertTo(c3, CV_8UC3, 255.);
    writeJpg("out.jpg", output_norm); // TBD write jpg
    cv::imshow("Output", c3);
    cv::waitKey(0);
}

