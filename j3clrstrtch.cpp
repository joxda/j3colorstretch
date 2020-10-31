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


#include "opencv2/imgproc.hpp"
#include <iostream>
#include "opencv2/highgui.hpp"

#include <chrono>

void hist(cv::InputArray image, cv::OutputArray hist, const bool blur)
{
    cv::Mat ima = image.getMat();
    int histSize[] = {65536};
    float range[] = {0., 1.0}; // the upper boundary is exclusive
    const float* histRange[] = {range};
    bool uniform = true, accumulate = false;
    int channels[] = {0};
    cv::calcHist(&ima, 1, channels, cv::Mat(), hist, 1, histSize, histRange,
        uniform, accumulate);

    if (blur)
    {
        cv::blur(hist, hist, cv::Size(1, 301), cv::Point(-1, -1),
            cv::BORDER_ISOLATED);
    }
}


// Should the split images be used troughout??!
// plot histograms for checking...?
// would it be benefitial to use calcHist on all channels and
//    to run through the for loop once?
//    or to use parallel_for_?

inline int skyDN(
    cv::InputArray inHist, const float skylevelfactor, float& skylevel)
{
    cv::Mat hist = inHist.getMat();

    cv::Point maxloc;
    int chistskydn = 0;
    double histGmax;

    cv::minMaxLoc(hist, 0, &histGmax, 0, &maxloc); 
    if(skylevel < 0) {
        skylevel = (float)histGmax * skylevelfactor;   
    }
    const float* p = hist.ptr<float>(0, maxloc.y); 
    
    int lower = 2;
    int upper = maxloc.y;
    int diff = upper - lower;
    int mid;
    float* val;
    while(diff>1) {
        mid = lower + diff/2;
        val = hist.ptr<float>(0, mid);
        if(*val > skylevel) {
            upper = mid;
        } else {
            lower = mid;
        }
        diff = upper-lower;
    }
    chistskydn = mid;

    return chistskydn;
}

void toneCurve(cv::InputArray inImage, cv::OutputArray outImage)
{
    //         af*b*(1/d)^((af/c)^0.4)
    // b^z = exp( z * ln b )
    float fac = log(1.0 / 12.0);
    float b = 12.0;

    cv::UMat tmpImage, tmpImage3; // TBD UMat or Mat?
    cv::pow(inImage, 0.4, tmpImage3);
    cv::multiply(tmpImage3, fac, tmpImage);
    cv::exp(tmpImage, tmpImage3);
    cv::multiply(inImage, tmpImage3, tmpImage);
    cv::multiply(tmpImage, b, outImage);
}

void CVskysub1Ch(cv::InputArray inImage, cv::OutputArray outImage,
    const float skylevelfactor, const float sky = 4096.0, const bool out=false) 
{ 
    float zerosky = sky / 65535.0;
    
    if(out) std::cout << "  Sky sub iteration " << std::flush;
    for (int i = 1; i <= 25; i++)
    {
        if(out) std::cout << "|" << std::flush;

        cv::Mat histh;
        hist(inImage, histh, true);
        
        cv::Rect roi = cv::Rect(0, 400, 1, 65100); 
        cv::Mat hist_cropped = histh(roi);
        
        float skylevel=-1.;
        int chistskydn;
        chistskydn = skyDN(hist_cropped, skylevelfactor, skylevel) + 400;
        
        if (pow(chistskydn - sky, 2) <= 100)
            break;

        float chistskysub1 = chistskydn / 65535. - zerosky;
        
        float cfscale = 1.0 / (1.0 - chistskysub1);

        cv::subtract(inImage, chistskysub1, outImage);
        cv::multiply(outImage, cfscale, outImage);
    }
    if(out) std::cout << std::endl;

    cv::max(outImage, 0.0, outImage);
}

