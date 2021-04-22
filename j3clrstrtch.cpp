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
//adjust blackpoint?
/*
Then, for each channel value R, G, B (0-255) for each pixel, do the following in order.

Apply the input levels:

ChannelValue = 255 * ( ( ChannelValue - ShadowValue ) /
    ( HighlightValue - ShadowValue ) )
Apply the midtones:

If Midtones <> 128 Then
    ChannelValue = 255 * ( Pow( ( ChannelValue / 255 ), GammaCorrection ) )
End If
Apply the output levels:

ChannelValue = ( ChannelValue / 255 ) *
    ( OutHighlightValue - OutShadowValue ) + OutShadowValue*/

#include "opencv2/imgproc.hpp"
#include <iostream>
#include "opencv2/highgui.hpp"
//#include <chrono>
#include <opencv2/core/cvdef.h>


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
        int border = CV_MAJOR_VERSION > 3 ? cv::BORDER_ISOLATED : cv::BORDER_REFLECT;
        cv::blur(hist, hist, cv::Size(1, 601), cv::Point(-1, -1), border);
    }
}

void setBlackPoint(cv::InputArray inImage, cv::OutputArray outImage, float bp)
{
    cv::subtract(inImage, cv::Scalar::all(bp), outImage);
    cv::divide(outImage, cv::Scalar::all(1 - bp), outImage);
    cv::max(outImage, 0, outImage);
}


// Should the split images be used troughout??!
// would it be benefitial to use calcHist on all channels and
//    to run through the for loop once?
//    or to use parallel_for_?


/**
 * @brief Class with the code for the color correction to be run by OpenCV's parallel_for_
 *
 */
class ParallelColorCorr : public cv::ParallelLoopBody
{
    public:
        /**
         * @brief Construct a new Parallel Color Corr object
         *
         * @param r_bg Input background subtracted stretched red iamge
         * @param g_bg Input background subtracted stretched green iamge
         * @param b_bg Input background subtracted stretched blue iamge
         * @param r_bg_ref Input background subtracted red reference iamge
         * @param g_bg_ref Input background subtracted green reference iamge
         * @param b_bg_ref Input background subtracted blue reference iamge
         * @param cfe Input matrix with the color correction factor (depending on user choice and luminosity of the pixel)
         * @param zeroskyred Red target sky level that which was used in the background subtraction
         * @param zeroskygreen Green target sky level that which was used in the background subtraction
         * @param zeroskyblue Blue target sky level that which was used in the background subtraction
         * @param ref_limit Lower limit for the values in the reference image
         * @param row_split Number of groups of rows which can be processed in parallel
         */
        ParallelColorCorr (cv::Mat &r_bg, cv::Mat &g_bg, cv::Mat &b_bg, cv::Mat &r_bg_ref, cv::Mat &g_bg_ref, cv::Mat &b_bg_ref,
                           cv::Mat &cfe, const float zeroskyred, const float zeroskygreen, const float zeroskyblue, const float ref_limit,
                           const int row_split) : r_bg(r_bg), g_bg(g_bg), b_bg(b_bg), r_bg_ref(r_bg_ref), g_bg_ref(g_bg_ref), b_bg_ref(b_bg_ref),
            cfe(cfe), zeroskyred(zeroskyred), zeroskygreen(zeroskygreen), zeroskyblue(zeroskyblue), ref_limit(ref_limit),
            row_split(row_split)
        {}
        virtual void operator ()(const cv::Range &range) const override
        {
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

                    float* r_ref = r_bg_ref.ptr<float>(row);
                    float* g_ref = g_bg_ref.ptr<float>(row);
                    float* b_ref = b_bg_ref.ptr<float>(row);

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
                        else
                        {
                            float rbratio = *r_ref / *b_ref / *r * *b;
                            float gbratio = *g_ref / *b_ref / *g * *b;

                            rbratio = rbratio > 1.0 ? 1.0 : (rbratio < 0.2 ? 0.2 : rbratio);
                            gbratio = gbratio > 1.0 ? 1.0 : (gbratio < 0.2 ? 0.2 : gbratio);

                            *r = *r * ( (rbratio - 1.) * *cfef  + 1.) ;
                            *g = *g * ( (gbratio - 1.) * *cfef  + 1.) ;
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
        }
        ParallelColorCorr &operator=(const ParallelColorCorr &)
        {
            return *this;
        };
    private:
        cv::Mat &r_bg, &g_bg, &b_bg, &r_bg_ref, &g_bg_ref, &b_bg_ref, &cfe;
        float zeroskyred, zeroskygreen, zeroskyblue, ref_limit;
        int row_split;
};

/**
 * @brief Class with the code for setting the minimum to be run by OpenCV's parallel_for_
 *
 */
class ParallelSetMin : public cv::ParallelLoopBody
{
    public:
        /**
         * @brief Construct a new Parallel Set Min object
         * The pixel values in the channels below a limit are damped by X = X * limit * zfac
         *
         * @param rbg Red input image
         * @param gbg Green input image
         * @param bbg Blue input image
         * @param rowsplit Number of groups of rows to be processes in parallel
         * @param mr Red limit
         * @param mg Grren limit
         * @param mb Blue limit
         * @param zfac Dampening factor
         */
        ParallelSetMin ( cv::Mat &rbg, cv::Mat &gbg, cv::Mat &bbg, const int rowsplit, const float mr,  const float mg,
                         const float mb, const float zfac) : r_bg(rbg), g_bg(gbg), b_bg(bbg), row_split(rowsplit), minr(mr), ming(mg), minb(mb),
            zx(zfac)
        {
        }

