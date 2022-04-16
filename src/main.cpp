#include <stdio.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <main.hpp>

#define MIN_AR 1        // Minimum aspect ratio
#define MAX_AR 6        // Maximum aspect ratio
#define KEEP 5          // Limit the number of license plates
#define RECT_DIFF 2000  // Set the difference between contour and rectangle


int main(int argc, char** argv )
{
	if ( argc != 2 )
	{
		printf("usage: DisplayImage.out <Image_Path>\n");
		return -1;
	}
	cv::Mat image;

	image = cv::imread( argv[1], 1 );
	if ( !image.data )
	{
		printf("No image data \n");
		return -1;
	}

	std::vector<std::vector<cv::Point>> candidates = locateCandidates(image);
	drawCandidates(image, candidates);


	cv::namedWindow("Display Image", cv::WINDOW_AUTOSIZE );
	cv::imshow("Display Image", image);
	cv::waitKey(0);

	return 0;
}

void drawCandidates(cv::Mat &frame, std::vector<std::vector<cv::Point>> &candidates) {
	const int width = frame.cols;
	const int height = frame.rows;
	const float ratio_width = width / (float) 512;    // Aspect ratio may affect the performance, but will be do the job as for now
	const float ratio_height = height / (float) 512;  // Aspect ratio may affect the performance

	// Convert to rectangle and also filter out the non-rectangle-shape.
	std::vector<cv::Rect> rectangles;
	for (std::vector<cv::Point> currentCandidate : candidates) {
		cv::Rect boundingRect = cv::boundingRect(currentCandidate); // Create a rect around our candidate
		float difference = boundingRect.area() - cv::contourArea(currentCandidate); // Get the difference in area between bouding rect and the area of the countour of the candidate
		if (difference < RECT_DIFF) { // If those two areas are similar enough, candidate is probably a rectangle
			rectangles.push_back(boundingRect); // Add it to possible number plates
		}
	}

	// Remove rectangle with wrong aspect ratio.
	rectangles.erase(std::remove_if(rectangles.begin(), rectangles.end(), [](cv::Rect temp) {
				const float aspect_ratio = temp.width / (float) temp.height; // calculate aspect ration
				return aspect_ratio < MIN_AR || aspect_ratio > MAX_AR; // if aspect ratio is outside allowed range, remove it
			}), rectangles.end());


	// Draw the bounding box of the possible numberplate
	for (cv::Rect rectangle : rectangles) {
		cv::Scalar color = cv::Scalar(255, 0, 0); // Blue Green Red, BGR
		cv::rectangle(frame, cv::Point(rectangle.x * ratio_width, rectangle.y * ratio_height), cv::Point((rectangle.x + rectangle.width) * ratio_width, (rectangle.y + rectangle.height) * ratio_height), color, 3, cv::LINE_8, 0);
	}
}

std::vector<std::vector<cv::Point>> locateCandidates(cv::Mat &frame) {
	// Reduce the image dimension to process
	cv::Mat processedFrame = frame;
	cv::resize(frame, processedFrame, cv::Size(512, 512));

	// Must be converted to grayscale
	if (frame.channels() == 3) {
		cv::cvtColor(processedFrame, processedFrame, cv::COLOR_BGR2GRAY);
	}

	// Perform blackhat morphological operation, reveal dark regions on light backgrounds
	cv::Mat blackhatFrame;
	cv::Mat rectangleKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(13, 5)); // Shapes are set 13 pixels wide by 5 pixels tall
	cv::morphologyEx(processedFrame, blackhatFrame, cv::MORPH_BLACKHAT, rectangleKernel);

	// Find license plate based on whiteness property
	cv::Mat lightFrame;
	cv::Mat squareKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
	cv::morphologyEx(processedFrame, lightFrame, cv::MORPH_CLOSE, squareKernel);
	cv::threshold(lightFrame, lightFrame, 0, 255, cv::THRESH_OTSU);

	// Compute Sobel gradient representation from blackhat using 32 float,
	// and then convert it back to normal [0, 255] single channel
	cv::Mat gradX;
	double minVal, maxVal;
	int dx = 1, dy = 0, ddepth = CV_32F, ksize = -1;
	cv::Sobel(blackhatFrame, gradX, ddepth, dx, dy, ksize);
	gradX = cv::abs(gradX);
	cv::minMaxLoc(gradX, &minVal, &maxVal);
	gradX = 255 * ((gradX - minVal) / (maxVal - minVal));
	gradX.convertTo(gradX, CV_8U);

	// Blur the gradient result, and apply closing operation
	cv::GaussianBlur(gradX, gradX, cv::Size(5,5), 0);
	cv::morphologyEx(gradX, gradX, cv::MORPH_CLOSE, rectangleKernel);
	cv::threshold(gradX, gradX, 0, 255, cv::THRESH_OTSU);

	// Erode and dilate
	cv::erode(gradX, gradX, 2);
	cv::dilate(gradX, gradX, 2);

	// Bitwise AND between threshold result and light regions
	cv::bitwise_and(gradX, gradX, lightFrame);
	cv::dilate(gradX, gradX, 2);
	cv::erode(gradX, gradX, 1);

	// Find contours in the thresholded image and sort by size
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(gradX, contours, cv::noArray(), cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	std::sort(contours.begin(), contours.end(), compareContourAreas);
	std::vector<std::vector<cv::Point>> top_contours;
	top_contours.assign(contours.end() - KEEP, contours.end()); // Descending order

	return top_contours;
}

bool compareContourAreas (std::vector<cv::Point>& contour1, std::vector<cv::Point>& contour2) {
	const double i = fabs(contourArea(cv::Mat(contour1)));
	const double j = fabs(contourArea(cv::Mat(contour2)));
	return (i < j);
}
