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


    cv::String get(int index, bool space_delete) const
    {
        return cv::String(pos_args[index]);
    }

    unsigned long n_positional_args() const
    {
        return pos_args.size();
    }
};


int main(int argc, char** argv)
{
    cv::String keys = "{b | | bad pixel mask}" // TBD: MODIFY
                      "{f | | flat field}"
                      "{o | | output image}"
                      "{x | | no display}";
                      
                      
    CustomCLP2 clp(argc, argv, keys);

    cv::String outf = "out.fit";
    if (clp.has("o"))
    {
        outf = clp.get<cv::String>("o");
    }

    long int N = clp.n_positional_args();

    myImage thisIma; // TBD change to just open image...

    cv::Mat output_norm;
    cv::normalize(thisIma.image, output_norm, 1., 0., cv::NORM_MINMAX);

    // toneCurve(output_norm,output_norm);
    CVskysub(output_norm, output_norm, 0.06);
    cv::Mat colref = output_norm.clone();

    stretching(output_norm, output_norm, 12);

    CVskysub(output_norm, output_norm, 0.06);
    scurve(output_norm, output_norm, 5.0, 0.42);

    CVskysub(output_norm, output_norm, 0.06);

    scurve(output_norm, output_norm, 3.0, 0.22);
    CVskysub(output_norm, output_norm, 0.06);

    colorcorr(output_norm, output_norm, colref, 1.05);
    CVskysub(output_norm, output_norm, 0.06);
        
    thisIma.writeTif("out.tif", output_norm);
    
    cv::Mat c3;
    output_norm.convertTo(c3, CV_8UC3, 255.);
    //thisIma.writeTif("out.tif", output_norm); // TBD write jpg
    cv::imshow("Output", c3);
}

