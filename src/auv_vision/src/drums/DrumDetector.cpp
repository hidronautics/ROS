#include <cmath>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <vector>

/**
#if CUDA_ENABLED == 0
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudaimgproc.hpp>
#endif
*/

//#include "../../include/mat/MatDetector.h"
//#include "../../include/mat/MatDescriptor.h"
#include "../../include/util/ImgprocPipeline.h"
#include "../../include/util/ImgprocUtil.h"
#include "../../include/drums/DrumDetector.h"
#include "../../include/drums/DrumDescriptor.h"
#include "../../include/util/ColorPicker.h"

void DrumDetector::defaultPreprocess(const cv::Mat &src, cv::Mat &dst) {
    // TODO Define which pre-processing methods should be in common scope
    dst = createPipeline(src, false)
            //.apply(std::bind(&DrumDetector::meanShift, this, std::placeholders::_1, std::placeholders::_2), "MeanShift")
            .apply(std::bind(&DrumDetector::extractValueChannel, this, std::placeholders::_1, std::placeholders::_2), "Value channel")
            .apply(std::bind(&DrumDetector::morphology, this, std::placeholders::_1, std::placeholders::_2), "Rid of noise")
            .getImage();
}

void DrumDetector::meanShift(const cv::Mat &src, cv::Mat &dst) {

    // TODO Fix problems with CUDA dependencies
    cv::pyrMeanShiftFiltering(src, dst, 30, 5);
/**
#if CUDA_ENABLED == 0
    cv::pyrMeanShiftFiltering(src, dst, 30, 5);
#else
    cv::cuda::GpuMat gpuSrc, gpuDest;
    gpuSrc.upload(src);
    cv::cuda::cvtColor(gpuSrc, gpuSrc, CV_BGR2BGRA);
    cv::cuda::meanShiftFiltering(gpuSrc, gpuDest, 30, 5);
    gpuDest.download(dst);
#endif
*/
}

void DrumDetector::extractValueChannel(const cv::Mat &src, cv::Mat &dst) {
    cv::Mat hsv;
    cv::cvtColor(src, hsv, CV_BGR2HSV);
    std::vector<cv::Mat> hsvChannels;
    cv::split(hsv, hsvChannels);
    dst = hsvChannels[2]; // Image hue represented as grayscale
}

void DrumDetector::morphology(const cv::Mat &src, cv::Mat &dst) {

    /// Try to fill
    int size = 2;
    cv::Mat element;
    element = cv::getStructuringElement(CV_SHAPE_RECT, cv::Size(2*size+1, 2*size+1));
    cv::morphologyEx(src, dst, cv::MORPH_CLOSE, element);

    if (!dst.empty()) cv::imshow("After morphologyEx", dst);

}

