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

#include <iostream>
#include <fstream>

#include "j3clrstrtch.hpp"


/**
 * @brief Trim white space from string
 *
 * @param[in] s Input string
 * @return Trimmed string
 */
inline std::string trim(const std::string &s)
{
    auto wsfront = std::find_if_not(s.begin(), s.end(), ::isspace);
    return std::string(wsfront,
                       std::find_if_not(
                           s.rbegin(), std::string::const_reverse_iterator(wsfront), ::isspace)
                       .base());
}


/**
 * @brief Class for parsing command line arguments
 *
 */
struct CustomCLP2
{
        /// OpenCV command line parser
        cv::CommandLineParser _clp;
        /// Vector to hold positional arguments
        std::vector<std::string> pos_args;

    public:
        /**
         * @brief Construct a command line parser object
         *
         * @param argc number of arguments
         * @param argv command line arguments
         * @param keys possible keyword arguments
         */
        CustomCLP2(int argc, const char* const argv[], const cv::String &keys)
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
            _clp.about("\nThis program stretches astronomical images while preserving the colors.\n\nThe OpenCV library is licensed under the Apache 2 license, https://github.com/opencv/opencv/blob/master/LICENSE\nSome algorthims are based on rnc-color-stretch, which is licensed under the GPL, https://clarkvision.com/articles/astrophotography.software/\nThis software is licensed under the GPLv3 and was retrieved from https://github.com/joxda/j3colorstretch\n");

        }

        /**
         * @brief Check for any parsing errors
         *
         * @return true
         * @return false
         */
        bool check() const
        {
            return _clp.check();
        }

        /**
         * @brief
         *
         * @param[in] name Name of keyword argument
         * @return true
         * @return false
         */
        bool has(const cv::String &name) const
        {
            return _clp.has(name);
        }

        template <typename T>
        T get(const cv::String &name, bool space_delete = true) const
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

        /**
         * @brief Print error messages if there were any
         *
         */
        void errorCheck()
        {
            if (!_clp.check())
            {
                _clp.printErrors();
            }
        }

        /**
         * @brief
         * Print messages
         *
         */
        void printMessage()
        {
            _clp.printMessage();
        }

        /**
         * @brief Get positional arguments by index
         *
         * @param index Index of the positional argument
         * @param space_delete Switch to strip white spaces
         * @return Value
         */
        cv::String get(int index, bool space_delete) const
        {
            return cv::String(pos_args[index]);
        }

        /**
         * @brief
         * Get number of positional arguments
         *
         * @return number
         */
        unsigned long n_positional_args() const
        {
            return pos_args.size();
        }
};

/**
 * @brief Scale image to 16 bit range and write tiff file
 *
 * @param[in] ofile File name
 * @param[in] output Image to be written
 * @return Status (0==OK)
 */
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
            out, CV_16UC1, 1, 65535.);
    }

    cv::imwrite(ofile, out);
    return 0;
}


/**
 * @brief Scale image to 8 bit range and write jpeg file
 *
 * @param[in] ofile File name
 * @param[in] output Image to be written
 * @return Status (0==OK)
 */
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
            out, CV_8UC1, 1, 255.);
    }

    cv::imwrite(ofile, out);
    return 0;
}

/**
 * @brief Read image and convert to 32 bit floating point
 *
 * @param[in] file File name
 * @param[in] image cv::Mat to hold the image
 * @return Status (0==OK)
 */
int readImage(const char* file, cv::Mat &image)
{
    image = cv::imread(file, cv::IMREAD_COLOR | cv::IMREAD_ANYDEPTH);
    if (image.empty())
    {
        std::cout << "Error reading image." << std::endl;
        return -1;
    }

    if (image.channels() == 1)
    {
        image.convertTo(image, CV_32FC1);
    }
    else
    {
        image.convertTo(image, CV_32FC3);
    }
    return 0;
}

/**
 * @brief Test whether file exists
 *
 * @param filename Filename
 * @return true
 * @return false
 */
