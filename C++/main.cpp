#include "pch.h"
#include <opencv2/opencv.hpp>
#include "main.h"
#include <fstream>

cv::Mat frame, frame1, disparityMap;
int cam;
bool patternIsFound = false;
const int numOfCalibFrames = 40;
bool isCalibFrame = false;
double rms1, rms2, rms;
int CHECKERBOARD[2]{ 6,9 };
std::vector<cv::Point3f> objp;
std::vector<std::vector<cv::Point3f> > objectPoints;
std::vector<std::vector<cv::Point2f> > imagePoints1, imagePoints2;
cv::Mat cameraMatrix1, distCoeffs1, rotationMatrix1, T1, cameraMatrix2, distCoeffs2, rotationMatrix2, T2,
	rotationMatrix, translationVector, essentialMatrix, fundamentalMatrix;
cv::Ptr<cv::StereoBM> bm = cv::StereoBM::create();
cv::Mat map11, map12, map21, map22;

int fillObjp()
{
	for (int k = 0; k < CHECKERBOARD[1]; k++)
	{
		for (int j = 0; j < CHECKERBOARD[0]; j++)
		{
			objp.push_back(cv::Point3f(j, k, 0));
		}
	}
	return 0;
}

int calibrateCameras()
{
	rms1 = cv::calibrateCamera(objectPoints, imagePoints1, cv::Size(frame.rows, frame.cols), cameraMatrix1, distCoeffs1, rotationMatrix1, T1);
	rms2 = cv::calibrateCamera(objectPoints, imagePoints2, cv::Size(frame.rows, frame.cols), cameraMatrix2, distCoeffs2, rotationMatrix2, T2);
	return 0;
}

int saveParams()
{
	std::ofstream fout("D:/ÍÈÐ/UnityProject/New Unity Project/output.txt");
	if (!fout.is_open())
	{
		std::cerr << "Can't open output file!" << std::endl;
		exit(1);
	}
	fout << "rms1 : " << rms1 << std::endl;
	fout << "cameraMatrix1 : " << cameraMatrix1 << std::endl;
	fout << "distCoeffs1 : " << distCoeffs1 << std::endl << std::endl;
	fout << "rms2 : " << rms2 << std::endl;
	fout << "cameraMatrix2 : " << cameraMatrix2 << std::endl;
	fout << "distCoeffs2 : " << distCoeffs2 << std::endl;
	fout << "rms : " << rms << std::endl;
	fout << "rotationMatrix : " << rotationMatrix << std::endl;
	fout << "translationVector : " << translationVector << std::endl;
	fout << "essentialMatrix : " << essentialMatrix << std::endl;
	fout << "fundamentalMatrix : " << fundamentalMatrix << std::endl;
	
	return 0;
}

int setBMParameters(cv::Size imageSize)
{
	bm->setPreFilterSize(5);
	bm->setPreFilterCap(5);
	bm->setBlockSize(19);
	bm->setMinDisparity(0);
	bm->setNumDisparities(48);
	bm->setTextureThreshold(0);
	bm->setUniquenessRatio(0);
	bm->setSpeckleWindowSize(0);
	bm->setSpeckleRange(0);
	bm->setDisp12MaxDiff(20);

	cv::Rect roi1, roi2;
	cv::Mat Q, R1, P1, R2, P2;
	stereoRectify(cameraMatrix1, distCoeffs1, cameraMatrix2, distCoeffs2, imageSize, rotationMatrix, translationVector, R1, R2, P1, P2, Q, /*CALIB_ZERO_DISPARITY*/0, -1, imageSize, &roi1, &roi2);

	initUndistortRectifyMap(cameraMatrix1, distCoeffs1, R1, P1, imageSize, CV_16SC2, map11, map12);
	initUndistortRectifyMap(cameraMatrix2, distCoeffs2, R2, P2, imageSize, CV_16SC2, map21, map22);

	bm->setROI1(roi1);
	bm->setROI2(roi2);

	return 0;
}

