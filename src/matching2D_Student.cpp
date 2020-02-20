#include <numeric>
#include <stdexcept>
#include "matching2D.hpp"

using namespace std;

/**
 * Find the match for keypoints in two camera images using the descriptors.
 * 
 * @param kPtsSource <std::vector<cv::KeyPoint>> Source keypoints.
 * @param kPtsRef <std::vector<cv::KeyPoint>> Reference keypoints.
 * @param descSource <cv::Mat> Descriptor source.
 * @param descRef <cv::Mat> Descriptor reference.
 * @param matches <std::vector<cv::DMatch>> Matches.
 * @param descriptorTypeCategory <std::string> Category of the descriptor (either DES_HOG or DES_BINARY).
 * @param matcherType <std::string> Type of the matcher (MAT_BF or MAT_FLANN).
 * @param selectorType <std::string> Type of the selector (SEL_NN or SEL_KNN).
 */
void matchDescriptors(
    std::vector<cv::KeyPoint> &kPtsSource, 
    std::vector<cv::KeyPoint> &kPtsRef, 
    cv::Mat &descSource, 
    cv::Mat &descRef,
    std::vector<cv::DMatch> &matches, 
    std::string descriptorTypeCategory, 
    std::string matcherType, 
    std::string selectorType
)
{
    // configure matcher
    bool crossCheck = true;
    cv::Ptr<cv::DescriptorMatcher> matcher;

    // Brute-Force matching.
    if (matcherType.compare("MAT_BF") == 0)
    {
        int normType;

        // If it is a HOG type (e.g. SIFT) use Euclidean distance.
        if (descriptorTypeCategory.compare("DES_HOG") == 0)
        {
            normType = cv::NORM_L2;
        }
        else if (descriptorTypeCategory.compare("DES_BINARY") == 0)
        {
            // For binary descriptors use Hamming distance.
            normType = cv::NORM_HAMMING;
        }
        else
        {
            throw std::runtime_error("Matcher type not known!");
        }

        matcher = cv::BFMatcher::create(normType, crossCheck);
    }
    else if (matcherType.compare("MAT_FLANN") == 0)
    {
        // Flann matching.
        // HOG based descriptor.
        if (descriptorTypeCategory.compare("DES_HOG") == 0)
        {
            matcher = cv::FlannBasedMatcher::create();
        }
        else if (descriptorTypeCategory.compare("DES_BINARY") == 0)
        {
            const int table_number = 12;
            const int key_size = 20;
            const int multi_probe_level = 2;

            const cv::Ptr<cv::flann::IndexParams> parameters = cv::makePtr<cv::flann::LshIndexParams>(table_number, key_size, multi_probe_level);

            matcher = cv::makePtr<cv::FlannBasedMatcher>(parameters);
        }
        else
        {
            throw std::runtime_error("Flann::Descriptor type not known!");
        }
    }

    // perform matching task
    if (selectorType.compare("SEL_NN") == 0)
    { 
        // nearest neighbor (best match)
        matcher->match(descSource, descRef, matches); // Finds the best match for each descriptor in desc1
    }
    else if (selectorType.compare("SEL_KNN") == 0)
    { 
        // k nearest neighbors (k=2)
        const int k = 2;
        std::vector<std::vector<cv::DMatch>> tmp_matches;
        matcher->knnMatch(descSource, descRef, tmp_matches, k);

        // Descriptor distance ratio test to compare the two best matches.
        const double dist_ratio_threshold = 0.8;

        for (auto match : tmp_matches)
        {
            // At least two matches needed for comparison.
            if (match.size() < 2)
            {
                continue;
            }

            // Ratio test: d1/d2 > threshold
            if (match[1].distance != 0 && (match[0].distance / match[1].distance ) > dist_ratio_threshold)
            {
                // Add to matches.
                matches.push_back(match[0]);
            }
        }
    }
}

