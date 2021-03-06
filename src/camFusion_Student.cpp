
#include <iostream>
#include <algorithm>
#include <numeric>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "camFusion.hpp"
#include "dataStructures.h"

using namespace std;


// Create groups of Lidar points whose projection into the camera falls into the same bounding box
void clusterLidarWithROI(std::vector<BoundingBox> &boundingBoxes, std::vector<LidarPoint> &lidarPoints, float shrinkFactor, cv::Mat &P_rect_xx, cv::Mat &R_rect_xx, cv::Mat &RT)
{
    // loop over all Lidar points and associate them to a 2D bounding box
    cv::Mat X(4, 1, cv::DataType<double>::type);
    cv::Mat Y(3, 1, cv::DataType<double>::type);

    for (auto it1 = lidarPoints.begin(); it1 != lidarPoints.end(); ++it1)
    {
        // assemble vector for matrix-vector-multiplication
        X.at<double>(0, 0) = it1->x;
        X.at<double>(1, 0) = it1->y;
        X.at<double>(2, 0) = it1->z;
        X.at<double>(3, 0) = 1;

        // project Lidar point into camera
        Y = P_rect_xx * R_rect_xx * RT * X;
        cv::Point pt;
        pt.x = Y.at<double>(0, 0) / Y.at<double>(0, 2); // pixel coordinates
        pt.y = Y.at<double>(1, 0) / Y.at<double>(0, 2);

        vector<vector<BoundingBox>::iterator> enclosingBoxes; // pointers to all bounding boxes which enclose the current Lidar point
        for (vector<BoundingBox>::iterator it2 = boundingBoxes.begin(); it2 != boundingBoxes.end(); ++it2)
        {
            // shrink current bounding box slightly to avoid having too many outlier points around the edges
            cv::Rect smallerBox;
            smallerBox.x = (*it2).roi.x + shrinkFactor * (*it2).roi.width / 2.0;
            smallerBox.y = (*it2).roi.y + shrinkFactor * (*it2).roi.height / 2.0;
            smallerBox.width = (*it2).roi.width * (1 - shrinkFactor);
            smallerBox.height = (*it2).roi.height * (1 - shrinkFactor);

            // check wether point is within current bounding box
            if (smallerBox.contains(pt))
            {
                enclosingBoxes.push_back(it2);
            }

        } // eof loop over all bounding boxes

        // check wether point has been enclosed by one or by multiple boxes
        if (enclosingBoxes.size() == 1)
        { 
            // add Lidar point to bounding box
            enclosingBoxes[0]->lidarPoints.push_back(*it1);
        }

    } // eof loop over all Lidar points
}


void show3DObjects(std::vector<BoundingBox> &boundingBoxes, cv::Size worldSize, cv::Size imageSize, bool bWait)
{
    // create topview image
    cv::Mat topviewImg(imageSize, CV_8UC3, cv::Scalar(255, 255, 255));

    for(auto it1=boundingBoxes.begin(); it1!=boundingBoxes.end(); ++it1)
    {
        // create randomized color for current 3D object
        cv::RNG rng(it1->boxID);
        cv::Scalar currColor = cv::Scalar(rng.uniform(0,150), rng.uniform(0, 150), rng.uniform(0, 150));

        // plot Lidar points into top view image
        int top=1e8, left=1e8, bottom=0.0, right=0.0; 
        float xwmin=1e8, ywmin=1e8, ywmax=-1e8;
        for (auto it2 = it1->lidarPoints.begin(); it2 != it1->lidarPoints.end(); ++it2)
        {
            // world coordinates
            float xw = (*it2).x; // world position in m with x facing forward from sensor
            float yw = (*it2).y; // world position in m with y facing left from sensor
            xwmin = xwmin<xw ? xwmin : xw;
            ywmin = ywmin<yw ? ywmin : yw;
            ywmax = ywmax>yw ? ywmax : yw;

            // top-view coordinates
            int y = (-xw * imageSize.height / worldSize.height) + imageSize.height;
            int x = (-yw * imageSize.width / worldSize.width) + imageSize.width / 2;

            // find enclosing rectangle
            top = top<y ? top : y;
            left = left<x ? left : x;
            bottom = bottom>y ? bottom : y;
            right = right>x ? right : x;

            // draw individual point
            cv::circle(topviewImg, cv::Point(x, y), 4, currColor, -1);
        }

        // draw enclosing rectangle
        cv::rectangle(topviewImg, cv::Point(left, top), cv::Point(right, bottom),cv::Scalar(0,0,0), 2);

        // augment object with some key data
        char str1[200], str2[200];
        sprintf(str1, "id=%d, #pts=%d", it1->boxID, (int)it1->lidarPoints.size());
        putText(topviewImg, str1, cv::Point2f(left-250, bottom+50), cv::FONT_ITALIC, 2, currColor);
        sprintf(str2, "xmin=%2.2f m, yw=%2.2f m", xwmin, ywmax-ywmin);
        putText(topviewImg, str2, cv::Point2f(left-250, bottom+125), cv::FONT_ITALIC, 2, currColor);  
    }

    // plot distance markers
    float lineSpacing = 2.0; // gap between distance markers
    int nMarkers = floor(worldSize.height / lineSpacing);
    for (size_t i = 0; i < nMarkers; ++i)
    {
        int y = (-(i * lineSpacing) * imageSize.height / worldSize.height) + imageSize.height;
        cv::line(topviewImg, cv::Point(0, y), cv::Point(imageSize.width, y), cv::Scalar(255, 0, 0));
    }

    // display image
    string windowName = "3D Objects";
    cv::namedWindow(windowName, 1);
    cv::imshow(windowName, topviewImg);

    if(bWait)
    {
        cv::waitKey(0); // wait for key to be pressed
    }
}


