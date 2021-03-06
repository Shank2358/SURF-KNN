// SURF+KNN.cpp: 定义控制台应用程序的入口点。
//

// ImageClassification _SURF_SVM.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>  
#include <fstream>  
#include <sstream>  
#include <math.h>  
#include <time.h>
#include "opencv2/core.hpp"   
#include "opencv2/highgui.hpp"  
#include "opencv2/features2d.hpp"   
#include "opencv2/imgproc.hpp"   
#include"opencv2/xfeatures2d.hpp"  
#include"opencv2/ml.hpp"  

using namespace cv;
using namespace cv::xfeatures2d;
using namespace cv::ml;
using namespace std;

void read_csv(const string& filename, vector<Mat>& images, vector<int>& labels)
{
	char separator = ';';
	std::ifstream file(filename.c_str(), ifstream::in);
	if (!file) {
		string error_message = "No valid input file was given, please check the given filename.";
		CV_Error(CV_StsBadArg, error_message);
	}
	string line, path, classlabel;
	while (getline(file, line)) {
		stringstream liness(line);
		getline(liness, path, separator);
		getline(liness, classlabel);
		if (!path.empty() && !classlabel.empty()) {
			images.push_back(imread(path, 0));
			labels.push_back(atoi(classlabel.c_str()));
		}
	}
}

int main()
{
	//time
	clock_t start, end;
	/*
	struct time {
	double time_loadtrain;
	double time_loadtest;
	double time_fextraction;
	double time_knntrain;
	double time_test;
	};
	*/

	//load datasets (images & labels)
	cout << "***** Load Images & Labels *****" << endl;
	//train datasets
	cout << "[Load training datasets]" << endl;
	vector<Mat> images_train;
	vector<int> labels_train;
	string fn_csv_train = "Data Set\\dataset1\\train\\at.txt";
	try
	{
		start = clock();
		read_csv(fn_csv_train, images_train, labels_train);
		end = clock();
	}
	catch (cv::Exception& e)
	{
		cerr << "Error opening file \"" << fn_csv_train << "\". Reason: " << e.msg << endl;
		exit(1);
	}
	if (images_train.size() <= 1)
	{
		string error_message = "This demo needs at least 2 images to work. Please add more images to your dataset!";
		CV_Error(CV_StsError, error_message);
	}
	cout << "The number of training images: " << images_train.size() << endl;
	double time_loadtrain = double(end - start) / CLOCKS_PER_SEC;
	cout << "The run time of loading training datasets: " << time_loadtrain << " seconds." << endl;
	//test datasets
	cout << "[Load testing datasets]" << endl;
	vector<Mat> images_test;
	vector<int> labels_test;
	string fn_csv_test = "Data Set\\dataset1\\test\\at.txt";
	try
	{
		start = clock();
		read_csv(fn_csv_test, images_test, labels_test);
		end = clock();
	}
	catch (cv::Exception& e)
	{
		cerr << "Error opening file \"" << fn_csv_test << "\". Reason: " << e.msg << endl;
		exit(1);
	}
	if (images_test.size() < 1) {
		string error_message = "This demo needs at least 1 images to work. Please add more images to your dataset!";
		CV_Error(CV_StsError, error_message);
	}
	cout << "The number of testing images: " << images_test.size() << endl;
	double time_loadtest;
	time_loadtest = double(end - start) / CLOCKS_PER_SEC;
	cout << "The run time of loading testing datasets: " << time_loadtest << " seconds." << endl;

	//Extract Feature
	cout << endl;
	cout << "***** Extract Image Feature *****" << endl;
	int minHessian = 2000;
	int clusterNum = 100;
	start = clock();
	Mat descriptors;
	Ptr<SURF> detector = SURF::create(minHessian);
	Ptr<SURF> extractor = SURF::create();
	for (int i = 0; i < images_train.size(); i++)
	{
		vector<KeyPoint> keypoints;
		detector->detect(images_train.at(i), keypoints);
		Mat descrip;
		extractor->compute(images_train.at(i), keypoints, descrip);
		descriptors.push_back(descrip);
		keypoints.clear();
		detector.empty();
		extractor.empty();
	}

	BOWKMeansTrainer* bowtrainer = new BOWKMeansTrainer(clusterNum);
	bowtrainer->add(descriptors);
	Mat dictionary = bowtrainer->cluster();
	Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce");
	BOWImgDescriptorExtractor* bowDE = new BOWImgDescriptorExtractor(extractor, matcher);
	bowDE->setVocabulary(dictionary);
	Mat bow;
	Mat labels;
	for (int i = 0; i < images_train.size(); i++)
	{
		vector<KeyPoint> kp;
		int lab = labels_train.at(i);
		Mat tem_image = images_train.at(i);
		Mat imageDescriptor;
		detector->detect(tem_image, kp);
		detector.empty();
		bowDE->compute(tem_image, kp, imageDescriptor);
		kp.clear();
		normalize(imageDescriptor, imageDescriptor);
		bow.push_back(imageDescriptor);
		labels.push_back(lab);
	}
	end = clock();
	double time_fextraction = double(end - start) / CLOCKS_PER_SEC;
	cout << "The run time of extracting image feature: " << time_fextraction << " seconds." << endl;

	//KNN training
	cout << endl;
	cout << "***** KNN Training *****" << endl;
	start = clock();

	Ptr<KNearest> knn = KNearest::create();  //创建knn分类器
	knn->setDefaultK(19);    //设定k值
	knn->setIsClassifier(true);
	Ptr<TrainData> trainData = TrainData::create(bow, ROW_SAMPLE, labels);
	knn->train(trainData);
	knn->save("knntrain.xml");
	end = clock();
	double time_knntrain = double(end - start) / CLOCKS_PER_SEC;
	cout << "The run time of KNN training: " << time_knntrain << " seconds." << endl;

	//Testing
	cout << endl;
	cout << "***** Testing *****" << endl;
	vector<int> result;
	start = clock();
	for (int i = 0; i < images_test.size(); i++)
	{
		vector<KeyPoint>kp;
		Mat test;
		detector->detect(images_test.at(i), kp);
		detector.empty();
		bowDE->compute(images_test.at(i), kp, test);
		kp.clear();
		normalize(test, test);
		result.push_back(knn->predict(test));
	}
	end = clock();
	double time_test = double(end - start) / CLOCKS_PER_SEC;
	cout << "The run time of testing: " << time_test << " seconds." << endl;
	double correct = 0;
	double wrong = 0;
	for (int i = 0; i < result.size(); i++)
	{
		if (result.at(i) == labels_test.at(i))
		{
			correct++;
		}
		else
		{
			wrong++;
		}
	}
	cout << "The number of correct classifications: " << correct << endl;
	cout << "The number of wrong classifications: " << wrong << endl;
	double proportion = correct / (correct + wrong);
	cout << "The proportion of correct classifications: " << proportion * 100 << "%" << endl;

	/*
	time rumtime=
	{
	time_loadtrain,
	time_loadtest,
	time_fextraction,
	time_knntrain,
	time_test,
	};
	*/

	return 0;
}

