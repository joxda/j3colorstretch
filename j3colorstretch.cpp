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
#include "opencv2/highgui.hpp"
#include <opencv2/core/ocl.hpp>

#include <algorithm>
#include <iostream>
#include <fstream>

#include "j3clrstrtch.hpp"


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
        _clp.about("\nThis program stretches astronomical images while preserving the colors.\n"); 

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
            out, CV_16UC1, 1, 65535.); // TBD factor?
    }

    cv::imwrite(ofile, out);
    return 0;
}

int writeJpg(const char* ofile, cv::Mat output)
{
    cv::Mat out;
    if (output.channels() == 3)
    {
        output.convertTo(out, CV_8UC3, 255.);
    }
    if (output.channels() == 1)
    {
        output.convertTo(
            out, CV_8UC1, 1, 255.); // TBD  factor?
    }

    cv::imwrite(ofile, out);
    return 0;
}

int readImage(const char* file, cv::Mat& image) {    
    image = cv::imread(file, cv::IMREAD_COLOR | cv::IMREAD_ANYDEPTH);
    if (image.empty()) {
		std::cout << "Error reading image." << std::endl;
		return -1;
	}    

    if (image.channels() == 1)
        {
            image.convertTo(image, CV_32FC1);
            cv::normalize(image, image, 0.0, 1.0, cv::NORM_MINMAX);
            // TBD FACTOR
        }
        else
        {
            image.convertTo(image, CV_32FC3); // TBD FACTOR...
            cv::normalize(image, image, 0.0, 1.0, cv::NORM_MINMAX);

        }
    return 0;
}

bool fexists(const std::string& filename) {
    std::ifstream ifile(filename.c_str());
    return (bool)ifile;
}

int main(int argc, char** argv)
{
    cv::String keys = "{help h usage   |        | print this message   }"
                      "{o output  |        | output image (without the result will be displayed, supports jpg and tif)}"
					  "{f               |       | force to overwrite output file}"
                      "{tc tonecurve   |        | application of a tone curve}"
                      "{sl skylevelfactor | 0.06 | sky level relative to the histogram peak  }"
                      "{zeroskyred    | 4096.0    | desired zero point on sky, red channel}"      
                      "{zeroskygreen  | 4096.0    | desired zero point on sky, green channel}"
                      "{zeroskyblue   | 4096.0    | desired zero point on sky, bue channel }"
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
                      "{x no-display    |        | no display}"
                      "{v verbose   |        | print some progress information }";
                      
    cv::ocl::setUseOpenCL(true);

    CustomCLP2 clp(argc, argv, keys);
    
    long int N = clp.n_positional_args();
    if (clp.get<bool>("help") || N==0)
    {
       clp.printMessage();
       return 0;
    }

    const bool verbose = clp.get<bool>("verbose");
	std::string ext = "";
    cv::String outf ;
	if (clp.has("o")) {
    	outf = clp.get<cv::String>("o");

		ext = outf.substr(outf.find_last_of(".")+1);
		if (ext != "jpg" && ext != "jpeg" && ext != "tif" && ext != "tiff")
		{
			std::cout << "    Unknown file extension" << std::endl;
			return -1;
		}
		const std::string ofile = std::string(outf.c_str());
		if ( fexists(ofile) && !clp.get<bool>("f"))
		{
			std::cout << "    File " << ofile << " exists" << std::endl;
			return -1;
		}
	}

    float skylevelfactor = clp.get<float>("sl");
    float skyLR = clp.get<float>("zeroskyred");
    float skyLG = clp.get<float>("zeroskygreen");
    float skyLB = clp.get<float>("zeroskyblue");

    //clp.errorCheck();

    cv::Mat thisIma;
    if(verbose) std::cout << "  Reading image" << clp.pos_args[0].c_str() << std::endl;
    if(readImage(clp.pos_args[0].c_str(),thisIma)<0)
		return -1;
    
    cv::UMat output_norm;
    cv::normalize(thisIma, output_norm, 1., 0., cv::NORM_MINMAX); // TBD

    if (clp.has("tc"))
    {
        if(verbose) std::cout << "    Applying tonecurve" << std::endl;
        toneCurve(output_norm,output_norm);
    }

    CVskysub(output_norm, output_norm, skylevelfactor, skyLR, skyLG, skyLB, verbose);
    cv::UMat colref;
    if (!clp.has("ncc") && output_norm.channels()==3)
    {   
        colref = output_norm.clone();
    }

    float rootpower = clp.get<float>("rp");
    float rootpower2 = rootpower;
    if(clp.has("rp2")) {
        rootpower2 = clp.get<float>("rp2");
    }
    float rtpwr;
    for(int i=0; i < clp.get<int>("ri"); i++) {
        rtpwr = i != 1 ? rootpower : rootpower2;
        if(verbose) std::cout << "    Image stretching iteration " << i+1 << std::endl;
        stretching(output_norm, output_norm, rootpower);
        CVskysub(output_norm, output_norm, skylevelfactor, skyLR, skyLG, skyLB, verbose);
    }
    
    float spwr, soff;
    float scurvepower1 = clp.get<float>("sc");
    float scurvepower2 = clp.get<float>("sc2");
    float scurveoff1 = clp.get<float>("so");
    float scurveoff2 = clp.get<float>("so2");
 
    for(int i=0; i < clp.get<int>("si"); i++) {
        if(verbose) std::cout << "    S-curve iteration " << i+1 << std::endl;
        spwr = i % 2 == 0 ? scurvepower1 : scurvepower2;
        soff = i % 2 == 0 ? scurveoff1 : scurveoff2;
        scurve(output_norm, output_norm, spwr, soff);
        CVskysub(output_norm, output_norm, skylevelfactor, skyLR, skyLG, skyLB, verbose);
    }
 
    float colorcorrectionfactor = clp.get<float>("ccf");

    if (!clp.has("ncc") && output_norm.channels()==3)
    {
        colorcorr(output_norm, output_norm, colref, colorcorrectionfactor, verbose);
        CVskysub(output_norm, output_norm, skylevelfactor, skyLR, skyLG, skyLB, verbose);
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

    if(verbose) std::cout << "  Writing " << outf.c_str() << std::endl;

	if (ext == "jpg" || ext == "jpeg") 
	{
		writeJpg(outf.c_str(), output_norm.getMat(cv::ACCESS_READ)); 
	} else if (ext == "tif" || ext == "tiff") {
		writeTif(outf.c_str(), output_norm.getMat(cv::ACCESS_READ));
	} else {
		cv::Mat c3;
    	output_norm.convertTo(c3, CV_8UC3, 255.);
    
    	cv::imshow("Output", c3);
    	cv::waitKey(0);
	}
    return 0;
}