// associate a given bounding box with the keypoints it contains
void clusterKptMatchesWithROI(BoundingBox &boundingBox, 
                              std::vector<cv::KeyPoint> &kptsPrev, 
                              std::vector<cv::KeyPoint> &kptsCurr, 
                              std::vector<cv::DMatch> &kptMatches)
{
    double accumulateDistances;
    double meanThreshold = 2.0;

    for (auto it = kptMatches.begin(); it != kptMatches.end(); ++it)
    {
        cv::KeyPoint currKeyPoint = kptsCurr.at(it->trainIdx);
        cv::KeyPoint prevKeyPoint = kptsPrev.at(it->queryIdx);
        if (boundingBox.roi.contains(currKeyPoint.pt))
        {
            accumulateDistances += cv::norm(currKeyPoint.pt - prevKeyPoint.pt);
            boundingBox.kptMatches.push_back(*it);
        }
    } // eof loop over all matches

    // remove outliers if the distance between corresponding keypoints in successive frames is too large
    double meanEuclideanDistances = accumulateDistances / boundingBox.kptMatches.size();
    for (auto it = boundingBox.kptMatches.begin(); it != boundingBox.kptMatches.end(); ++it)
    {
        cv::KeyPoint currKeyPoint = kptsCurr.at(it->trainIdx);
        cv::KeyPoint prevKeyPoint = kptsPrev.at(it->queryIdx);
        double distance = cv::norm(currKeyPoint.pt - prevKeyPoint.pt);
        if (distance > meanEuclideanDistances * meanThreshold)
        {
            boundingBox.kptMatches.erase(it);
            it--;
        }
    } // eof loop over containing matches in current bounding box
}


// Compute time-to-collision (TTC) based on keypoint correspondences in successive images
void computeTTCCamera(std::vector<cv::KeyPoint> &kptsPrev, std::vector<cv::KeyPoint> &kptsCurr, 
                      std::vector<cv::DMatch> kptMatches, double frameRate, double &TTC, cv::Mat *visImg)
{
    vector<double> distRatios; // stores the distance ratios for all keypoints between curr. and prev. frame
    for (auto it1 = kptMatches.begin(); it1 != kptMatches.end() - 1; ++it1)
    { // outer kpt. loop

        // get current keypoint and its matched partner in the prev. frame
        cv::KeyPoint kpOuterCurr = kptsCurr.at(it1->trainIdx);
        cv::KeyPoint kpOuterPrev = kptsPrev.at(it1->queryIdx);

        for (auto it2 = kptMatches.begin() + 1; it2 != kptMatches.end(); ++it2)
        { // inner kpt.-loop

            double minDist = 100.0; // min. required distance

            // get next keypoint and its matched partner in the prev. frame
            cv::KeyPoint kpInnerCurr = kptsCurr.at(it2->trainIdx);
            cv::KeyPoint kpInnerPrev = kptsPrev.at(it2->queryIdx);

            // compute distances and distance ratios
            double distCurr = cv::norm(kpOuterCurr.pt - kpInnerCurr.pt);
            double distPrev = cv::norm(kpOuterPrev.pt - kpInnerPrev.pt);

            if (distPrev > std::numeric_limits<double>::epsilon() && distCurr >= minDist)
            { // avoid division by zero

                double distRatio = distCurr / distPrev;
                distRatios.push_back(distRatio);
            }
        } // eof inner loop over all matched kpts
    }     // eof outer loop over all matched kpts

    // only continue if list of distance ratios is not empty
    if (distRatios.size() == 0)
    {
        TTC = NAN;
        return;
    }

    // compute camera-based TTC from distance ratios
    // double meanDistRatio = std::accumulate(distRatios.begin(), distRatios.end(), 0.0) / distRatios.size();
    std::sort(distRatios.begin(), distRatios.end());
    long medIndex = floor(distRatios.size() / 2.0);
    // compute median dist. ratio to remove outlier influence
    double medDistRatio = distRatios.size() % 2 == 0 ? (distRatios[medIndex - 1] + distRatios[medIndex]) / 2.0 : distRatios[medIndex];

    double dT = 1 / frameRate;
    TTC = -dT / (1 - medDistRatio);
}

