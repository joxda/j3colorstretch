/*******************************************************************************
  Copyright(c) 2020 Joachim Janz. All rights reserved.
 
  The algorithms are based on Roger N. Clark's rnc-color-stretch, which can be 
  obtained from 
  https://clarkvision.com/articles/astrophotography.software/rnc-color-stretch/  
  and which is licenced under the GPL.
  
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

#ifndef libj3colorstretch_hpp
#define libj3colorstretch_hpp

#include "opencv2/core.hpp"

void hist(cv::InputArray image, cv::OutputArray hist, const bool blur);

inline int skyDN(
    cv::InputArray hist, const float skylevelfactor, float& skylevel);


void toneCurve(cv::InputArray inImage, cv::OutputArray outImage);

void CVskysub1Ch(cv::InputArray inImage, cv::OutputArray outImage,
    const float skylevelfactor, const float sky = 4096.0, const bool out=false);
void CVskysub(cv::InputArray inImage, cv::OutputArray outImage,
    const float skylevelfactor, const float skyLR = 4096.0,
    const float skyLG = 4096.0,
    const float skyLB = 4096.0, const bool out=false);

void stretching(
    cv::InputArray inImage, cv::OutputArray outImage, const double rootpower);

void scurve(cv::InputArray inImage, cv::OutputArray outImage, const float xfactor,
    const float xoffset);

void colorcorr(cv::InputArray inImage, cv::InputArray ref, cv::OutputArray outImage, 
    const float skyLR = 4096.0,
    const float skyLG = 4096.0,
    const float skyLB = 4096.0, const float colorenhance =
        1.0, const bool verbose=false);

#endif /* libj3colorstretch_hpp */