std::vector<cv::Vec3f> DrumDetector::findCircles(const cv::Mat& src, cv::Mat& dst) {
    std::vector<cv::Vec3f> circles;

    /// https://www.tydac.ch/color/
    /// In HSV space, the red color wraps around 180
    cv::Scalar lower_red_1(0, 0, 0); /// Mean - var for low
    cv::Scalar higher_red_1(40, 255, 255); /// Mean + var for high

    cv::Scalar lower_red_2(110, 0, 0); /// Mean - var for low
    cv::Scalar higher_red_2(180, 255, 255); /// Mean + var for high

    cv::Scalar lower_blue(80, 160, 0); /// Mean - var for low
    cv::Scalar higher_blue(180, 255, 255); /// Mean + var for high


    /// CLAHE stuff
    /*
    cv::Mat lab_image;
    cv::cvtColor(image_copy, lab_image, CV_BGR2Lab);

    /// Extract the L channel
    std::vector<cv::Mat> lab_planes(3);
    cv::split(lab_image, lab_planes);  // Now we have the L image in lab_planes[0]

    /// Apply the CLAHE algorithm to the L channel
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    clahe->setClipLimit(4);
    clahe->apply(lab_planes[0], dst);

    /// Merge the the color planes back into an Lab image
    dst.copyTo(lab_planes[0]);
    cv::merge(lab_planes, lab_image);

    /// Convert back to RGB
    cv::Mat image_clahe;
    cv::cvtColor(lab_image, image_clahe, CV_Lab2BGR);
    */

    cv::Mat hsv;

    /// Convert from BGR to HSV colorspace
    cv::cvtColor(src, hsv, CV_BGR2HSV);

    /// Detect the object based on HSV Range Values
    cv::Mat1b mask1, mask2, mask3;
    cv::Mat src_copy;

    cv::inRange(hsv, lower_red_1, higher_red_1, mask1);
    cv::inRange(hsv, lower_red_2, higher_red_2, mask2);
    cv::inRange(hsv, lower_blue, higher_blue, mask3);

    cv::Mat1b mask = mask1 | mask2 | mask3;

    /// Morphology
    int dilation_type = cv::MORPH_ELLIPSE;
    int dilation_size = 4;
    cv::Mat element = cv::getStructuringElement(dilation_type,
                                             cv::Size(2*dilation_size + 1, 2*dilation_size+1),
                                             cv::Point(dilation_size, dilation_size));
    cv::dilate(mask, mask, element);

    int size = 3;
    element = cv::getStructuringElement(CV_SHAPE_RECT, cv::Size(2*size+1, 2*size+1));
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, element);

    //cv::blur(mask, mask, cv::Size(3,3));
    cv::GaussianBlur(mask, mask, cv::Size(0, 0), 2);
    cv::imshow("Mask", mask);


    cv::bitwise_and(hsv, hsv, src_copy, mask = mask);
    cv::blur(src_copy, src_copy, cv::Size(3,3)); /// src_copy - image after applying mask
    cv::addWeighted(hsv, 1, src_copy, 1, 0, src_copy); /// Sum to increase Drum color intensity


    cv::namedWindow("After color enhancement");
    cv::cvtColor(src_copy, src_copy, CV_HSV2BGR);
    cv::imshow("After color enhancement", src_copy);

    cv::Mat gray;

    cv::cvtColor(src_copy, gray, CV_BGR2GRAY);

    /// Apply the Hough Transform to find the circles
    cv::HoughCircles(gray, circles, CV_HOUGH_GRADIENT, 1, gray.rows/2, 100, 60, 0, 0);

    src_copy = src.clone();

    /// Draw the circles detected
    for(size_t i = 0; i < circles.size(); i++) {
        cv::Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
        int radius = cvRound(circles[i][2]);
        cv::circle(src_copy, center, 3, cv::Scalar(0,255,255), -1);
        cv::circle(src_copy, center, radius, cv::Scalar(0,0,255), 1);
    }

    cv::imshow("Circles", src_copy);

    return circles;
}


cv::Mat red_color_pick_1;
cv::Mat red_color_pick_2;

/// Mouse callback function (returns color of the place where you clicked)
void pick_red_Drum_color(int event, int x, int y, int f, void *)
{
    if (event==1)
    {
        int r = 3;
        int off[9*2] = {0,0, -r,-r, -r,0, -r,r, 0,r, r,r, r,0, r,-r, 0,-r};
        for (int i=0; i < 9; i++)
        {
            cv::Vec3b p = red_color_pick_1.at<cv::Vec3b>(y+off[2*i], x+off[2*i+1]);
            std::cerr << int(p[0]) << " " << int(p[1]) << " " << int(p[2]) << std::endl;
            red_color_pick_2.push_back(p);
        }
    }
}

cv::Mat blue_color_pick_1;
cv::Mat blue_color_pick_2;

/// Mouse callback function (returns color of the place where you clicked)
void pick_blue_Drum_color(int event, int x, int y, int f, void *)
{
    if (event==1)
    {
        int r = 3;
        int off[9*2] = {0,0, -r,-r, -r,0, -r,r, 0,r, r,r, r,0, r,-r, 0,-r};
        for (int i=0; i < 9; i++)
        {
            cv::Vec3b p = blue_color_pick_1.at<cv::Vec3b>(y+off[2*i], x+off[2*i+1]);
            std::cerr << int(p[0]) << " " << int(p[1]) << " " << int(p[2]) << std::endl;
            blue_color_pick_2.push_back(p);
        }
    }
}


