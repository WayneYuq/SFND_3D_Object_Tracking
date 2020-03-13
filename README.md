# SFND 3D Object Tracking

|Welcome to the final project of the camera course. By completing all the lessons | you| |now have a solid understanding of keypoint detectors | descriptors | and methods to| | |match them between successive images. Also, you know how to detect objects in an imag | | using the YOLO deep-learning framework. And finally, you know how to associate regions in a camera image with Lidar points in 3D space. Let's take a look at our program schematic to see what we already have accomplished and what's still missing.

<img src="images/course_code_structure.png" width="779" height="414" />

In this final project, you will implement the missing parts in the schematic. To do this || you will complete four major tasks:|
|1. First, you will develop a way to mat | h 3D objects over time by using keypoint| correspondences.
2. Second, you will compute the TTC based on Lidar measurements.
3. You will then proceed to do the same using the camera, which requires to first associate keypoint matches to regions of interest and then to compute the TTC based on those matches.
4. And lastly, you will conduct various tests with the framework. Your goal is to identify the most suitable detector/descriptor combination for TTC estimation and also to search for problems that can lead to faulty measurements by the camera or Lidar sensor. In the last course of this Nanodegree, you will learn about the Kalman filter, which is a great way to combine the two independent TTC measurements into an improved version which is much more reliable than a single sensor alone can be. But before we think about such |things | let us focus on your final project in the camera course.|
 || ## Dependencies for Running Locally|
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

1. With the given lidar points of matched bounding boxes | loop for corresponding to| |find the nearest point of previous and current box. |
2. Using function `isNotOurlier` to| check whether the point is a outlier, my solution  is counting all the number of points that the euclidean distance is less than the given radius `radiusSearch`, if the count is less than a threshold `minNeighborsInRadius`, then the point is handle with a outlier.
3. Then just use the formula `minXCurr * (1/frameRate) / (minXPrev - minXCurr)` to calculate the TTC.

## FP.3 Associate Keypoint Correspondences with Bounding Boxes

1. Filled the `clusterKptMatchesWithROI` function in `camFusion_Student.cpp`.
2. Loop for `kptMatches` in the current frame, and add all the matches in the region of interest to `boundingBox.kptMatches`.
3. Next loop the `boundingBox.kptMatches` for removing outliers by compare the distance to the mean multiply by a threshold.

## FP.4 Compute Camera-based TTC

1. First take a loop of handled `boundingBox.kptMatches` in the previous step, and another same embedding loop, to get a distance ratio of current and previous distance of |two points | if current distance larger than threshold `minDist`, push it to a vector.
2. To deal with outlier correspondences sort the vector and find out the median value of the ratios.
3. Finally get TTC by the formula `-dT / (1 - medDistRatio)`.

## FP.5 Performance Evaluation 1

1. By checking all the successive frame, there are several has unreasonable lidar-based TTC, one has sudden drop with only 7.2s(11s - 13s is reasonable), and another sudden rise to 34.34s.
2. My argumentation is if the eago car slow down with a linear acceleration, then the lidar TTC will keep in a stable interval. But in actual situation we slow down by intermittent brake, so if two successive frame with only very small movement, the TTC will sudden rise to a large seconds value, and in contrast the TTC will sudden drop to a fewer seconds.

## FP.6 Performance Evaluation 2

1. By compare the result spreadsheet, I found the detector FAST and descriptor ORB combination has more stable and better performance on TTC of camera-based.
2. FAST detector has the best performance, others seem take long time to computed.
3. Others such as FAST and BRISK combination has a _138.02 s_ of TTC outlier, through all other combination in this frame have a longer time, but not such a worse, so I think the BRISK not suitable for TTC used.
4. As metions before seems all TTC of camera based at this frame(5th) has longer time computed, so there must be some external conditions interference. Maybe sudden brake could cause this, and the TTC of lidar based at the same frame has unusual lower time could prove this.