// TBD UMat or Mat?
void CVskysub(cv::InputArray inImage, cv::OutputArray outImage,
    const float skylevelfactor, const float skyLR = 4096.0,
    const float skyLG = 4096.0,
    const float skyLB = 4096.0, const bool out=false) // TBD desired zero point in channels... ---
{ 
    if(inImage.channels()==1) {
        CVskysub1Ch(inImage,  outImage, skylevelfactor, skyLR);
        return; 
    }

    std::vector<cv::UMat> bgr_planes(3);

    cv::split(inImage, bgr_planes);

    cv::UMat r = bgr_planes[2];
    cv::UMat g = bgr_planes[1];
    cv::UMat b = bgr_planes[0];

    float zeroskyred = skyLR / 65535.0;
    float zeroskygreen = skyLG / 65535.0;
    float zeroskyblue = skyLB / 65535.0;
    
    if(out) std::cout << "    Sky sub iteration " << std::flush;
    for (int i = 1; i <= 25; i++)
    {
        if(out) std::cout << "|" << std::flush;
        cv::UMat r_hist, g_hist, b_hist;
        hist(r, r_hist, true);
        hist(g, g_hist, true);
        hist(b, b_hist, true);

        cv::Rect roi = cv::Rect(0, 400, 1, 65100); 
        cv::UMat r_hist_cropped = r_hist(roi);
        cv::UMat g_hist_cropped = g_hist(roi);
        cv::UMat b_hist_cropped = b_hist(roi);

        float skylevel = -1.;
        int chistredskydn, chistgreenskydn, chistblueskydn;

        chistgreenskydn = skyDN(g_hist_cropped, skylevelfactor, skylevel) + 400;
        chistredskydn = skyDN(r_hist_cropped, skylevelfactor, skylevel) + 400;
        chistblueskydn = skyDN(b_hist_cropped, skylevelfactor, skylevel) + 400;

	    if (  i>1 && (chistredskydn == 400 || chistgreenskydn == 400 || chistblueskydn == 400)) {
            std::cout << "    WARNING: histogram sky level not found" << std::endl;
            std::cout << "    Try increasing the -zerosky values" << std::endl;
           // break;        
        }

        if (pow(chistgreenskydn - skyLG, 2) <= 10 &&
            pow(chistredskydn - skyLR, 2) <= 10 &&
            pow(chistblueskydn - skyLB, 2) <= 10 && i > 1)
            break; 

        float chistredskysub1 = chistredskydn / 65535. - zeroskyred;
        float chistgreenskysub1 = chistgreenskydn / 65535. - zeroskygreen;
        float chistblueskysub1 = chistblueskydn / 65535. - zeroskyblue;

        float cfscalered = 1.0 / (1.0 - chistredskysub1);
        float cfscalegreen = 1.0 / (1.0 - chistgreenskysub1);
        float cfscaleblue = 1.0 / (1.0 - chistblueskysub1);

        cv::subtract(r, chistredskysub1, r);
        cv::multiply(r, cfscalered, r);
        cv::subtract(g, chistgreenskysub1, g);
        cv::multiply(g, cfscalegreen, g);
        cv::subtract(b, chistblueskysub1, b);
        cv::multiply(b, cfscaleblue, b);
    }
    if(out) std::cout << std::endl;

    std::vector<cv::UMat> channels;
    channels.push_back(b);
    channels.push_back(g);
    channels.push_back(r);

    cv::merge(channels, outImage);
    cv::max(outImage, 0.0, outImage);
}


void stretching(
    cv::InputArray inImageA, cv::OutputArray outImage, const double rootpower)
{
    double x = 1. / rootpower;

    cv::UMat dim;
    if (rootpower > 30.)
    {
        cv::Mat inImage = inImageA.getMat();
        inImage.convertTo(dim, CV_64F);
    }
    else
    {
        dim = inImageA.getUMat();
    }

    cv::add(dim, 1.0 / 65535.0, dim);
    cv::divide(dim, (1. + 1.0 / 65535.), dim);
    cv::pow(dim, x, dim);

    double immin;
    cv::minMaxLoc(dim, &immin, 0, 0, 0); 

    immin -= 4096.0 / 65535.;               
    immin = immin > 0. ? immin : 0.;

    cv::subtract(dim, immin, dim);
    cv::divide(dim, (1. - immin), dim);

    if (rootpower > 30.)
    {
        dim.convertTo(outImage, CV_32F);
    }
    else
    {
        dim.copyTo(outImage);
    }
}