bool DrumDetector::isRed(const cv::Mat& src, const cv::Vec3f circle) {

    cv::Mat hsv;

    /// Convert from BGR to HSV colorspace
    cv::cvtColor(src, hsv, CV_BGR2HSV);

    /// For by mouse color detection
    /*
    red_color_pick_1 = hsv;

    cv::namedWindow("Pick Red");
    if (!src.empty()) cv::imshow("Pick Red", src);

    cv::setMouseCallback("Pick Red", pick_red_Drum_color, 0);

    cv::Scalar m, v;
    cv::meanStdDev(red_color_pick_2, m, v);

    std::cerr << "mean, var (RED): " << std::endl;
    std::cerr << m[0] << " " << m[1] << " " << m[2] << " " << v[0] << " " << v[1] << " " << v[2] << std::endl; /// Average colors of picked pixel

    cv::Scalar lower_red(m[0]-v[0], m[1]-v[1], m[2]-v[2]); // Mean - var for low
    cv::Scalar higher_red(m[0]+v[0], m[1]+v[1], m[2]+v[2]); // Mean + var for high

    */

    //cv::Scalar lower_red(33, 40, 107); /// Mean - var for low
    //cv::Scalar higher_red(147, 219, 210); /// Mean + var for high

    /// In HSV space, the red color wraps around 180
    cv::Scalar lower_red_1(0, 0, 0); /// Mean - var for low
    cv::Scalar higher_red_1(60, 255, 255); /// Mean + var for high

    cv::Scalar lower_red_2(110, 0, 0); /// Mean - var for low
    cv::Scalar higher_red_2(180, 255, 255); /// Mean + var for high

    /**
    mean, var (RED):
    90.1111 160.963 159.333 57.1666 59.4082 51.9679
    */

    /// Detect the object based on HSV Range Values
    cv::Mat1b mask1, mask2;
    cv::Mat src_copy;

    cv::inRange(hsv, lower_red_1, higher_red_1, mask1);
    cv::inRange(hsv, lower_red_2, higher_red_2, mask2);

    cv::Mat1b mask = mask1 | mask2;
    cv::bitwise_and(hsv, hsv, src_copy, mask = mask);

    src_copy = mask;
    //cv::namedWindow("Red Color Mask");
    //if (!src_copy.empty()) cv::imshow("Red Color Mask", src_copy);

    /// Draw the circle
    //cv::circle(mask, cv::Point(circle[0], circle[1]), circle[2], cv::Scalar(255, 0, 0), -1, 8, 0);

    mask.setTo(cv::Scalar(0, 0, 0)); /// Creates black-Image
    //mask = cv::Mat::zeros(hsv.size(), hsv.type()); /// It is black boy

    cv::Mat maskedImage; /// Stores masked Image

    /// Add circle to mask
    cv::circle(mask, cv::Point(circle[0], circle[1]), circle[2], cv::Scalar(255,255,255), -1, 8, 0);    /// Circle(img, center, radius, color, thickness=1, lineType=8, shift=0)

    src.copyTo(maskedImage, mask); // creates masked Image and copies it to maskedImage

    cv::namedWindow("maskedImage_red");
    if (!maskedImage.empty()) cv::imshow("maskedImage_red", maskedImage);

    cv::Scalar m, v;
    cv::meanStdDev(hsv, m, v, mask);

    std::cerr << "mean, var (RED real): " << std::endl;
    std::cerr << m[0] << " " << m[1] << " " << m[2] << " " << v[0] << " " << v[1] << " " << v[2] << std::endl; /// Average colors of masked image

    /// Check color
    if (((lower_red_1[0] <= m[0]) && (m[0] <= higher_red_1[0]) && (lower_red_1[1] <= m[1]) && (m[1] <= higher_red_1[1]) && (lower_red_1[2] <= m[2]) && (m[2] <= higher_red_1[2])) ||
        ((lower_red_2[0] <= m[0]) && (m[0] <= higher_red_2[0]) && (lower_red_2[1] <= m[1]) && (m[1] <= higher_red_2[1]) && (lower_red_2[2] <= m[2]) && (m[2] <= higher_red_2[2])))
    return true;
    else return false;

}