// Use one of several types of state-of-art descriptors to uniquely identify keypoints
// Possible descriptors: 
void descKeypoints(vector<cv::KeyPoint> &keypoints, cv::Mat &img, cv::Mat &descriptors, string descriptorType, double& time)
{
    // select appropriate descriptor
    cv::Ptr<cv::DescriptorExtractor> extractor;
    if (descriptorType.compare("BRISK") == 0)
    {
        int threshold = 30;        // FAST/AGAST detection threshold score.
        int octaves = 3;           // detection octaves (use 0 to do single scale)
        float patternScale = 1.0f; // apply this scale to the pattern used for sampling the neighbourhood of a keypoint.

        extractor = cv::BRISK::create(threshold, octaves, patternScale);
    }
    else if (descriptorType.compare("BRIEF") == 0)
    {
        const int bytes = 32;
        const bool use_orientation = false;

        extractor = cv::xfeatures2d::BriefDescriptorExtractor::create(bytes, use_orientation);
    }
    else if (descriptorType.compare("ORB") == 0)
    {
        const int features = 500;
        const float scale_factor = 1.2f;
        const int nlevels = 8;
        const int edge_threshold = 31;
        const int first_level = 0;
        const int wta_k = 2;
        const cv::ORB::ScoreType score_type = cv::ORB::HARRIS_SCORE;
        const int patch_size = 31;
        const int fast_threshold = 20;

        extractor = cv::ORB::create(
            features,
            scale_factor,
            nlevels,
            edge_threshold,
            first_level,
            wta_k,
            score_type,
            patch_size, 
            fast_threshold
        );
    }
    else if (descriptorType.compare("FREAK") == 0)
    {
        const bool orientation_normalized = true;
        const bool scale_normalized = true;
        const float pattern_scale = 22.0f;
        const int n_octaves = 4;
        const std::vector<int> selected_pairs = std::vector<int>();

        extractor = cv::xfeatures2d::FREAK::create(
            orientation_normalized,
            scale_normalized,
            pattern_scale,
            n_octaves,
            selected_pairs
        );
    }
    else if (descriptorType.compare("AKAZE") == 0)
    {
        const cv::AKAZE::DescriptorType descriptor_type = cv::AKAZE::DESCRIPTOR_MLDB;
        const int descriptor_size = 0;
        const int descriptor_channels = 3;
        const float threshold = 0.001f;
        const int n_octaves = 4;
        const int n_octave_layers = 4;
        const cv::KAZE::DiffusivityType diffusivity = cv::KAZE::DIFF_PM_G2;

        extractor = cv::AKAZE::create(
            descriptor_type,
            descriptor_size,
            descriptor_channels,
            threshold,
            n_octaves,
            n_octave_layers,
            diffusivity
        );
    }
    else if (descriptorType.compare("SIFT") == 0)
    {
        const int n_features = 0;
        const int n_octave_layers = 3;
        const double contrast_threshold = 0.04;
        const double edge_threshold = 10;
        const double sigma = 1.6;        

        extractor = cv::xfeatures2d::SIFT::create(
            n_features,
            n_octave_layers,
            contrast_threshold,
            edge_threshold,
            sigma
        );
    }
    else
    {
        throw std::runtime_error("Descriptor " + descriptorType + " now known to this program.");
    }

    double t = (double)cv::getTickCount();
    // perform feature description
    extractor->compute(img, keypoints, descriptors);
    t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();

    // Return result in ms.
    time = t * 1000.0;
}