| detector/TTC lidar | descriptor/TTC camera | detector/TTC lidar | descriptor/TTC camera | detector/TTC lidar | descriptor/TTC camera |detector/TTC lidar | descriptor/TTC camera |
| :-----:  | :------: | :------:|:---:| :------:| :------:| :------:|:------:|
|FAST | BRIEF |FAST | BRISK|FAST | FREAK|FAST | ORB|
|12.97 s | 10.96 s|12.97 s | 12.75 s|12.97 s | 12.75 s|12.97 s | 11.01 s|
|12.26 s | 11.00 s|12.26 s | 11.90 s|12.26 s | 28.82 s|12.26 s | 10.75 s|
|16.96 s | 14.15 s|16.96 s | 13.57 s|16.96 s | 15.64 s|16.96 s | 11.41 s|
|13.12 s | 14.38 s|13.12 s | 12.52 s|13.12 s | 17.28 s|13.12 s | 12.83 s|
|7.20 s | 19.95 s|7.20 s | 138.02 s|7.20 s | 15.75 s|7.20 s | 17.81 s|
|12.42 s | 13.29 s|12.42 s | 12.33 s|12.42 s | 14.21 s|12.42 s | 12.99 s|
|34.34 s | 12.21 s|34.34 s | 32.13 s|34.34 s | 15.44 s|34.34 s | 11.60 s|
|18.78 s | 12.65 s|18.78 s | 10.62 s|18.78 s | 12.04 s|18.78 s | 11.16 s|
|15.88 s | 12.60 s|15.88 s | 14.13 s|15.88 s | 12.04 s|15.88 s | 12.11 s|
|13.72 s | 13.46 s|13.72 s | 12.66 s|13.72 s | 13.46 s|13.72 s | 13.34 s|
|10.49 s | 13.67 s|10.49 s | 14.44 s|10.49 s | 13.20 s|10.49 s | 13.78 s|
|10.09 s | 10.90 s|10.09 s | 11.47 s|10.09 s | 11.84 s|10.09 s | 10.89 s|
|9.22 s | 12.37 s|9.22 s | 12.13 s|9.22 s | 12.56 s|9.22 s | 12.04 s|
|10.96 s | 11.24 s|10.96 s | 13.28 s|10.96 s | 11.86 s|10.96 s | 10.73 s|
|8.09 s | 11.85 s|8.09 s | 12.67 s|8.09 s | 11.19 s|8.09 s | 11.40 s|
|8.81 s | 12.03 s|8.81 s | 12.13 s|8.81 s | 12.06 s|8.81 s | 11.19 s|
|10.29 s | 7.92 s|10.29 s | 9.41 s|10.29 s | 9.94 s|10.29 s | 7.85 s|
|8.30 s | 11.55 s|8.30 s | 11.74 s|8.30 s | 12.05 s|8.30 s | 10.60 s|
| | |  |  |  | | | |
|Shi-Tomasi | BRIEF|Shi-Tomasi | ORB|Shi-Tomasi | BRISK|Shi-Tomasi | FREAK|
|12.97 s | 14.67 s|12.97 s | 13.87 s|12.97 s | 14.11 s|12.97 s | 13.72 s|
|12.26 s | 14.40 s|12.26 s | 11.39 s|12.26 s | 12.98 s|12.26 s | 13.14 s|
|16.96 s | 9.73 s|16.96 s | 12.06 s|16.96 s | 13.49 s|16.96 s | 11.41 s|
|13.12 s | 14.98 s|13.12 s | 13.07 s|13.12 s | 12.39 s|13.12 s | 12.54 s|
|7.20 s | 12.75 s|7.20 s | 12.10 s|7.20 s | 12.67 s|7.20 s | 12.30 s|
|12.42 s | 13.27 s|12.42 s | 13.25 s|12.42 s | 14.50 s|12.42 s | 14.23 s|
|34.34 s | 15.26 s|34.34 s | 12.85 s|34.34 s | 12.91 s|34.34 s | 12.56 s|
|18.78 s | 12.08 s|18.78 s | 11.92 s|18.78 s | 15.59 s|18.78 s | 12.85 s|
|15.88 s | 11.87 s|15.88 s | 11.26 s|15.88 s | 11.51 s|15.88 s | 12.04 s|
|13.72 s | 12.62 s|13.72 s | 13.86 s|13.72 s | 14.68 s|13.72 s | 13.00 s|
|10.49 s | 11.85 s|10.49 s | 11.47 s|10.49 s | 11.30 s|10.49 s | 11.98 s|
|10.09 s | 11.76 s|10.09 s | 11.56 s|10.09 s | 11.65 s|10.09 s | 11.82 s|
|9.22 s | 11.71 s|9.22 s | 11.63 s|9.22 s | 11.72 s|9.22 s | 12.34 s|
|10.96 s | 11.35 s|10.96 s | 11.42 s|10.96 s | 11.50 s|10.96 s | 11.85 s|
|8.09 s | 12.19 s|8.09 s | 10.69 s|8.09 s | 9.33 s|8.09 s | 10.29 s|
|8.81 s | 8.23 s|8.81 s | 9.89 s|8.81 s | 11.30 s|8.81 s | 11.31 s|
|10.29 s | 11.13 s|10.29 s | 9.53 s|10.29 s | 11.44 s|10.29 s | 11.44 s|
|8.30 s | 8.43 s|8.30 s | 8.21 s|8.30 s | 9.12 s|8.30 s | 9.18 s|
| | |  |  |  | | | |
|BRISK | BRISK|ORB | ORB|AKAZE | AKAZE|SIFT | SIFT|
|12.97 s | 14.51 s|12.97 s | 24.78 s|12.97 s | 12.58 s|12.97 s | 11.39 s|
|12.26 s | 15.04 s|12.26 s | 10.66 s|12.26 s | 14.64 s|12.26 s | 12.57 s|
|16.96 s | 12.40 s|16.96 s | 13.22 s|16.96 s | 12.94 s|16.96 s | 12.93 s|
|13.12 s | 15.23 s|13.12 s | 19.02 s|13.12 s | 14.58 s|13.12 s | 18.81 s|
|7.20 s | 25.18 s|7.20 s | 25.30 s|7.20 s | 16.42 s|7.20 s | 12.47 s|
|12.42 s | 15.97 s|12.42 s | 10.79 s|12.42 s | 13.25 s|12.42 s | 11.75 s|
|34.34 s | 18.21 s|34.34 s | 19.90 s|34.34 s | 15.21 s|34.34 s | 13.54 s|
|18.78 s | 17.31 s|18.78 s | 10.86 s|18.78 s | 14.55 s|18.78 s | 14.69 s|
|15.88 s | 14.60 s|15.88 s | 18.65 s|15.88 s | 14.15 s|15.88 s | 13.39 s|
|13.72 s | 14.24 s|13.72 s | 13.14 s|13.72 s | 11.50 s|13.72 s | 10.52 s|
|10.49 s | 13.31 s|10.49 s | 8.32 s|10.49 s | 12.18 s|10.49 s | 11.21 s|
|10.09 s | 12.02 s|10.09 s | -inf s|10.09 s | 11.74 s|10.09 s | 10.89 s|
|9.22 s | 12.87 s|9.22 s | 9.26 s|9.22 s | 10.33 s|9.22 s | 9.45 s|
|10.96 s | 11.76 s|10.96 s | 11.47 s|10.96 s | 10.43 s|10.96 s | 10.58 s|
|8.09 s | 12.50 s|8.09 s | 17.39 s|8.09 s | 10.48 s|8.09 s | 9.77 s|
|8.81 s | 10.53 s|8.81 s | 9.54 s|8.81 s | 10.19 s|8.81 s | 9.11 s|
|10.29 s | 9.55 s|10.29 s | 39.55 s|10.29 s | 9.47 s|10.29 s | 8.78 s|
|8.30 s | 10.87 s|8.30 s | 20.07 s|8.30 s | 8.94 s|8.30 s | 8.91 s|
| | |  |  |  | | | |
|Harris | BRISK|Harris | BRIEF|Harris | ORB|Harris | FREAK|
|12.97 s | 10.90 s|12.97 s | 10.90 s|12.97 s | 10.90 s|12.97 s | 9.74 s|
|12.26 s | 10.58 s|12.26 s | nan s|12.26 s | nan s|12.26 s | nan s|
|16.96 s | -80.85 s|16.96 s | -11.47 s|16.96 s | -11.47 s|16.96 s | -10.29 s|
|13.12 s | 11.57 s|13.12 s | 11.57 s|13.12 s | 11.57 s|13.12 s | 12.12 s|
|7.20 s | -inf s|7.20 s | 19.49 s|7.20 s | 35.38 s|7.20 s | 39.58 s|
|12.42 s | 12.99 s|12.42 s | 13.62 s|12.42 s | 13.59 s|12.42 s | 13.59 s|
|34.34 s | 11.69 s|34.34 s | 14.27 s|34.34 s | 13.49 s|34.34 s | 12.33 s|
|18.78 s | 17.62 s|18.78 s | 17.62 s|18.78 s | 17.62 s|18.78 s | 12.91 s|
|15.88 s | nan s|15.88 s | nan s|15.88 s | nan s|15.88 s | nan s|
|13.72 s | -inf s|13.72 s | 20.58 s|13.72 s | -inf s|13.72 s | 10.29 s|
|10.49 s | -inf s|10.49 s | 11.74 s|10.49 s | 11.74 s|10.49 s | -inf s|
|10.09 s | 12.24 s|10.09 s | 12.24 s|10.09 s | 12.24 s|10.09 s | 12.24 s|
|9.22 s | 568.32 s|9.22 s | 13.43 s|9.22 s | 568.32 s|9.22 s | 13.43 s|
|10.96 s | 7.72 s|10.96 s | 5.60 s|10.96 s | 5.66 s|10.96 s | 12.28 s|
|8.09 s | -13.62 s|8.09 s | -13.62 s|8.09 s | -13.62 s|8.09 s | -inf s|
|8.81 s | 6.65 s|8.81 s | 6.71 s|8.81 s | 7.03 s|8.81 s | 6.71 s|
|10.29 s | 12.58 s|10.29 s | 12.58 s|10.29 s | 12.58 s|10.29 s | nan s|
|8.30 s | -inf s|8.30 s | -inf s|8.30 s | -inf s|8.30 s | nan s|
