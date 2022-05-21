#include <stdio.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <main.hpp>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

#define MIN_AR 1        // Minimum aspect ratio
#define MAX_AR 6        // Maximum aspect ratio
#define KEEP 5          // Limit the number of license plates
#define RECT_DIFF 2000  // Set the difference between contour and rectangle

// Macros
void debug_img(const char* name, cv::Mat &img);
void extract_reg(cv::Mat &frame, std::vector<std::vector<cv::Point>> &candidates);

int debug_imgs_cnt = 0;

struct {
	bool debug = false;
} FLAGS;

int run_ocr(cv::Mat input) {
	char *outText;

	tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
	// Initialize tesseract-ocr with English, without specifying tessdata path
	if (api->Init(NULL, "swe")) {
		fprintf(stderr, "Could not initialize tesseract.\n");
		exit(1);
	}

	// Pass image data to tesseract
	api->SetImage((uchar*)input.data, input.size().width, input.size().height, input.channels(), input.step1());

	// Open input image with leptonica library
	// Pix *image = pixRead("debug/13-crop.jpg");
	// api->SetImage(image);
	// Get OCR result
	outText = api->GetUTF8Text();
	printf("OCR output:\n%s", outText);

	// Destroy used object and release memory
	api->End();
	delete api;
	delete [] outText;
	// pixDestroy(&image);

	return 0;
}


int main(int argc, char** argv )
{
	if ( argc < 2 )
	{
		printf("usage: DisplayImage.out <Image_Path>\n");
		return -1;
	}
	if (argc > 2) {
		if (strcmp(argv[2], "--debug")) {
			FLAGS.debug = true;
			std::cout << "Debug mode is on, will output debug files" << std::endl;
		}
	}
	cv::Mat image;

	image = cv::imread( argv[1], 1 );
	if ( !image.data )
	{
		printf("No image data \n");
		return -1;
	}

	std::vector<std::vector<cv::Point>> candidates = locateCandidates(image);
	extract_reg(image, candidates);
	drawCandidates(image, candidates);

/*j
	cv::namedWindow("Display Image", cv::WINDOW_AUTOSIZE );
	cv::imshow("Display Image", image);
	cv::waitKey(0);
	*/

	debug_img("done", image);

	return 0;
}

void extract_reg(cv::Mat &frame, std::vector<std::vector<cv::Point>> &candidates) {
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
	
	std::cout << "Found " << rectangles.size() << " candidates:" << std::endl;
	for (cv::Rect rect : rectangles) {
		cv::Range cols(rect.x * ratio_width, (rect.x + rect.width) * ratio_width);
		cv::Range rows(rect.y * ratio_height, (rect.y + rect.height) * ratio_height);
		cv::Mat crop = frame(rows, cols);
		debug_img("crop", crop);

		std::cout << run_ocr(crop) << std::endl;
	}
	return;

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

	// Print circles
	int i = 0;
	for (std::vector<cv::Point> currentCandidate : candidates) {
		for (cv::Point p : currentCandidate) {
			cv::Point new_p = cv::Point(p.x * ratio_width, p.y * ratio_height);
			cv::Scalar color = cv::Scalar(0, (25*i)&255, (255-25*i)&255);
			cv::circle(frame, new_p, 4, color);
		}
		i++;
	}
}

void debug_img(const char* name, cv::Mat &img) {
	if (FLAGS.debug) {
		/* Create image path */
		std::string 	path  = "debug/";
				path += std::to_string(debug_imgs_cnt);
				path += "-";
				path += name;
				path += ".jpg";
		imwrite(path, img);
		debug_imgs_cnt++;
	}
}

std::vector<std::vector<cv::Point>> locateCandidates(cv::Mat &frame) {
	// Reduce the image dimension to process
	cv::Mat processedFrame = frame;
	cv::resize(frame, processedFrame, cv::Size(512, 512));

	debug_img("start", processedFrame);

	// Must be converted to grayscale
	if (frame.channels() == 3) {
		cv::cvtColor(processedFrame, processedFrame, cv::COLOR_BGR2GRAY);
	}

	debug_img("grayscale", processedFrame);

	// Remove islands, especially the eu country character such as "s" for sweden or "hr" for croatia. Otherwise this will mess with the rectangle
	cv::Mat largerKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(4, 4));
	cv::morphologyEx(processedFrame, processedFrame, cv::MORPH_OPEN, largerKernel);

	debug_img("removed_island", processedFrame);

	// Perform blackhat morphological operation, reveal dark regions on light backgrounds
	cv::Mat blackhatFrame;
	cv::Mat rectangleKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(13, 5)); // Shapes are set 13 pixels wide by 5 pixels tall
	cv::morphologyEx(processedFrame, blackhatFrame, cv::MORPH_BLACKHAT, rectangleKernel);
	
	debug_img("morphological_opt", blackhatFrame);

	// Find license plate based on whiteness property
	cv::Mat lightFrame;
	cv::Mat squareKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
	cv::morphologyEx(processedFrame, lightFrame, cv::MORPH_CLOSE, squareKernel);
	cv::threshold(lightFrame, lightFrame, 0, 255, cv::THRESH_OTSU);

	debug_img("white_regions", lightFrame);

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

	debug_img("sobel", gradX);

	// Blur the gradient result, and apply closing operation
	cv::GaussianBlur(gradX, gradX, cv::Size(5,5), 0);
	debug_img("blur", gradX);
	cv::morphologyEx(gradX, gradX, cv::MORPH_CLOSE, rectangleKernel);
	debug_img("morph", gradX);
	cv::threshold(gradX, gradX, 0, 255, cv::THRESH_OTSU);

	debug_img("thres", gradX);

	// Erode and dilate
	cv::erode(gradX, gradX, 2);
	cv::dilate(gradX, gradX, 2);

	debug_img("erode_dilate", gradX);

	// Bitwise AND between threshold result and light regions
	cv::bitwise_and(gradX, gradX, lightFrame);
	cv::dilate(gradX, gradX, 2);
	cv::erode(gradX, gradX, 1);

	debug_img("bitwise_AND", gradX);

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
