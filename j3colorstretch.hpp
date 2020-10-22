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


#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"

void hist(const cv::Mat& image, cv::Mat& hist, const bool blur)
{
    int histSize[] = {65536};
    float range[] = {0., 1.0}; // the upper boundary is exclusive
    const float* histRange[] = {range};
    bool uniform = true, accumulate = false;
    int channels[] = {0};
    cv::calcHist(&image, 1, channels, cv::Mat(), hist, 1, histSize, histRange,
        uniform, accumulate);

    if (blur)
    {
        cv::blur(hist, hist, cv::Size(1, 601), cv::Point(-1, -1),
            cv::BORDER_ISOLATED);
    }
}


inline int skyDN(const cv::Mat& hist, const float skylevel)
{
    cv::Point maxloc;
    int chistskydn = 0;
    cv::minMaxLoc(hist, 0, 0, 0, &maxloc);         
    const float* p = hist.ptr<float>(0, maxloc.y); 
    for (int ih = maxloc.y; ih >= 2; ih--)
    {
        float top = *p;
        *p--;
        float bottom = *p;
        if (top >= skylevel && bottom <= skylevel)
        {
            chistskydn = ih;
            break;
        }
    }
    return chistskydn;
}

inline int skyDN(
    const cv::Mat& hist, const float skylevelfactor, float& skylevel)
{
    cv::Point maxloc;
    int chistskydn = 0;
    double histGmax;
    cv::minMaxLoc(hist, 0, &histGmax, 0, &maxloc); 
    skylevel = (float)histGmax * skylevelfactor;   

    const float* p = hist.ptr<float>(0, maxloc.y); 
    for (int ih = maxloc.y; ih >= 2; ih--)
    {
        float top = *p;
        *p--;
        float bottom = *p;
        if (top >= skylevel && bottom <= skylevel)
        {
            chistskydn = ih;
            break;
        }
    }
    return chistskydn;
}


void toneCurve(const cv::Mat& inImage, cv::Mat& outImage)
{
    //         af*b*(1/d)^((af/c)^0.4)
    // b^z = exp( z * ln b )
    float fac = log(1.0 / 12.0);
    float b = 12.0;

    cv::Mat tmpImage, tmpImage3;
    cv::pow(inImage, 0.4, tmpImage3);
    cv::multiply(tmpImage3, fac, tmpImage);
    cv::exp(tmpImage, tmpImage3);
    cv::multiply(inImage, tmpImage3, tmpImage);
    cv::multiply(tmpImage, b, outImage);
}

