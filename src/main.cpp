#include <stdio.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <main.hpp>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include "file_handler.hpp"

#define MIN_AR 1        // Minimum aspect ratio
#define MAX_AR 6        // Maximum aspect ratio
#define KEEP 5          // Limit the number of license plates
#define RECT_DIFF 2000  // Set the difference between contour and rectangle

int debug_imgs_cnt = 0;

struct {
	bool debug = false;
} FLAGS;

struct {
	int frame_w = 0;
	int frame_h = 0;
	float ratio_w = 0.0f;
	float ratio_h = 0.0f;
} FRAME_METADATA;

struct Match {
	cv::Rect rectangle;
	std::string id;
	bool id_valid;
	bool parking_valid;
};

// Macros
void debug_img(const char* name, cv::Mat &img);
std::vector<Match> extract_ids(tesseract::TessBaseAPI *api, cv::Mat &frame, std::vector<std::vector<cv::Point>> &candidates, std::vector<std::string> &known_cars);
std::vector<std::string> _split(std::string s, std::string delimiter, bool avoid_double);
void drawMatches(cv::Mat &frame, std::vector<Match> &matches, std::vector<std::vector<cv::Point>> &candidates);

/* Code */
void run_ocr(tesseract::TessBaseAPI *api, cv::Mat input, std::string &answer) {
	char *outText;

	// Pass image data to tesseract
	api->SetImage((uchar*)input.data, input.size().width, input.size().height, input.channels(), input.step1());

	// Get OCR result
	outText = api->GetUTF8Text();
	answer = outText;

	// Release memory
	delete [] outText;
}

void anpr(tesseract::TessBaseAPI *api, cv::Mat &image, std::vector<std::string> &known_cars) {
	std::vector<std::vector<cv::Point>> candidates = locateCandidates(image);
	std::vector<Match> matches = extract_ids(api, image, candidates, known_cars);
	drawMatches(image, matches, candidates);
	debug_img("done", image);
}


int main(int argc, char** argv )
{
	/* init tesseract */
	tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
	if (api->Init(NULL, "swe")) {
		fprintf(stderr, "Could not initialize tesseract.\n");
		exit(1);
	}

	if (argc < 2) {
		std::cout << "Please pass video url" << std::endl;
		api->End();
		delete api;
		exit(1);
	}
	if (argc < 3) {
		std::cout << "Please pass list of known cars" << std::endl;
		api->End();
		delete api;
		exit(1);
	}
	if (argc > 3) {
		if (strcmp(argv[3], "--debug")) {
			FLAGS.debug = true;
			std::cout << "Debug mode is on, will output debug files" << std::endl;
		}
	}
	
	/* Read ok parked cars */
	FileHandler fileHandler(argv[2]);
	std::vector<std::string> known_cars = fileHandler.getIDs();

	/* Process video */

	cv::VideoCapture cap(argv[1]);
	cv::Mat frame;

	if (!cap.isOpened()) {
		std::cout << "Cannot open video stream or file" << std::endl;
	} else {
		std::cout << "fps: " << cap.get(cv::CAP_PROP_FPS);
		std::cout << "frame count: " << cap.get(cv::CAP_PROP_FRAME_COUNT);
	}

	/* Read every frame until the end */
	while (cap.isOpened()){
		if (cap.read(frame)) {
			if (frame.empty()) {
				std::cerr << "Error: blank frame grabbed" << std::endl;
				continue;
			}
			anpr(api, frame, known_cars);
			cv::imshow("Frame", frame);
		} else {
			std::cout << "Stream is closed or video camera is disconnected" << std::endl;
			break;
		}

		// wait 20ms or until q is pressed, if q is pressed, break
		if (cv::waitKey(20) == 'q') {
			std::cout << "Sigkill received, exiting now..." << std::endl;
			break;
		}
	}
	cap.release();
	cv::destroyAllWindows();

	api->End();
	delete api;
	return 0;
}

bool valid_chars(std::string &s) {
	for (char c : s) {
		if (!(	(c >= '0' && c <= '9') ||
			(c >= 'A' && c <= 'Z')
		     ))
			return false;
	}
	return true;
}

void parse_answer(std::string answer, std::string &answer_parsed) {
	std::vector<std::string> answer_arr = _split(answer, " ", true);

	/* Sometimes the plate is devided into two parts by a blank space, remove it */
	for (int i = 0; i < answer_arr.size(); i++) {
		answer_arr[i].erase(std::remove(answer_arr[i].begin(), answer_arr[i].end(), ' '), answer_arr[i].end()); //remove A from string
	}
	
	/* In case result has new lines, remove them */
	for (int i = 0; i < answer_arr.size(); i++) {
		answer_arr[i].erase(std::remove(answer_arr[i].begin(), answer_arr[i].end(), '\n'), answer_arr[i].end()); //remove A from string
	}
	
	/* Remove element that doesn't have three characters */
	answer_arr.erase(std::remove_if(answer_arr.begin(), answer_arr.end(), [](std::string &s) {
				if (valid_chars(s))
					return s.length() != 3 && s.length() != 6; // Sometimes the plate is devided into numbers and letters, sometimes they end up together
				else
					return true;
		}), answer_arr.end()
	);

	/* Assembly */
	for (std::string s : answer_arr)
		answer_parsed += s;

	/* Validate final answer */
	if (answer_parsed.length() != 6)
		answer_parsed = ""; // Failsafe: if answer isn't valid, we shall not use it
}