// caculate the nearest point number based on a given radius
bool isNotOurlier(const LidarPoint &lidarPoint, const std::vector<LidarPoint> &lidarPoints, 
                  const double radiusSearch, const int minNeighborsInRadius)
{
    int count = 0;
    for (auto it = lidarPoints.begin(); it != lidarPoints.end(); ++it)
    {
        // the distance between two lidar points
        double dis = sqrt(pow(lidarPoint.x - it->x, 2)+pow(lidarPoint.y - it->y, 2)+pow(lidarPoint.z - it->z, 2));
        if (dis <= radiusSearch) count++;
    }
    if (count <= minNeighborsInRadius) return false;
    return true;
}

void computeTTCLidar(std::vector<LidarPoint> &lidarPointsPrev,
                     std::vector<LidarPoint> &lidarPointsCurr, double frameRate, double &TTC)
{
    double laneWidth = 4.0; // assumed width of the ego lane
    double radiusSearch = 0.1;
    int minNeighborsInRadius = 3;

    // find closest distance to Lidar points within ego lane
    double minXPrev = 1e9, minXCurr = 1e9;
    for (auto it = lidarPointsPrev.begin(); it != lidarPointsPrev.end(); ++it)
    {
        if ((abs(it->y) <= laneWidth / 2.0) && isNotOurlier(*it, lidarPointsPrev, radiusSearch, minNeighborsInRadius)) // 3D point within ego lane?
            minXPrev = minXPrev > it->x ? it->x : minXPrev;
    }

    for (auto it = lidarPointsCurr.begin(); it != lidarPointsCurr.end(); ++it)
    {
        if ((abs(it->y) <= laneWidth / 2.0) && isNotOurlier(*it, lidarPointsCurr, radiusSearch, minNeighborsInRadius))
            minXCurr = minXCurr > it->x ? it->x : minXCurr;
    }

    // compute TTC from both measurements
    TTC = minXCurr * (1/frameRate) / (minXPrev - minXCurr);
}

// associate bounding boxes between current and previous frame using keypoint matches
void matchBoundingBoxes(std::vector<cv::DMatch> &matches, 
                        std::map<int, int> &bbBestMatches, 
                        DataFrame &prevFrame, 
                        DataFrame &currFrame)
{
    // key of outer map is prevFrame boxID, key of inner map is currFrame boxID
    // value of inner map is the count number of keypoint correspondences
    map<int, map<int, int>> mymap;

    for (auto it = matches.begin(); it != matches.end(); ++it)
    {
        cv::KeyPoint prevPoint = prevFrame.keypoints[it->queryIdx];
        cv::KeyPoint currPoint = currFrame.keypoints[it->trainIdx];
        int prevIdx;
        int currIdx;
        for (auto it_pre_bb = prevFrame.boundingBoxes.begin(); it_pre_bb != prevFrame.boundingBoxes.end(); ++it_pre_bb)
        {
            if (it_pre_bb->roi.contains(prevPoint.pt))
            {
                prevIdx = it_pre_bb->boxID;
                break;
            }
        }
        for (auto it_curr_bb = currFrame.boundingBoxes.begin(); it_curr_bb != currFrame.boundingBoxes.end(); ++it_curr_bb)
        {
            if (it_curr_bb->roi.contains(currPoint.pt))
            {
                currIdx = it_curr_bb->boxID;
                break;
            }
        }
        // count the number of associate bounding boxes
        mymap[prevIdx][currIdx]++;
    }
    
    for (auto it_prev = mymap.begin(); it_prev != mymap.end(); ++it_prev)
    {
        int highest_curr_idx = 0;
        for (auto it_curr = it_prev->second.begin(); it_curr != it_prev->second.end(); ++it_curr)
        {
            // find the highest number of keypoint correspondences
            if (highest_curr_idx < it_curr->second)
            {
                highest_curr_idx = it_curr->second;
                bbBestMatches[it_prev->first] = it_curr->first;
            }
        }
    }
}
