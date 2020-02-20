/* INCLUDES FOR THIS PROJECT */
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <limits>
#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>

#include "dataStructures.h"
#include "matching2D.hpp"

#include <deque>

using namespace std;

/* MAIN PROGRAM */
int main(int argc, const char *argv[])
{
    // Input parameters.
    string detectorType = "HARRIS"; // SHITOMASI; HARRIS; FAST; BRISK; ORB; AKAZE; SIFT
    string descriptorType = "BRIEF"; // BRISK; BRIEF, ORB, FREAK, AKAZE, SIFT
    string matcherType = "MAT_BF";        // MAT_BF, MAT_FLANN
    string selectorType = "SEL_NN";       // SEL_NN, SEL_KNN
    bool bVis = true;            // visualize results

    // Try to read the descriptor/detector type from the command line.
    // Visualization.
    if (argc > 1)
    {
        const std::string arg = argv[1];
        bVis = (arg == "true" || arg == "True" || arg == "TRUE");
    }

    // Detector type.
    if (argc > 2)
    {
        detectorType = argv[2];
    }

    // // Descriptor type.
    if (argc > 3)
    {
        descriptorType = argv[3];
    }
    
    // Matcher type.
    if (argc > 4)
    {
        matcherType = argv[4];
    }

    // Selector type.
    if (argc > 5)
    {
        selectorType = argv[5];
    }

     // Display used paramters.
    std::cout << "Using detector: " << detectorType << std::endl;
    std::cout << "Using descriptor: " << descriptorType << std::endl;
    std::cout << "Using matcher: " << matcherType << std::endl;
    std::cout << "Using selector: " << selectorType << std::endl;

    string descriptorTypeCat = descriptorType.compare("SIFT") == 0 ? "DES_HOG" : "DES_BINARY"; // DES_BINARY, DES_HOG
    std::cout << "Using descriptor type: " << descriptorTypeCat << std::endl;


    /* INIT VARIABLES AND DATA STRUCTURES */

    // data location
    string dataPath = "../";

    // camera
    string imgBasePath = dataPath + "images/";
    string imgPrefix = "KITTI/2011_09_26/image_00/data/000000"; // left camera, color
    string imgFileType = ".png";
    int imgStartIndex = 0; // first file index to load (assumes Lidar and camera names have identical naming convention)
    int imgEndIndex = 9;   // last file index to load
    int imgFillWidth = 4;  // no. of digits which make up the file index (e.g. img-0001.png)
    // misc
    constexpr int dataBufferSize = 2;       // no. of images which are held in memory (ring buffer) at the same time
    std::deque<DataFrame> dataBuffer; // Use deque for FIFO ring buffer.

    /* MAIN LOOP OVER ALL IMAGES */

    for (size_t imgIndex = 0; imgIndex <= imgEndIndex - imgStartIndex; imgIndex++)
    {
        size_t pts_total = 0;
        size_t pts_on_vehicle = 0;

        /* LOAD IMAGE INTO BUFFER */

        // assemble filenames for current index
        ostringstream imgNumber;
        imgNumber << setfill('0') << setw(imgFillWidth) << imgStartIndex + imgIndex;
        string imgFullFilename = imgBasePath + imgPrefix + imgNumber.str() + imgFileType;

        // load image from file and convert to grayscale
        cv::Mat img, imgGray;
        img = cv::imread(imgFullFilename);
        cv::cvtColor(img, imgGray, cv::COLOR_BGR2GRAY);

        //// STUDENT ASSIGNMENT
        //// TASK MP.1 -> replace the following code with ring buffer of size dataBufferSize
        // Added the sfnd::RingBuffer data structure, which implements part of the std-container interface.

        // push image into data frame buffer
        DataFrame frame;
        frame.cameraImg = imgGray;
        dataBuffer.push_back(frame);
        if (dataBuffer.size() > dataBufferSize)
        {
            // Remove front element.
            dataBuffer.pop_front();
        }

        //// EOF STUDENT ASSIGNMENT
        // std::cout << "#1 : LOAD IMAGE INTO BUFFER done" << std::endl;

        /* DETECT IMAGE KEYPOINTS */

        // extract 2D keypoints from current image
        vector<cv::KeyPoint> keypoints; // create empty feature list for current image

        //// STUDENT ASSIGNMENT
        //// TASK MP.2 -> add the following keypoint detectors in file matching2D.cpp and enable string-based selection based on detectorType
        //// -> HARRIS, FAST, BRISK, ORB, AKAZE, SIFT

        // Timer for detector speed.
        const double detector_start = static_cast<double>(cv::getTickCount());
        
        if (detectorType.compare("SHITOMASI") == 0)
        {
            detKeypointsShiTomasi(keypoints, imgGray, false);
        }
        else if (detectorType.compare("HARRIS") == 0)
        {
            detKeypointsHarris(keypoints, imgGray, false);
        }
        else
        {
            detKeypointsModern(keypoints, imgGray, detectorType, false);
        }

        // Detector time.
        const double detector_time = (static_cast<double>(cv::getTickCount()) - detector_start) / cv::getTickFrequency() * 1000.0 / 1.0;

        //// EOF STUDENT ASSIGNMENT

        //// STUDENT ASSIGNMENT
        //// TASK MP.3 -> only keep keypoints on the preceding vehicle

        // only keep keypoints on the preceding vehicle
        const bool bFocusOnVehicle = true;
        const cv::Rect vehicleRect(535, 180, 180, 150);

        pts_total = keypoints.size();
        
        if (bFocusOnVehicle)
        {
            std::vector<cv::KeyPoint> contained_points;

            for (auto keypoint : keypoints)
            {
                if (vehicleRect.contains(keypoint.pt))
                {
                    contained_points.push_back(keypoint);
                }
                
            }
            
            // Save the cropped points to keypoints.
            keypoints = contained_points;
            pts_on_vehicle = keypoints.size();
        }

        //// EOF STUDENT ASSIGNMENT

        // optional : limit number of keypoints (helpful for debugging and learning)
        bool bLimitKpts = false;
        if (bLimitKpts)
        {
            int maxKeypoints = 50;

            if (detectorType.compare("SHITOMASI") == 0)
            { // there is no response info, so keep the first 50 as they are sorted in descending quality order
                keypoints.erase(keypoints.begin() + maxKeypoints, keypoints.end());
            }

            cv::KeyPointsFilter::retainBest(keypoints, maxKeypoints);
            // std::cout << " NOTE: Keypoints have been limited!" << std::endl;
        }

        // push keypoints and descriptor for current frame to end of data buffer
        (dataBuffer.end() - 1)->keypoints = keypoints;

        // std::cout << "detected: " << (dataBuffer.end() - 1)->keypoints.size() << " kepyoints" << std::endl;
        // cout << "#2 : DETECT KEYPOINTS done" << endl;

        /* EXTRACT KEYPOINT DESCRIPTORS */

        //// STUDENT ASSIGNMENT
        //// TASK MP.4 -> add the following descriptors in file matching2D.cpp and enable string-based selection based on descriptorType
        //// -> BRIEF, ORB, FREAK, AKAZE, SIFT

        cv::Mat descriptors;

        // Timer for the descriptor.
        // const double descriptor_start = static_cast<double>(cv::getTickCount());
        double descriptor_time = 0.0;

        descKeypoints((dataBuffer.end() - 1)->keypoints, (dataBuffer.end() - 1)->cameraImg, descriptors, descriptorType, descriptor_time);

        // Descriptor time.
        // const double descriptor_time = (static_cast<double>(cv::getTickCount()) - descriptor_start) / cv::getTickFrequency() * 1000.0 / 1.0;

        // push descriptors for current frame to end of data buffer
        (dataBuffer.end() - 1)->descriptors = descriptors;


        // std::cout << detectorType << " detector took " << detector_time << " ms." << std::endl;

        // std::cout << descriptorType << " descriptor took " << descriptor_time << " ms." << std::endl;
        // std::cout << detectorType << " | " << descriptorType << " together took " << detector_time + descriptor_time << " ms" << std::endl;
        //// EOF STUDENT ASSIGNMENT
        

        // std::cout << "#3 : EXTRACT DESCRIPTORS done" << std::endl;

        if (dataBuffer.size() > 1) // wait until at least two images have been processed
        {

            /* MATCH KEYPOINT DESCRIPTORS */
            vector<cv::DMatch> matches;

            //// STUDENT ASSIGNMENT
            //// TASK MP.5 -> add FLANN matching in file matching2D.cpp
            //// TASK MP.6 -> add KNN match selection and perform descriptor distance ratio filtering with t=0.8 in file matching2D.cpp
            matchDescriptors(
                (dataBuffer.end() - 2)->keypoints, 
                (dataBuffer.end() - 1)->keypoints,
                (dataBuffer.end() - 2)->descriptors,
                (dataBuffer.end() - 1)->descriptors,
                matches, 
                descriptorTypeCat, 
                matcherType, 
                selectorType
            );

            //// EOF STUDENT ASSIGNMENT

            // store matches in current data frame
            (dataBuffer.end() - 1)->kptMatches = matches;

            // std::cout << "#4 : MATCH KEYPOINT DESCRIPTORS done" << std::endl;

            // std::cout << "Detector: " << detectorType << " | Descriptor: " << descriptorType << " | Matcher: " << matcherType << " || Matches: " << (dataBuffer.end() - 1)->kptMatches.size() << std::endl;

            // visualize matches between current and previous image
            if (bVis)
            {
                cv::Mat matchImg = ((dataBuffer.end() - 1)->cameraImg).clone();
                cv::drawMatches((dataBuffer.end() - 2)->cameraImg, (dataBuffer.end() - 2)->keypoints,
                                (dataBuffer.end() - 1)->cameraImg, (dataBuffer.end() - 1)->keypoints,
                                matches, matchImg,
                                cv::Scalar::all(-1), cv::Scalar::all(-1),
                                vector<char>(), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

                string windowName = "Matching keypoints between two camera images";
                cv::namedWindow(windowName, 7);
                cv::imshow(windowName, matchImg);
                std::cout << "Press key to continue to next image" << std::endl;
                
                // Only if visualization is on wait for key.
                if (bVis)
                {
                    cv::waitKey(0); // wait for key to be pressed
                }
            }
        }

        // Output the results.
        std::cout << "Detector:" << detectorType 
                    << "|Descriptor:" << descriptorType 
                    << "|Matcher:" << matcherType 
                    << "|Total:" << pts_total 
                    << "|Vehicle:" << pts_on_vehicle 
                    << "|Matches:" << (dataBuffer.end() - 1)->kptMatches.size() 
                    << "|Time Detector[ms]:" << detector_time
                    << "|Time Descriptor[ms]:" << descriptor_time
                    << std::endl;

    } // eof loop over all images

    return 0;
}