int calculateDisparity(const cv::Mat& image1,
	const cv::Mat& image2,
	cv::Mat& image1recified,
	cv::Mat& image2recified,
	cv::Mat& disparityMap)
{
	remap(image1, image1recified, map11, map12, cv::INTER_LINEAR);
	remap(image2, image2recified, map21, map22, cv::INTER_LINEAR);

	cv::Mat image1gray, image2gray;
	cv::cvtColor(image1recified, image1gray, cv::COLOR_BGR2GRAY);
	cv::cvtColor(image2recified, image2gray, cv::COLOR_BGR2GRAY);

	int numberOfDisparities = bm->getNumDisparities();
	cv::Mat disp, disp8bit;
	bm->compute(image1gray, image2gray, disp);

	disp.convertTo(disp8bit, CV_8U, 255 / (numberOfDisparities * 16.));

	cv::cvtColor(disp8bit, disparityMap, cv::COLOR_GRAY2BGRA, 4);

	return 0;
}

int getImages(Color32** raw, int width, int height, int numOfImg, int numOfCam)
{
	frame = cv::Mat(height, width, CV_8UC4, raw);
	cam = numOfCam;
	cvtColor(frame, frame, cv::COLOR_BGRA2BGR);
	flip(frame, frame, 0);
	if (numOfImg < numOfCalibFrames)
	{
		isCalibFrame = true;
	}
	if (numOfImg == 0 && cam == 1)
	{
		fillObjp();
	}
	if (numOfImg == numOfCalibFrames)
	{
		calibrateCameras();

		rms = cv::stereoCalibrate(
			objectPoints, imagePoints1, imagePoints2,

			cameraMatrix1, distCoeffs1,
			cameraMatrix2, distCoeffs2,
			frame.size(),
			rotationMatrix,
			translationVector,
			essentialMatrix,
			fundamentalMatrix,
			cv::CALIB_USE_INTRINSIC_GUESS,
			cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 100, 1e-5)
		);
		saveParams();

		setBMParameters(cv::Size(width, height));
	}
	return 0;
}

void processImage(unsigned char* data, int width, int height)
{
	if (isCalibFrame)
	{
		cv::Mat gray;
		cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
		std::vector<cv::Point2f> corner_pts;
		bool success = cv::findChessboardCorners(gray, cv::Size(CHECKERBOARD[0], CHECKERBOARD[1]), corner_pts, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FAST_CHECK | cv::CALIB_CB_NORMALIZE_IMAGE);
		if (success)
		{
			cv::TermCriteria criteria(cv::TermCriteria::Type::EPS | cv::TermCriteria::Type::MAX_ITER, 30, 0.001);

			cv::cornerSubPix(gray, corner_pts, cv::Size(11, 11), cv::Size(-1, -1), criteria);


			cv::drawChessboardCorners(frame, cv::Size(CHECKERBOARD[0], CHECKERBOARD[1]), corner_pts, success);

			if (cam == 1)
			{
				objectPoints.push_back(objp);
				imagePoints1.push_back(corner_pts);
				patternIsFound = true;
			}
			if (cam == 2 && patternIsFound)
			{
				imagePoints2.push_back(corner_pts);
				patternIsFound = false;
			}
		}
		else if (cam == 2 && patternIsFound)
		{
			objectPoints.pop_back();
			imagePoints1.pop_back();
			patternIsFound = false;
		}
	}
	else if (cam == 1)
	{
		frame1 = frame.clone();
	}
	else if (cam == 2)
	{
		cv::Mat image1recified, image2recified;
		calculateDisparity(frame1, frame, image1recified, image2recified, disparityMap);
		flip(disparityMap, disparityMap, 0);
	}
	cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA);
	flip(frame, frame, 0);
	if (isCalibFrame)
	{
		memcpy(data, frame.data, frame.total() * frame.elemSize());
	}
	else
	{
		memcpy(data, disparityMap.data, disparityMap.total() * disparityMap.elemSize());
	}
	isCalibFrame = false;
}