void setMin(cv::InputArray inImage, cv::OutputArray outImage, const float minr, const float ming, const float minb)
{
    std::vector<cv::Mat> bgr_planes(3);

    cv::split(inImage, bgr_planes);

    cv::Mat r_bg = bgr_planes[2];
    cv::Mat g_bg = bgr_planes[1];
    cv::Mat b_bg = bgr_planes[0];

	const float zx = 0.2;  // keep some of the low level, which is noise, so it looks more natural.

    cv::setNumThreads(-1);

    const int split = 7;
    const int row_split = r_bg.rows/split;

    parallel_for_(cv::Range(0, split+1), [&](const cv::Range& range){
        for (int n = range.start; n < range.end; n++)
        {
            int start = n * row_split;
            int stop = start + row_split;
            stop = stop < r_bg.rows ? stop : r_bg.rows;

            for (int row = start; row < stop; row++) 
            {
                float* r = r_bg.ptr<float>(row);
                float* g = g_bg.ptr<float>(row);
                float* b = b_bg.ptr<float>(row);



                for (int col = 0; col < r_bg.cols; col++)
                {
                    if ( *r < minr ) minr * zx * *r;
                    if ( *g < minr ) ming * zx * *g;
                    if ( *b < minr ) minb * zx * *b;

                    r++;
                    g++;
                    b++;
                }

            }       
        }
    }, 8);

    std::vector<cv::Mat> channels;
    channels.push_back(b_bg);
    channels.push_back(g_bg);
    channels.push_back(r_bg);

    cv::merge(channels, outImage);
}

void scurve(cv::InputArray inImage, cv::OutputArray outImage, const float xfactor,
    const float xoffset)
{
    float scurvemin =
        (xfactor / (1.0 + exp(-1.0 * (-xoffset * xfactor))) - (1.0 - xoffset));
    float scurvemax =
        (xfactor / (1.0 + exp(-1.0 * ((1.0 - xoffset) * xfactor))) -
            (1.0 - xoffset));

    float scurveminsc = scurvemin / scurvemax;
    float x0 = 1.0 - xoffset;

    cv::subtract(inImage, xoffset, outImage);
    cv::multiply(outImage, -xfactor, outImage);
    cv::exp(outImage, outImage);

    cv::add(outImage, 1.0, outImage);
    cv::divide(xfactor, outImage, outImage);

    cv::subtract(outImage, x0, outImage);
    cv::divide(outImage, scurvemax, outImage);

    cv::subtract(outImage, scurveminsc, outImage);
    cv::divide(outImage, (1.0 - scurveminsc), outImage);

    cv::max(outImage, 0.0, outImage);
}