// Detect keypoints in image using the traditional Shi-Thomasi detector
void detKeypointsShiTomasi(vector<cv::KeyPoint> &keypoints, cv::Mat &img, bool bVis)
{
    // compute detector parameters based on image size
    int blockSize = 4;       //  size of an average block for computing a derivative covariation matrix over each pixel neighborhood
    double maxOverlap = 0.0; // max. permissible overlap between two features in %
    double minDistance = (1.0 - maxOverlap) * blockSize;
    int maxCorners = img.rows * img.cols / max(1.0, minDistance); // max. num. of keypoints

    double qualityLevel = 0.01; // minimal accepted quality of image corners
    double k = 0.04;

    // Apply corner detection
    vector<cv::Point2f> corners;
    cv::goodFeaturesToTrack(img, corners, maxCorners, qualityLevel, minDistance, cv::Mat(), blockSize, false, k);

    // add corners to result vector
    for (auto it = corners.begin(); it != corners.end(); ++it)
    {
        cv::KeyPoint newKeyPoint;
        newKeyPoint.pt = cv::Point2f((*it).x, (*it).y);
        newKeyPoint.size = blockSize;
        keypoints.push_back(newKeyPoint);
    }

    // visualize results
    if (bVis)
    {
        cv::Mat visImage = img.clone();
        cv::drawKeypoints(img, keypoints, visImage, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
        string windowName = "Shi-Tomasi Corner Detector Results";
        cv::namedWindow(windowName, 6);
        imshow(windowName, visImage);
        cv::waitKey(0);
    }
}

void detKeypointsHarris(std::vector<cv::KeyPoint> &keypoints, cv::Mat &img, bool bVis)
{
    // Detector parameters.
    const int block_size = 2;         // For every pixel, a block_size x block_size neighbourhood is considered.
    const int aperture_size = 3;      // Apperture parameter for Sobel operator (must be odd).
    const int min_response = 100;     // Minimum value for a corner in the 8bit scaled response matrix.
    double k = 0.04;                  // Harris parameter.
    const double max_overlap = 0.0;   // Maximal permissible overlab between two features in %.

    // Detect Harris corners and normalize output.
    cv::Mat dst = cv::Mat::zeros(img.size(), CV_32FC1);
    cv::Mat dst_norm;
    cv::Mat dst_norm_scaled;

    cv::cornerHarris(img, dst, block_size, aperture_size, k, cv::BORDER_DEFAULT);
    cv::normalize(dst, dst_norm, 0, 255, cv::NORM_MINMAX, CV_32FC1, cv::Mat());
    cv::convertScaleAbs(dst_norm, dst_norm_scaled);

    for (int j = 0; j < dst_norm.rows; ++j)
    {
        for (int i = 0; i < dst_norm.cols; ++i)
        {
            const int response = static_cast<int>(dst_norm.at<float>(j, i));

            // Only store points above the threshold.
            if (response > min_response)
            {
                cv::KeyPoint new_keypoint;
                
                new_keypoint.pt = cv::Point2f(i, j);
                new_keypoint.size = 2 * aperture_size;
                new_keypoint.response = response;

                // Perform NMS (non-maxima suppression) in local neighbourhood around new key point.
                bool overlap = false;

                for (auto it = keypoints.begin(); it != keypoints.end(); ++it)
                {
                    double kpt_overlap = cv::KeyPoint::overlap(new_keypoint, *it);

                    if (kpt_overlap > max_overlap)
                    {
                        overlap = true;
                        
                        if (new_keypoint.response > it->response)
                        {
                            // If overlap is > threshold and response is higher for new kpt.
                            *it = new_keypoint; // replace old keypoint with the new one
                            break; // quit looping over keypoints.
                        }
                    }
                }

                if ( ! overlap)
                {
                    // Only add new keypoint if no overlap has been found in previous NMS.
                    keypoints.push_back(new_keypoint);
                }
            }
        }
    }


    // Visualize results.
    if (bVis)
    {
        cv::Mat visImage = img.clone();
        cv::drawKeypoints(img, keypoints, visImage, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
        std::string windowName = "Harris Corner Detector Results";
        cv::namedWindow(windowName, 6);
        cv::imshow(windowName, visImage);
        cv::waitKey(0);
    }
}

void detKeypointsModern(std::vector<cv::KeyPoint> &keypoints, cv::Mat &img, std::string detectorType, bool bVis)
{
    cv::Ptr<cv::FeatureDetector> detector;

    if (detectorType == "FAST")
    {
        detector = cv::FastFeatureDetector::create();
    }
    else if (detectorType == "BRISK")
    {
        detector = cv::BRISK::create();
    }
    else if (detectorType == "ORB")
    {
        detector = cv::ORB::create();
    }
    else if (detectorType == "AKAZE")
    {
        detector = cv::AKAZE::create();
    }
    else if (detectorType == "SIFT")
    {
        detector = cv::xfeatures2d::SIFT::create();
    }
    else
    {
        throw std::runtime_error("Detector " + detectorType + " now known to this program.");
    }

    detector->detect(img, keypoints);

    // Visualize results.
    if (bVis)
    {
        cv::Mat visImage = img.clone();
        cv::drawKeypoints(img, keypoints, visImage, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
        std::string windowName = detectorType + " Corner Detector Results";
        cv::namedWindow(windowName, 6);
        cv::imshow(windowName, visImage);
        cv::waitKey(0);
    }
}