void CVskysub1Ch(const cv::Mat& inImage, cv::Mat& outImage,
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
        
        float skylevel;
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

void CVskysub(const cv::Mat& inImage, cv::Mat& outImage,
    const float skylevelfactor, const float skyLR = 4096.0,
    const float skyLG = 4096.0,
    const float skyLB = 4096.0, const bool out=false) // TBD desired zero point in channels... ---
{ 
    if(inImage.channels()==1) {
        CVskysub1Ch(inImage,  outImage, skylevelfactor, skyLR);
        return; 
    }

    std::vector<cv::Mat> bgr_planes(3);

    cv::split(inImage, bgr_planes);

    cv::Mat r = bgr_planes[2];
    cv::Mat g = bgr_planes[1];
    cv::Mat b = bgr_planes[0];

    float zeroskyred = skyLR / 65535.0;
    float zeroskygreen = skyLG / 65535.0;
    float zeroskyblue = skyLB / 65535.0;
    
    if(out) std::cout << "    Sky sub iteration " << std::flush;
    for (int i = 1; i <= 25; i++)
    {
        if(out) std::cout << "|" << std::flush;
        cv::Mat r_hist, g_hist, b_hist;
        hist(r, r_hist, true);
        hist(g, g_hist, true);
        hist(b, b_hist, true);

        cv::Rect roi = cv::Rect(0, 400, 1, 65100); 
        cv::Mat r_hist_cropped = r_hist(roi);
        cv::Mat g_hist_cropped = g_hist(roi);
        cv::Mat b_hist_cropped = b_hist(roi);

        float skylevel;
        int chistredskydn, chistgreenskydn, chistblueskydn;

        chistgreenskydn = skyDN(g_hist_cropped, skylevelfactor, skylevel) + 400;
        chistredskydn = skyDN(r_hist_cropped, skylevel) + 400;
        chistblueskydn = skyDN(b_hist_cropped, skylevel) + 400;

	    if (  i>1 && (chistredskydn == 400 || chistgreenskydn == 400 || chistblueskydn == 400)) {
            std::cout << "    WARNING: histogram sky level not found" << std::endl;
            std::cout << "    Try increasing the -zerosky values" << std::endl;
           // break;        
        }

        if (pow(chistgreenskydn - skyLG, 2) <= 100 &&
            pow(chistredskydn - skyLR, 2) <= 100 &&
            pow(chistblueskydn - skyLB, 2) <= 100 && i > 1)
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

    std::vector<cv::Mat> channels;
    channels.push_back(b);
    channels.push_back(g);
    channels.push_back(r);

    cv::merge(channels, outImage);
    cv::max(outImage, 0.0, outImage);
}


void stretching(
    const cv::Mat& inImage, cv::Mat& outImage, const double rootpower)
{
    double x = 1. / rootpower;

    cv::Mat dim;
    if (rootpower > 30.)
    {
        inImage.convertTo(dim, CV_64F);
    }
    else
    {
        dim = inImage.clone();
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
        outImage = dim.clone();
    }
}

void scurve(const cv::Mat& inImage, cv::Mat& outImage, const float xfactor,
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


void colorcorr(const cv::Mat& inImage, cv::Mat& outImage, const cv::Mat& ref,
    const float colorenhance =
        1.0, const bool verbose=false) // possibly merge colorenhance with colorfactor?!?
{
    if(verbose) std::cout << "    Color correction " << std::flush;

    cv::Mat bgr_planes[3];
    cv::split(inImage, bgr_planes);

    cv::Mat bgr_planes_ref[3];
    cv::split(ref, bgr_planes_ref);

    float zeroskyred = 4096.0 / 65535.0;
    float zeroskygreen = 4096.0 / 65535.0;
    float zeroskyblue = 4096.0 / 65535.0;

    cv::Mat rref_bg, gref_bg, bref_bg;
    cv::subtract(bgr_planes_ref[2], zeroskyred, rref_bg);
    cv::subtract(bgr_planes_ref[1], zeroskygreen, gref_bg);
    cv::subtract(bgr_planes_ref[0], zeroskyblue, bref_bg);
    if(verbose) std::cout << "|" << std::flush;

    cv::Mat r_bg = bgr_planes[2];
    cv::Mat g_bg = bgr_planes[1];
    cv::Mat b_bg = bgr_planes[0];

    cv::Mat gr_ref, gr, br_ref, br, grratio, brratio;
    cv::divide(gref_bg, rref_bg, gr_ref);
    cv::divide(bref_bg, rref_bg, br_ref);
    cv::divide(g_bg, r_bg, gr);
    cv::divide(b_bg, r_bg, br);
    cv::divide(gr_ref, gr, grratio);
    cv::divide(br_ref, br, brratio);
    if(verbose) std::cout << "|" << std::flush;

    cv::Mat rg_ref, rg, bg_ref, bg, bgratio, rgratio;
    cv::divide(rref_bg, gref_bg, rg_ref);
    cv::divide(bref_bg, gref_bg, bg_ref);
    cv::divide(r_bg, g_bg, rg);
    cv::divide(b_bg, g_bg, bg);
    cv::divide(rg_ref, rg, rgratio);
    cv::divide(bg_ref, bg, bgratio);
    if(verbose) std::cout << "|" << std::flush;

    cv::Mat rb_ref, rb, gb_ref, gb, gbratio, rbratio;
    cv::divide(rref_bg, bref_bg, rb_ref);
    cv::divide(gref_bg, bref_bg, gb_ref);
    cv::divide(r_bg, b_bg, rb);
    cv::divide(g_bg, b_bg, gb);
    cv::divide(rb_ref, rb, rbratio);
    cv::divide(gb_ref, gb, gbratio);

    float zmin = 0.2;
    float zmax = 1.2;

    if(verbose) std::cout << "|" << std::flush;
    cv::max(grratio, zmin, grratio);
    cv::min(grratio, zmax, grratio);

    cv::max(brratio, zmin, brratio);
    cv::min(brratio, zmax, brratio);

    cv::max(rgratio, zmin, rgratio);
    cv::min(rgratio, zmax, rgratio);

    cv::max(bgratio, zmin, bgratio);
    cv::min(bgratio, zmax, bgratio);

    cv::max(gbratio, zmin, gbratio);
    cv::min(gbratio, zmax, gbratio);

    cv::max(rbratio, zmin, rbratio);
    cv::min(rbratio, zmax, rbratio);

    cv::Mat tmprg, lum;
    cv::add(r_bg, g_bg, tmprg);
    cv::add(tmprg, b_bg, lum);
    cv::max(lum, 0.0, lum);

    double maxlum;
    cv::minMaxLoc(lum, 0, &maxlum, 0, 0);
    cv::divide(lum, maxlum, lum);
    cv::pow(lum, 0.2, lum);
    cv::add(lum, 0.3, lum);
    cv::divide(lum, 1.3, lum);

    const float cfactor = 1.2;
    if(verbose) std::cout << "|" << std::flush;

    cv::Mat cfe;
    cv::multiply(lum, cfactor * colorenhance, cfe);

    cv::subtract(grratio, 1.0, grratio);
    cv::subtract(brratio, 1.0, brratio);
    cv::subtract(rgratio, 1.0, rgratio);
    cv::subtract(bgratio, 1.0, bgratio);
    cv::subtract(gbratio, 1.0, gbratio);
    cv::subtract(rbratio, 1.0, rbratio);

    cv::multiply(grratio, cfe, grratio);
    cv::multiply(brratio, cfe, brratio);
    cv::multiply(rgratio, cfe, rgratio);
    cv::multiply(bgratio, cfe, bgratio);
    cv::multiply(gbratio, cfe, gbratio);
    cv::multiply(rbratio, cfe, rbratio);

    cv::add(grratio, 1.0, grratio);
    cv::add(brratio, 1.0, brratio);
    cv::add(rgratio, 1.0, rgratio);
    cv::add(bgratio, 1.0, bgratio);
    cv::add(gbratio, 1.0, gbratio);
    cv::add(rbratio, 1.0, rbratio);
    if(verbose) std::cout << "|" << std::flush;

    cv::Mat c2gr, c3br, c1rg, c3bg, c1rb, c2gb;

    cv::multiply(g_bg, grratio, c2gr);
    cv::multiply(b_bg, brratio, c3br);

    cv::multiply(r_bg, rgratio, c1rg);
    cv::multiply(b_bg, bgratio, c3bg);

    cv::multiply(r_bg, rbratio, c1rb);
    cv::multiply(g_bg, gbratio, c2gb);

    if(verbose) std::cout << "|" << std::flush;

    for (int row = 0; row < r_bg.rows; row++)
    {
        float* r_bg_ptr = r_bg.ptr<float>(row);
        float* g_bg_ptr = g_bg.ptr<float>(row);
        float* b_bg_ptr = b_bg.ptr<float>(row);

        float* c2gr_ptr = c2gr.ptr<float>(row);
        float* c3br_ptr = c3br.ptr<float>(row);
        float* c1rg_ptr = c1rg.ptr<float>(row);
        float* c3bg_ptr = c3bg.ptr<float>(row);
        float* c1rb_ptr = c1rb.ptr<float>(row);
        float* c2gb_ptr = c2gb.ptr<float>(row);

        for (int col = 0; col < r_bg.cols; col++)
        {
            // We invert the blue and red values of the pixel
            if (*r_bg_ptr >= *g_bg_ptr && *r_bg_ptr >= *b_bg_ptr)
            {
                *g_bg_ptr = *c2gr_ptr;
                *b_bg_ptr = *c3br_ptr;
            }
            if (*g_bg_ptr > *r_bg_ptr && *g_bg_ptr >= *b_bg_ptr)
            {
                *r_bg_ptr = *c1rg_ptr;
                *b_bg_ptr = *c3bg_ptr;
            }
            if (*b_bg_ptr > *g_bg_ptr && *b_bg_ptr > *r_bg_ptr)
            {
                *g_bg_ptr = *c2gb_ptr;
                *r_bg_ptr = *c1rb_ptr;
            }

            r_bg_ptr++;
            g_bg_ptr++;
            b_bg_ptr++;

            c2gr_ptr++;
            c3br_ptr++;
            c1rg_ptr++;
            c3bg_ptr++;
            c1rb_ptr++;
            c2gb_ptr++;
        }
    }
    if(verbose) std::cout << "|" << std::flush;
    std::vector<cv::Mat> channels;
    channels.push_back(b_bg);
    channels.push_back(g_bg);
    channels.push_back(r_bg);
    cv::merge(channels, outImage);
    if(verbose) std::cout << std::endl;
}