void colorcorr(cv::InputArray inImage, cv::InputArray ref, cv::OutputArray outImage, const float skyLR = 4096.0,
    const float skyLG = 4096.0,
    const float skyLB = 4096.0,
    const float colorenhance =
        1.0, const bool verbose=false) // possibly merge colorenhance with colorfactor?!?
{
    if(verbose) std::cout << "    Color correction " << std::flush;
    
    cv::Mat inIma = inImage.getMat();

    cv::Mat bgr_planes[3];
    cv::split(inIma, bgr_planes);
    cv::Mat r_bg = bgr_planes[2];
    cv::Mat g_bg = bgr_planes[1];
    cv::Mat b_bg = bgr_planes[0];

    cv::Mat rf = ref.getMat();
    cv::Mat bgr_planes_ref[3];
    cv::split(rf, bgr_planes_ref);

    float zeroskyred = skyLR / 65535.0;   
    float zeroskygreen = skyLG / 65535.0;
    float zeroskyblue = skyLB / 65535.0;
    
    if(verbose) std::cout << "|" << std::flush;
   
    cv::Mat tmp, lum;
    cv::add(r_bg, g_bg, lum);
    cv::add(lum, b_bg, tmp);
    cv::max(tmp, 0.0, lum); 

    double maxlum;
    cv::minMaxLoc(lum, 0, &maxlum, 0, 0);
    cv::divide(lum, maxlum, tmp);
    cv::pow(tmp, 0.2, lum);
    cv::add(lum, 0.3, tmp);
    cv::divide(tmp, 1.3, lum);

    const float cfactor = 1.2;
    if(verbose) std::cout << "|" << std::flush;

    cv::Mat cfe;
    cv::multiply(lum, cfactor * colorenhance, cfe);

    const float ref_limit = 10./65355.;

    cv::setNumThreads(-1);

    const int split = 7;
    const int row_split = r_bg.rows/split;

    parallel_for_(cv::Range(0, split+1), [&](const cv::Range& range){
        for (int n = range.start; n < range.end; n++)
        {
            int start = n * row_split;
            int stop = start + row_split;
            stop = stop < r_bg.rows ? stop : r_bg.rows;

            for (int row = start; row < stop; row++) 
            {
                float* r = r_bg.ptr<float>(row);
                float* g = g_bg.ptr<float>(row);
                float* b = b_bg.ptr<float>(row);

                float* r_ref = bgr_planes_ref[2].ptr<float>(row);
                float* g_ref = bgr_planes_ref[1].ptr<float>(row);
                float* b_ref = bgr_planes_ref[0].ptr<float>(row);

                float* cfef = cfe.ptr<float>(row);

                for (int col = 0; col < r_bg.cols; col++)
                {
                    *r_ref -= zeroskyred;
                    *g_ref -= zeroskygreen;
                    *b_ref -= zeroskyblue;
            
                    *r_ref = *r_ref < ref_limit ? ref_limit : *r_ref;
                    *g_ref = *g_ref < ref_limit ? ref_limit : *g_ref;
                    *b_ref = *b_ref < ref_limit ? ref_limit : *b_ref;

                    if (*r >= *g && *r >= *b)
                    {           
                        float grratio = *g_ref / *r_ref / *g * *r;
                        float brratio = *b_ref / *r_ref / *b * *r;

                        grratio = grratio > 1.0 ? 1.0 : (grratio < 0.2 ? 0.2 : grratio);
                        brratio = brratio > 1.0 ? 1.0 : (brratio < 0.2 ? 0.2 : brratio);

                        *g = *g * ( (grratio - 1.) * *cfef  + 1.) ;
                        *b = *b * ( (brratio - 1.) * *cfef  + 1.) ;
                    }
                    else if (*g > *r && *g >= *b)
                    {
                        float rgratio = *r_ref / *g_ref / *r * *g;
                        float bgratio = *b_ref / *g_ref / *b * *g;

                        rgratio = rgratio > 1.0 ? 1.0 : (rgratio < 0.2 ? 0.2 : rgratio);
                        bgratio = bgratio > 1.0 ? 1.0 : (bgratio < 0.2 ? 0.2 : bgratio);

                        *r = *r * ( (rgratio - 1.) * *cfef  + 1.) ;
                        *b = *b * ( (bgratio - 1.) * *cfef  + 1.) ;
                    }
                    //if (b > g && b > r)
                    else 
                    {
                        float rbratio = *r_ref / *b_ref / *r * *b;
                        float gbratio = *g_ref / *b_ref / *g * *b;

                        rbratio = rbratio > 1.0 ? 1.0 : (rbratio < 0.2 ? 0.2 : rbratio);
                        gbratio = gbratio > 1.0 ? 1.0 : (gbratio < 0.2 ? 0.2 : gbratio);

                        *r = *r * ( (rbratio - 1.) * *cfef  + 1.) ;
                        *g = *b * ( (gbratio - 1.) * *cfef  + 1.) ;
                    }

                    r++;
                    g++;
                    b++;
                    r_ref++;
                    g_ref++;
                    b_ref++;
                    cfef++;
                }

            }       
        }
    }, 8);

    if(verbose) std::cout << "|" << std::flush;

    std::vector<cv::Mat> channels;
    channels.push_back(b_bg);
    channels.push_back(g_bg);
    channels.push_back(r_bg);
    
    cv::merge(channels, outImage) ;
    if(verbose) std::cout << std::endl;

    //auto end = std::chrono::steady_clock::now();
    //auto diff = end - start;
    //std::cout << std::chrono::duration <double, std::milli> (diff).count() << " ms" << std::endl;
}