        virtual void operator ()(const cv::Range &range) const override
        {
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
        }

        ParallelSetMin &operator=(const ParallelSetMin &)
        {
            return *this;
        };
    private:
        cv::Mat &r_bg, &g_bg, &b_bg;
        int row_split;
        float minr, ming, minb, zx;
};


inline int skyDN(
    cv::InputArray inHist, const float skylevelfactor, float &skylevel)
{
    cv::Mat histH = inHist.getMat();

    cv::Point maxloc;
    int chistskydn = 0;
    double histGmax;

    cv::minMaxLoc(histH, 0, &histGmax, 0, &maxloc);
    if(skylevel < 0)
    {
        skylevel = (float)histGmax * skylevelfactor;
    }

    const float* p = histH.ptr<float>(0, maxloc.y);
    for (int ih = maxloc.y; ih >= 2; ih--)
    {
        float top = *p;
        p--;
        float bottom = *p;
        if (top >= skylevel && bottom <= skylevel)
        {
            chistskydn = ih;
            break;
        }
    }
    return chistskydn;
}


void toneCurve(cv::InputArray inImage, cv::OutputArray outImage)
{
    /// X*b*(1/12.)^(X^0.4)
    /// uses: b^z = exp( z * ln b )
    float fac = log(1.0 / 12.0);
    float b = 12.0;

    cv::Mat tmpImage, tmpImage3; // TBD UMat or Mat?
    cv::pow(inImage, 0.4, tmpImage3);
    cv::multiply(tmpImage3, fac, tmpImage);
    cv::exp(tmpImage, tmpImage3);
    cv::multiply(inImage, tmpImage3, tmpImage);
    cv::multiply(tmpImage, b, outImage);
}


void CVskysub1Ch(cv::InputArray inImage, cv::OutputArray outImage,
                 const float skylevelfactor, const float sky = 4096.0, const bool out = false)
{
    if(out) std::cout << "  Sky sub iteration " << std::flush;
    for (int i = 1; i <= 25; i++)
    {
        if(out) std::cout << "|" << std::flush;

        cv::Mat histh;
        hist(inImage, histh, true);

        cv::Rect roi = cv::Rect(0, 400, 1, 65100);
        cv::Mat hist_cropped = histh(roi);

        float skylevel = -1.;
        int chistskydn;
        chistskydn = skyDN(hist_cropped, skylevelfactor, skylevel) + 400;

        if (pow(chistskydn - sky, 2) <= 25)
            break;

        float chistskysub1 = (chistskydn - sky ) / 65535.;

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
              const float skyLB = 4096.0, const bool out = false)
{
    if(inImage.channels() == 1)
    {
        CVskysub1Ch(inImage,  outImage, skylevelfactor, skyLR);
        return;
    }

    std::vector<cv::Mat> bgr_planes(3);

    cv::split(inImage, bgr_planes);

    cv::Mat r = bgr_planes[2];
    cv::Mat g = bgr_planes[1];
    cv::Mat b = bgr_planes[0];

    if(out) std::cout << "    Sky sub iteration " << std::flush;
    for (int i = 1; i <= 25; i++)
    {
        if(out) std::cout << "|" << std::flush;
        cv::Mat r_hist, g_hist, b_hist;
        // histograms use 65535 bins corresponding to 16bits (pixel values should be in the range from 0 to 1)
        hist(r, r_hist, true);
        hist(g, g_hist, true);
        hist(b, b_hist, true);

        // Histrograms are igroring the first 400 and last about 400 bins
        // to avoid problems with saturated or clipped pixels
        cv::Rect roi = cv::Rect(0, 400, 1, 65100);
        cv::Mat r_hist_cropped = r_hist(roi);
        cv::Mat g_hist_cropped = g_hist(roi);
        cv::Mat b_hist_cropped = b_hist(roi);

        float skylevel = -1.;
        int chistredskydn, chistgreenskydn, chistblueskydn;

        // Green is the reference channel
        // offset the value by 400 to account fot the clipping of the histogram above
        chistgreenskydn = skyDN(g_hist_cropped, skylevelfactor, skylevel) + 400;

        //parallel_for_(cv::Range(0, 2), [&](const cv::Range& range){
        //for (int n = range.start; n < range.end; n++)
        //{
        //    if(n==1) {
        chistredskydn = skyDN(r_hist_cropped, skylevelfactor, skylevel) + 400;
        //    } else {
        chistblueskydn = skyDN(b_hist_cropped, skylevelfactor, skylevel) + 400;
        //}
        //}},2.);
        if (  i > 1 && (chistredskydn == 400 ))
        {
            std::cout << "    WARNING: histogram sky level red not found" << std::endl;
            //std::cout << "    Try increasing the -zerosky values" << std::endl;
            // break;
        }
        if (  i > 1 && chistgreenskydn == 400)
        {
            std::cout << "    WARNING: histogram sky level green not found" << std::endl;
            //std::cout << "    Try increasing the -zerosky values" << std::endl;
            // break;
        }
        if (  i > 1 && (chistblueskydn == 400))
        {
            std::cout << "    WARNING: histogram sky level blue not found" << std::endl;
            //std::cout << "    Try increasing the -zerosky values" << std::endl;
            // break;
        }

        // Condition to stop when the value is within 5 pxiels after the first iteration
        if (pow(chistgreenskydn - skyLG, 2) <= 25 &&
                pow(chistredskydn - skyLR, 2) <= 25 &&
                pow(chistblueskydn - skyLB, 2) <= 25 && i > 1)
            break;

        float chistredskysub1 = (chistredskydn - skyLR) / 65535.;
        float chistgreenskysub1 = (chistgreenskydn - skyLG) / 65535.;
        float chistblueskysub1 = (chistblueskydn - skyLB) / 65535.;

        // normalizatons
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
    cv::InputArray inImageA, cv::OutputArray outImage, const double rootpower)
{
    double x = 1. / rootpower;

    cv::Mat dim;
    // For high root powers use double precision
    if (rootpower > 30.)
    {
        cv::Mat inImage = inImageA.getMat();
        inImage.convertTo(dim, CV_64F);
    }
    else
    {
        dim = inImageA.getMat();
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

void showHist(cv::InputArray im, const char* window)
{
    cv::Mat src = im.getMat();
    std::vector<cv::Mat> bgr_planes;
    cv::split( src, bgr_planes );
    int histSize = 256;
    float range[] = { 0, 1 }; //the upper boundary is exclusive
    const float* histRange = { range };
    bool uniform = true, accumulate = false;
    cv::Mat b_hist, g_hist, r_hist;
    calcHist( &bgr_planes[0], 1, 0, cv::Mat(), b_hist, 1, &histSize, &histRange, uniform, accumulate );
    calcHist( &bgr_planes[1], 1, 0, cv::Mat(), g_hist, 1, &histSize, &histRange, uniform, accumulate );
    calcHist( &bgr_planes[2], 1, 0, cv::Mat(), r_hist, 1, &histSize, &histRange, uniform, accumulate );
    int hist_w = src.cols, hist_h = 400;
    int bin_w = cvRound( (double) hist_w / (double) histSize );
    cv::Mat histImage( hist_h, hist_w, CV_32FC3, cv::Scalar( 0, 0, 0) );
    cv::normalize(b_hist, b_hist, 0, histImage.rows, cv::NORM_MINMAX, -1, cv::Mat() );
    cv::normalize(g_hist, g_hist, 0, histImage.rows, cv::NORM_MINMAX, -1, cv::Mat() );
    cv::normalize(r_hist, r_hist, 0, histImage.rows, cv::NORM_MINMAX, -1, cv::Mat() );
    for( int i = 1; i < histSize; i++ )
    {
        line( histImage, cv::Point( bin_w * (i - 1), hist_h - cvRound(b_hist.at<float>(i - 1)) ),
              cv::Point( bin_w * (i), hist_h - cvRound(b_hist.at<float>(i)) ),
              cv::Scalar( 1, 0, 0), 2, 8, 0  );
        line( histImage, cv::Point( bin_w * (i - 1), hist_h - cvRound(g_hist.at<float>(i - 1)) ),
              cv::Point( bin_w * (i), hist_h - cvRound(g_hist.at<float>(i)) ),
              cv::Scalar( 0, 1, 0), 2, 8, 0  );
        line( histImage, cv::Point( bin_w * (i - 1), hist_h - cvRound(r_hist.at<float>(i - 1)) ),
              cv::Point( bin_w * (i), hist_h - cvRound(r_hist.at<float>(i)) ),
              cv::Scalar( 0, 0, 1), 2, 8, 0  );
    }
    cv::Mat dst;
    cv::vconcat(im, histImage, dst);
    cv::namedWindow( window, cv::WINDOW_AUTOSIZE | cv::WINDOW_NORMAL);
    cv::imshow(window, dst );
    std::cout << "    Hit a key to continue (with the image window being active)..." << std::endl;
    cv::waitKey(0);
    cv::destroyAllWindows();
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
    const int row_split = r_bg.rows / split;

    ParallelSetMin parallelSetMin(r_bg, g_bg, b_bg, row_split, minr, ming, minb, zx);
    parallel_for_(cv::Range(0, split + 1), parallelSetMin, 8);

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
                   1.0, const bool verbose = false) // possibly merge colorenhance with colorfactor?!?
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

    const float ref_limit = 10. / 65535.;

    cv::setNumThreads(-1);

    const int split = 7;
    const int row_split = r_bg.rows / split;

    cv::Mat r_bg_ref = bgr_planes_ref[2];
    cv::Mat g_bg_ref = bgr_planes_ref[1];
    cv::Mat b_bg_ref = bgr_planes_ref[0];

    ParallelColorCorr parallelColorCorr(r_bg, g_bg, b_bg, r_bg_ref, g_bg_ref, b_bg_ref, cfe, zeroskyred, zeroskygreen,
                                        zeroskyblue, ref_limit, row_split);
    parallel_for_(cv::Range(0, split + 1), parallelColorCorr, 8);

    if(verbose) std::cout << "|" << std::flush;

    std::vector<cv::Mat> channels;
    channels.push_back(b_bg);
    channels.push_back(g_bg);
    channels.push_back(r_bg);

    cv::merge(channels, outImage) ;
    if(verbose) std::cout << std::endl;
}

//auto start = std::chrono::steady_clock::now();
//auto end = std::chrono::steady_clock::now();
//auto diff = end - start;
//std::cout << std::chrono::duration <double, std::milli> (diff).count() << " ms" << std::endl;