std::vector<struct Match> extract_ids(tesseract::TessBaseAPI *api, cv::Mat &frame, std::vector<std::vector<cv::Point>> &candidates, std::vector<std::string> &known_cars) {
	FRAME_METADATA.frame_w = frame.cols;
	FRAME_METADATA.frame_h = frame.rows;
	FRAME_METADATA.ratio_w = frame.cols / (float) 512; // Aspect ratio may affect the performance, but will be do the job as for now
	FRAME_METADATA.ratio_h = frame.rows / (float) 512; // Aspect ratio may affect the performance


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

	std::string answer;
	std::string answer_parsed;
	std::vector<struct Match> matches;
	for (cv::Rect rect : rectangles) {
		struct Match match;
		answer_parsed = "";
		cv::Range cols(rect.x * FRAME_METADATA.ratio_w, (rect.x + rect.width) * FRAME_METADATA.ratio_w);
		cv::Range rows(rect.y * FRAME_METADATA.ratio_h, (rect.y + rect.height) * FRAME_METADATA.ratio_h);
		cv::Mat crop = frame(rows, cols);
		run_ocr(api, crop, answer);
		parse_answer(answer, answer_parsed);

		match.rectangle = rect;
		match.id = answer_parsed;
		match.id_valid = answer_parsed.length() > 1;
		match.parking_valid = false;
		for (std::string s : known_cars) {
			if (strcmp(s.c_str(), answer_parsed.c_str()) == 0) {
				match.parking_valid = true;
				break;
			}
		}
		matches.push_back(match);

		debug_img("crop", crop);
	}
	return matches;
}

void drawMatches(cv::Mat &frame, std::vector<Match> &matches, std::vector<std::vector<cv::Point>> &candidates) {

	// Draw the bounding box of the possible numberplate
	for (struct Match match : matches) {
		cv::Scalar color;
		if (match.parking_valid) {
			color = cv::Scalar(0, 255, 0); // Blue Green Red, BGR
		} else if (match.id_valid) {
			color = cv::Scalar(0, 0, 255);
		} else {
			color = cv::Scalar(255, 255, 255);
		}
		cv::rectangle(
				frame,
				cv::Point(match.rectangle.x * FRAME_METADATA.ratio_w, match.rectangle.y * FRAME_METADATA.ratio_h),
				cv::Point((match.rectangle.x + match.rectangle.width) * FRAME_METADATA.ratio_w, (match.rectangle.y + match.rectangle.height) * FRAME_METADATA.ratio_h),
				color,
				3,
				cv::LINE_8,
				0
			);
		cv::putText(
				frame,
				match.id,
				cv::Point((match.rectangle.x) * FRAME_METADATA.ratio_w, (match.rectangle.y + match.rectangle.height - 30) * FRAME_METADATA.ratio_h),
				cv::FONT_HERSHEY_DUPLEX,
				1.0f,
				color,
				2
			);
		if (match.id_valid && match.parking_valid) {
			cv::putText(
					frame,
					"OK " + match.id,
					cv::Point((match.rectangle.x) * FRAME_METADATA.ratio_w, (match.rectangle.y + match.rectangle.height) * FRAME_METADATA.ratio_h),
					cv::FONT_HERSHEY_DUPLEX,
					1.0f,
					color,
					2
				);
		}
	}

	// Print circles
	int i = 0; for (std::vector<cv::Point> currentCandidate : candidates) {
		for (cv::Point p : currentCandidate) {
			cv::Point new_p = cv::Point(p.x * FRAME_METADATA.ratio_w, p.y * FRAME_METADATA.ratio_h);
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
	if (contours.size() > KEEP) {
		top_contours.assign(contours.end() - KEEP, contours.end()); // Descending order
	}


	return top_contours;
}

bool compareContourAreas (std::vector<cv::Point>& contour1, std::vector<cv::Point>& contour2) {
	const double i = fabs(contourArea(cv::Mat(contour1)));
	const double j = fabs(contourArea(cv::Mat(contour2)));
	return (i < j);
}

std::vector<std::string> _split(std::string s, std::string delimiter, bool avoid_double){
	std::vector<std::string> output;

	size_t pos = 0;
	std::string token;
	while ((pos = s.find(delimiter)) != std::string::npos) {
		/* If pos is 0 we have a double delimiter and something is probably wrong */
		if (pos == 0 && avoid_double) {
			s.erase(0, 1);
			continue;
		}
		token = s.substr(0, pos);
		output.push_back(token);
		s.erase(0, pos + delimiter.length());
	}
	// add the rest of the string to last element
	output.push_back(s);
	return output;
}