bool DrumDetector::isBlue(const cv::Mat& src, const cv::Vec3f circle) {

    cv::Mat hsv;

    /// Convert from BGR to HSV colorspace
    cv::cvtColor(src, hsv, CV_BGR2HSV);

    /// For by mouse color detection
    /*
    blue_color_pick_1 = hsv;

    cv::namedWindow("Pick Blue");
    if (!src.empty()) cv::imshow("Pick Blue", src);

    cv::setMouseCallback("Pick Blue", pick_blue_Drum_color, 0);

    cv::Scalar m, v;
    cv::meanStdDev(blue_color_pick_2, m, v);

    std::cerr << "mean, var (BLUE): " << std::endl;
    std::cerr << m[0] << " " << m[1] << " " << m[2] << " " << v[0] << " " << v[1] << " " << v[2] << std::endl; /// Average colors of picked pixel

    cv::Scalar lower_blue(m[0]-v[0], m[1]-v[1], m[2]-v[2]); /// Mean - var for low
    cv::Scalar higher_blue(m[0]+v[0], m[1]+v[1], m[2]+v[2]); /// Mean + var for high
    */

    //cv::Scalar lower_blue(0, 121, 166); /// Mean - var for low
    //cv::Scalar higher_blue(58, 245, 242); /// Mean + var for high

    cv::Scalar lower_blue(80, 160, 0); /// Mean - var for low
    cv::Scalar higher_blue(180, 255, 255); /// Mean + var for high

    /**
    mean, var (BLUE):
    27.2361 183.389 204.056 30.8988 62.4325 38.849
    */

    /// Detect the object based on HSV Range Values
    cv::Mat1b mask;
    cv::Mat src_copy;

    cv::inRange(hsv, lower_blue, higher_blue, mask);
    cv::bitwise_and(hsv, hsv, src_copy, mask = mask);

    src_copy = mask;
    //cv::namedWindow("Blue Color Mask");
    //if (!src_copy.empty()) cv::imshow("Blue Color Mask", src_copy);


    mask.setTo(cv::Scalar(0, 0, 0)); /// Creates black-Image
    //mask = cv::Mat::zeros(hsv.size(), hsv.type()); /// It is black boy

    cv::Mat maskedImage; /// Stores masked Image

    /// Add circle to mask
    cv::circle(mask, cv::Point(circle[0], circle[1]), circle[2], cv::Scalar(255,255,255), -1, 8, 0); /// Circle (img, center, radius, color, thickness=1, lineType=8, shift=0)

    src.copyTo(maskedImage, mask); // Creates masked Image and copies it to maskedImage

    cv::namedWindow("maskedImage_blue");
    if (!maskedImage.empty()) cv::imshow("maskedImage_blue", maskedImage);

    cv::Scalar m, v;
    cv::meanStdDev(hsv, m, v, mask);
    std::cerr << "mean, var (BLUE real): " << std::endl;
    std::cerr << m[0] << " " << m[1] << " " << m[2] << " " << v[0] << " " << v[1] << " " << v[2] << std::endl; /// Average colors of masked image

    /// Check color
    if ((lower_blue[0] <= m[0]) && (m[0] <= higher_blue[0]) && (lower_blue[1] <= m[1]) && (m[1] <= higher_blue[1]) && (lower_blue[2] <= m[2]) && (m[2] <= higher_blue[2]))
        return true;
    else return false;
}

DrumDescriptor DrumDetector::detect(const cv::Mat& src, bool withPreprocess) {
    cv::Mat image;
    if (withPreprocess) {
        /*
        extractValueChannel(src, image); /// Image is noisy
        cv::namedWindow( "extractValueChannel" );
        cv::imshow( "extractValueChannel", image );
        */

        morphology(src, image);
        //defaultPreprocess(src, image);
    }
    else
        image = src.clone();

    if (!src.empty()) {

        std::vector<cv::Vec3f> detectedCircles = findCircles(image, image);

        int counter_red = 0;
        int counter_blue = 0;
        float min_Radius = src.rows/4;

        std::vector<cv::Vec3f> redDrums (detectedCircles.size());
        std::vector<cv::Vec3f> blueDrums (detectedCircles.size());

        if (!detectedCircles.empty()) {
            for (int i = 0; i < detectedCircles.size(); i++) {
                if (detectedCircles[i][2] > min_Radius) {
                    if (isRed(image, detectedCircles[i])) {
                        redDrums [counter_red] = detectedCircles[i];
                        counter_red++;
                    }

                    else if (isBlue(image, detectedCircles[i])) {
                        blueDrums [counter_blue] = detectedCircles[i];
                        counter_blue++;
                    }
                }
            }
        }

        if (counter_red + counter_blue == 0) return DrumDescriptor::noDrums();
        else return DrumDescriptor::create(redDrums, blueDrums);
    } else return DrumDescriptor::noDrums();
}

