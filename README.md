# SFND 3D Object Tracking

Welcome to the final project of the camera course. By completing all the lessons, you now have a solid understanding of keypoint detectors, descriptors, and methods to match them between successive images. Also, you know how to detect objects in an image using the YOLO deep-learning framework. And finally, you know how to associate regions in a camera image with Lidar points in 3D space. Let's take a look at our program schematic to see what we already have accomplished and what's still missing.

<img src="images/course_code_structure.png" width="779" height="414" />

In this final project, you will implement the missing parts in the schematic. To do this, you will complete four major tasks: 
1. First, you will develop a way to match 3D objects over time by using keypoint correspondences. 
2. Second, you will compute the TTC based on Lidar measurements. 
3. You will then proceed to do the same using the camera, which requires to first associate keypoint matches to regions of interest and then to compute the TTC based on those matches. 
4. And lastly, you will conduct various tests with the framework. Your goal is to identify the most suitable detector/descriptor combination for TTC estimation and also to search for problems that can lead to faulty measurements by the camera or Lidar sensor. In the last course of this Nanodegree, you will learn about the Kalman filter, which is a great way to combine the two independent TTC measurements into an improved version which is much more reliable than a single sensor alone can be. But before we think about such things, let us focus on your final project in the camera course. 

## Dependencies for Running Locally
* cmake >= 2.8
  * All OSes: [click here for installation instructions](https://cmake.org/install/)
* make >= 4.1 (Linux, Mac), 3.81 (Windows)
  * Linux: make is installed by default on most Linux distros
  * Mac: [install Xcode command line tools to get make](https://developer.apple.com/xcode/features/)
  * Windows: [Click here for installation instructions](http://gnuwin32.sourceforge.net/packages/make.htm)
* Git LFS
  * Weight files are handled using [LFS](https://git-lfs.github.com/)
* OpenCV >= 4.1
  * This must be compiled from source using the `-D OPENCV_ENABLE_NONFREE=ON` cmake flag for testing the SIFT and SURF detectors.
  * The OpenCV 4.1.0 source code can be found [here](https://github.com/opencv/opencv/tree/4.1.0)
* gcc/g++ >= 5.4
  * Linux: gcc / g++ is installed by default on most Linux distros
  * Mac: same deal as make - [install Xcode command line tools](https://developer.apple.com/xcode/features/)
  * Windows: recommend using [MinGW](http://www.mingw.org/)

## Basic Build Instructions

1. Clone this repo.
2. Make a build directory in the top level project directory: `mkdir build && cd build`
3. Compile: `cmake .. && make`
4. Run it: `./3D_object_tracking`.

## FP.1 Match 3D Objects

1. Create a embedding `std::map` structure `map<int, map<int, int>> mymap`. Key of outer map is previous frame boxID, key of inner map is current frame boxID and the value of inner map is the count number of keypoint correspondences.
2. Run two loops for the `matches`, first is counting every boxIDs combination of successive frames. Second is filtering the highest number of keypoint correspondences.

## FP.2 Compute Lidar-based TTC

1. With the given lidar points of matched bounding boxes, loop for corresponding to find the nearest point of previous and current box.
2. Using function `isNotOurlier` to check whether the point is a outlier, my solution  is counting all the number of points that the euclidean distance is less than the given radius `radiusSearch`, if the count is less than a threshold `minNeighborsInRadius`, then the point is handle with a outlier.
3. Then just use the formula `minXCurr * (1/frameRate) / (minXPrev - minXCurr)` to calculate the TTC.

## FP.3 Associate Keypoint Correspondences with Bounding Boxes

1. Filled the `clusterKptMatchesWithROI` function in `camFusion_Student.cpp`.
2. Loop for `kptMatches` in the current frame, and add all the matches in the region of interest to `boundingBox.kptMatches`.
3. Next loop the `boundingBox.kptMatches` for removing outliers by compare the distance to the mean multiply by a threshold.

## FP.4 Compute Camera-based TTC

1. First take a loop of handled `boundingBox.kptMatches` in the previous step, and another same embedding loop, to get a distance ratio of current and previous distance of two points, if current distance larger than threshold `minDist`, push it to a vector.
2. To deal with outlier correspondences, sort the vector and find out the median value of the ratios.
3. Finally get TTC by the formula `-dT / (1 - medDistRatio)`.

## FP.5 Performance Evaluation 1

1. By checking all the successive frame, there are several has unreasonable lidar-based TTC, one has sudden drop with only 7.2s(11s - 13s is reasonable), and another sudden rise to 34.34s.
2. My argumentation is if the eago car slow down with a linear acceleration, then the lidar TTC will keep in a stable interval. But in actual situation we slow down by intermittent brake, so if two successive frame with only very small movement, the TTC will sudden rise to a large seconds value, and in contrast the TTC will sudden drop to a fewer seconds.

## FP.6 Performance Evaluation 2


