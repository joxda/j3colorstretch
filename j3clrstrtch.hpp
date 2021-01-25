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

/** @file 
 * 
 * Functions that facilitate the steps in the background subtraction,
 * image stretching, and color preservation, as well as several helper functions.
 */

#ifndef libj3colorstretch_hpp
#define libj3colorstretch_hpp

#include "opencv2/core.hpp"

/**
 * @brief 
 * 
 * Calculates the histogram of the image data between 0 and 1 with 65536 bins. 
 * Optionally the histogram is smoothed.
 * 
 * @param[in] image Input image
 * @param[out] hist Output histogram
 * @param[in] blur Switch whether or not to blurr the histogram
 */
void hist(cv::InputArray image, cv::OutputArray hist, const bool blur);

/**
 * @brief Helper function to display RGB histograms
 * 
 * @param[in] im Input image
 * @param[in] window Name of the window
 */
void showHist(cv::InputArray im, const char* window);


/**
 * @brief Finds where the histogram reaches the skylevel
 * 
 * @param[in] inHist Input histogram
 * @param[in] skylevelfactor The skylevel is considered to be value corresponding the maximum in the histogram times the skylevelfactor, if the parameter skylevel < 0
 * @param[in,out] skylevel If > 0, it specifies the sky level directly
 * @return int Number of the bin where the skylevel was found
 *
 * @bug With 8bit input images that are stretch the histogram becomes sparse and the stretching is not sufficient to consistently find the skylevel.
*/
inline int skyDN(
    cv::InputArray hist, const float skylevelfactor, float &skylevel);


/**
 * @brief Set damp small pixel values in an image to avoid enhancing noise
 * The pixel values in the channels below a limit are damped by X = X * limit * zfac
 * 
 * @param[in] inImage Input image
 * @param[out] outImage Output image
 * @param[in] minr Red limit
 * @param[in] ming Green limit
 * @param[in] minb Blue limit
 */
void setMin(cv::InputArray inImage, cv::OutputArray outImage, const float minr, const float ming, const float minb);

/**
 * @brief Applies a simple gamma correction
 * X = X*12./(1/12.)^(X^0.4)
 * @param[in] inImage Input Image
 * @param[out] outImage Output Image
 */
void toneCurve(cv::InputArray inImage, cv::OutputArray outImage);

/**
 * @brief Subtracts for an image with a single channel the sky background and adjusts it to the requested skylevel
 * 
 * @param[in] inImage Input image
 * @param[out] outImage Output image
 * @param[in] skylevelfactor Skylevel will be considered to be the skylevelfactor times the value corresponding to the histogram maximum
 * @param[in] sky Target sky value (in 16bit, i.e. between 0 and 65535)
 * @param[in] out Switch progress information output
 */
void CVskysub1Ch(cv::InputArray inImage, cv::OutputArray outImage,
                 const float skylevelfactor, const float sky = 4096.0, const bool out = false);

/**
 * @brief Subtracts the sky background in an image and adjusts it to the requested skylevel
 * 
 * @param[in] inImage Input image
 * @param[out] outImage Output image
 * @param[in] skylevelfactor Skylevel will be considered to be the skylevelfactor times the value corresponding to the histogram maximum
 * @param[in] skyLR Target red sky value (in 16bit, i.e. between 0 and 65535)
 * @param[in] skyLG Target green sky value (in 16bit, i.e. between 0 and 65535)
 * @param[in] skyLB Target blue sky value (in 16bit, i.e. between 0 and 65535)
 * @param[in] out Switch progress information output
 */
void CVskysub(cv::InputArray inImage, cv::OutputArray outImage,
              const float skylevelfactor, const float skyLR = 4096.0,
              const float skyLG = 4096.0,
              const float skyLB = 4096.0, const bool out = false);


/**
 * @brief Set the Black Point object
 * The new pixel values are given by (X - bp)/(1 - bp) and clipped be larger than 0.
 * 
 * @param[in] inImage Input image
 * @param[out] outImage Output image
 * @param[in] bp Black point (in the range from 0 <= bp < 1)
 */
void setBlackPoint(cv::InputArray inImage, cv::OutputArray outImage, float bp);

/**
 * @brief Applies a root stretch
 * 
 * 
 * @param[in] inImageA Input image
 * @param[out] outImage Output image
 * @param[in] rootpower Root power of the stretch
 */
void stretching(
    cv::InputArray inImage, cv::OutputArray outImage, const double rootpower);


/**
 * @brief Applies an S-Curve stretch
 * 
 * @param[in] inImage Input image
 * @param[out] outImage Output image
 * @param[in] xfactor Factor parameter for the S-curve
 * @param[in] xoffset Offset parameter for the S-curve
 */
void scurve(cv::InputArray inImage, cv::OutputArray outImage, const float xfactor,
            const float xoffset);

/**
 * @brief Applies a colour correcetion
 * This essentially uses the colours before the images is stretched to correct for the 
 * less colourful bright parts in the image after stretching.
 * 
 * @param[in] inImage Input image (background subtracted and stretched)
 * @param[in] ref Referene image for the colours
 * @param[out] outImage Ouput image
 * @param[in] skyLR Red target sky level that which was used in the background subtraction
 * @param[in] skyLG Green target sky level that which was used in the background subtraction
 * @param[in] skyLB Blue target sky level that which was used in the background subtraction
 * @param[in] colorenhance Colour enhancement factor
 * @param[in] verbose Switch for verbose option
 */
void colorcorr(cv::InputArray inImage, cv::InputArray ref, cv::OutputArray outImage,
               const float skyLR = 4096.0,
               const float skyLG = 4096.0,
               const float skyLB = 4096.0, const float colorenhance =
                   1.0, const bool verbose = false);

#endif /* libj3colorstretch_hpp */