bool fexists(const std::string &filename)
{
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
                      "{zerosky    | 4096.0    | desired zero point on sky, sets all channels}"
                      "{zeroskyred    |        | desired zero point on sky, red channel}"
                      "{zeroskygreen  |        | desired zero point on sky, green channel}"
                      "{zeroskyblue   |        | desired zero point on sky, bue channel }"
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
                      "{min    |        | set minimum in all channels (in 16bit)}"
                      "{minr   |        | set minimum r (in 16bit)}"
                      "{ming   |        | set minimum g (in 16bit)}"
                      "{minb   |        | set minimum b (in 16bit)}"
                      "{x no-display    |        | no display}"
                      "{v verbose   |        | print some progress information }";
    //                      "{bp blackpoint   |     0   | set blackpoint (in units..) }";

    cv::ocl::setUseOpenCL(true);

    CustomCLP2 clp(argc, argv, keys);

    long int N = clp.n_positional_args();
    if (clp.get<bool>("help") || N == 0)
    {
        clp.printMessage();
        return 0;
    }

    const bool verbose = clp.get<bool>("verbose");
    std::string ext = "";
    cv::String outf ;
    if (clp.has("o"))
    {
        outf = clp.get<cv::String>("o");

        ext = outf.substr(outf.find_last_of(".") + 1);
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
    float skyLR = clp.get<float>("zerosky");
    float skyLG = clp.get<float>("zerosky");
    float skyLB = clp.get<float>("zerosky");

    if(clp.has("zeroskyred")) skyLR = clp.get<float>("zeroskyred");
    if(clp.has("zeroskygreen")) skyLG = clp.get<float>("zeroskygreen");
    if(clp.has("zeroskyblue")) skyLB = clp.get<float>("zeroskyblue");

    //clp.errorCheck();

    cv::Mat thisIma;
    if(verbose) std::cout << "  Reading image" << clp.pos_args[0].c_str() << std::endl;
    if(readImage(clp.pos_args[0].c_str(), thisIma) < 0)
        return -1;

    cv::Mat output_norm;
    cv::normalize(thisIma, output_norm, 1., 0., cv::NORM_MINMAX); // TBD factor

    if(!clp.has("x"))    showHist(output_norm, "Input Image");

    if (clp.has("tc"))
    {
        if(verbose) std::cout << "    Applying tonecurve" << std::endl;
        toneCurve(output_norm, output_norm);
    }

    CVskysub(output_norm, output_norm, skylevelfactor, skyLR, skyLG, skyLB, verbose);
    cv::Mat colref;
    if (!clp.has("ncc") && output_norm.channels() == 3)
    {
        colref = output_norm.clone();
    }

    if(!clp.has("x"))    showHist(output_norm, "Skysub");

    float rootpower = clp.get<float>("rp");
    float rootpower2 = rootpower;
    if(clp.has("rp2"))
    {
        rootpower2 = clp.get<float>("rp2");
    }
    
    for(int i = 0; i < clp.get<int>("ri"); i++)
    {
        float rtpwr = i != 1 ? rootpower : rootpower2;
        if(verbose) std::cout << "    Image stretching iteration " << i + 1 << " (rootpower " << rtpwr << ")" <<  std::endl;
        stretching(output_norm, output_norm, rootpower);
        if(!clp.has("x"))    showHist(output_norm, "Stretched");
        CVskysub(output_norm, output_norm, skylevelfactor, skyLR, skyLG, skyLB, verbose);
        if(!clp.has("x"))    showHist(output_norm, "Skysub");
    }

    float scurvepower1 = clp.get<float>("sc");
    float scurvepower2 = clp.get<float>("sc2");
    float scurveoff1 = clp.get<float>("so");
    float scurveoff2 = clp.get<float>("so2");

    for(int i = 0; i < clp.get<int>("si"); i++)
    {
        float spwr = i % 2 == 0 ? scurvepower1 : scurvepower2;
        float soff = i % 2 == 0 ? scurveoff1 : scurveoff2;
        if(verbose) std::cout << "    S-curve iteration " << i + 1 << " (Power: " << spwr << " offset: " << soff << ")" <<
                                  std::endl;
        scurve(output_norm, output_norm, spwr, soff);
        if(!clp.has("x"))    showHist(output_norm, "S-curve");
        CVskysub(output_norm, output_norm, skylevelfactor, skyLR, skyLG, skyLB, verbose);
        if(!clp.has("x"))    showHist(output_norm, "Skysub");
    }

    if(clp.has("minr") || clp.has("minb") || clp.has("ming") || clp.has("min"))
    {
        float minr = 0;
        float ming = 0;
        float minb = 0;

        if(clp.has("min"))
        {
            minr = clp.get<float>("min") / 65535.;
            ming = clp.get<float>("min") / 65535.;
            minb = clp.get<float>("min") / 65535.;
        }
        if(clp.has("minr"))
        {
            minr = clp.get<float>("minr") / 65535.;
        }
        if(clp.has("ming"))
        {
            ming = clp.get<float>("ming") / 65535.;
        }
        if(clp.has("minb"))
        {
            minb = clp.get<float>("minb") / 65535.;
        }

        setMin(output_norm, output_norm, minr, ming, minb);

        if(!clp.has("x"))    showHist(output_norm, "Set min");
    }


    float colorcorrectionfactor = clp.get<float>("ccf");

    if (!clp.has("ncc") && output_norm.channels() == 3)
    {
        colorcorr(output_norm, colref, output_norm,  skyLR, skyLG, skyLB, colorcorrectionfactor, verbose);
        if(!clp.has("x"))    showHist(output_norm, "Color corrected");
        CVskysub(output_norm, output_norm, skylevelfactor, skyLR, skyLG, skyLB, verbose);
        if(!clp.has("x"))    showHist(output_norm, "Skubsub");
    }

    // TBD include option....
    //if(clp.get<float>("bp")>0) {
    //    setBlackPoint(output_norm, output_norm, clp.get<float>("bp")*4096/65535.);
    //    if(!clp.has("x"))    showHist(output_norm,"Set blackpoint");
    //}
    if(verbose) std::cout << "  Writing " << outf.c_str() << std::endl;

    if (ext == "jpg" || ext == "jpeg")
    {
        writeJpg(outf.c_str(), output_norm);
    }
    else if (ext == "tif" || ext == "tiff")
    {
        writeTif(outf.c_str(), output_norm);
    }
    else
    {
        cv::Mat c3;
        output_norm.convertTo(c3, CV_8UC3, 255.);

        cv::imshow("Output", c3);
        cv::waitKey(0);
    }
    return 0;
}

