/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2008-2013, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and / or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#ifndef __OPENCV_SOFTCASCADE_HPP__
#define __OPENCV_SOFTCASCADE_HPP__

#include "opencv2/core/core.hpp"
#include "opencv2/ml/ml.hpp"

namespace cv {

class CV_EXPORTS_W ICFPreprocessor
{
public:
    CV_WRAP ICFPreprocessor();
    CV_WRAP void apply(cv::InputArray _frame, cv::OutputArray _integrals) const;
protected:
    enum {BINS = 10};
};

// Representation of detectors result.
struct CV_EXPORTS Detection
{
    // Default object type.
    enum {PEDESTRIAN = 1};

    // Creates Detection from an object bounding box and confidence.
    // Param b is a bounding box
    // Param c is a confidence that object belongs to class k
    // Param k is an object class
    Detection(const cv::Rect& b, const float c, int k = PEDESTRIAN) : bb(b), confidence(c), kind(k) {}

    cv::Rect bb;
    float confidence;
    int kind;
};

// Create channel integrals for Soft Cascade detector.
class CV_EXPORTS Channels
{
public:
    // constrictor form resizing factor.
    // Param shrinkage is a resizing factor. Resize is applied before the computing integral sum
    Channels(const int shrinkage);

    // Appends specified number of HOG first-order features integrals into given vector.
    // Param gray is an input 1-channel gray image.
    // Param integrals is a vector of integrals. Hog-channels will be appended to it.
    // Param bins is a number of hog-bins
    void appendHogBins(const cv::Mat& gray, std::vector<cv::Mat>& integrals, int bins) const;

    // Converts 3-channel BGR input frame in  Luv and appends each channel to the integrals.
    // Param frame is an input 3-channel BGR colored image.
    // Param integrals is a vector of integrals. Computed from the frame luv-channels will be appended to it.
    void appendLuvBins(const cv::Mat& frame, std::vector<cv::Mat>& integrals) const;

private:
    int shrinkage;
};

class CV_EXPORTS FeaturePool
{
public:

    virtual int size() const = 0;
    virtual float apply(int fi, int si, const Mat& integrals) const = 0;
    virtual void write( cv::FileStorage& fs, int index) const = 0;

    virtual void preprocess(InputArray frame, OutputArray integrals) const = 0;

    virtual ~FeaturePool();
};

class CV_EXPORTS Dataset
{
public:
    typedef enum {POSITIVE = 1, NEGATIVE = 2} SampleType;

    virtual cv::Mat get(SampleType type, int idx) const = 0;
    virtual int available(SampleType type) const = 0;
    virtual ~Dataset();
};

// ========================================================================== //
//             Implementation of soft (stageless) cascaded detector.
// ========================================================================== //
class CV_EXPORTS_W SoftCascadeDetector : public Algorithm
{
public:

    enum { NO_REJECT = 1, DOLLAR = 2, /*PASCAL = 4,*/ DEFAULT = NO_REJECT};

    // An empty cascade will be created.
    // Param minScale is a minimum scale relative to the original size of the image on which cascade will be applied.
    // Param minScale is a maximum scale relative to the original size of the image on which cascade will be applied.
    // Param scales is a number of scales from minScale to maxScale.
    // Param rejCriteria is used for NMS.
    CV_WRAP SoftCascadeDetector(double minScale = 0.4, double maxScale = 5., int scales = 55, int rejCriteria = 1);

    CV_WRAP virtual ~SoftCascadeDetector();

    cv::AlgorithmInfo* info() const;

    // Load soft cascade from FileNode.
    // Param fileNode is a root node for cascade.
    CV_WRAP virtual bool load(const FileNode& fileNode);

    // Load soft cascade config.
    CV_WRAP virtual void read(const FileNode& fileNode);

    // Return the vector of Detection objects.
    // Param image is a frame on which detector will be applied.
    // Param rois is a vector of regions of interest. Only the objects that fall into one of the regions will be returned.
    // Param objects is an output array of Detections
    virtual void detect(InputArray image, InputArray rois, std::vector<Detection>& objects) const;

    // Param rects is an output array of bounding rectangles for detected objects.
    // Param confs is an output array of confidence for detected objects. i-th bounding rectangle corresponds i-th confidence.
    CV_WRAP virtual void detect(InputArray image, InputArray rois, CV_OUT OutputArray rects, CV_OUT OutputArray confs) const;

private:
    void detectNoRoi(const Mat& image, std::vector<Detection>& objects) const;

    struct Fields;
    Fields* fields;

    double minScale;
    double maxScale;

    int   scales;
    int   rejCriteria;
};

// ========================================================================== //
//      Implementation of singe soft (stageless) cascade octave training.
// ========================================================================== //
class CV_EXPORTS SoftCascadeOctave : public cv::Boost
{
public:

    enum
    {
        // Direct backward pruning. (Cha Zhang and Paul Viola)
        DBP = 1,
        // Multiple instance pruning. (Cha Zhang and Paul Viola)
        MIP = 2,
        // Originally proposed by L. Bourdev and J. Brandt
        HEURISTIC = 4
    };

    SoftCascadeOctave(cv::Rect boundingBox, int npositives, int nnegatives, int logScale, int shrinkage);
    virtual bool train(const Dataset* dataset, const FeaturePool* pool, int weaks, int treeDepth);
    virtual void setRejectThresholds(OutputArray thresholds);
    virtual void write( CvFileStorage* fs, string name) const;
    virtual void write( cv::FileStorage &fs, const FeaturePool* pool, InputArray thresholds) const;
    virtual float predict( InputArray _sample, InputArray _votes, bool raw_mode, bool return_sum ) const;
    virtual ~SoftCascadeOctave();
protected:
    virtual bool train( const cv::Mat& trainData, const cv::Mat& responses, const cv::Mat& varIdx=cv::Mat(),
       const cv::Mat& sampleIdx=cv::Mat(), const cv::Mat& varType=cv::Mat(), const cv::Mat& missingDataMask=cv::Mat());

    void processPositives(const Dataset* dataset, const FeaturePool* pool);
    void generateNegatives(const Dataset* dataset, const FeaturePool* pool);

    float predict( const Mat& _sample, const cv::Range range) const;
private:
    void traverse(const CvBoostTree* tree, cv::FileStorage& fs, int& nfeatures, int* used, const double* th) const;
    virtual void initial_weights(double (&p)[2]);

    int logScale;
    cv::Rect boundingBox;

    int npositives;
    int nnegatives;

    int shrinkage;

    Mat integrals;
    Mat responses;

    CvBoostParams params;

    Mat trainData;
};

CV_EXPORTS bool initModule_softcascade(void);

}

#endif