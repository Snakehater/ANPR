std::vector<std::vector<cv::Point>> locateCandidates(cv::Mat &colorMat);
bool compareContourAreas (std::vector<cv::Point>& contour1, std::vector<cv::Point>& contour2);
void drawCandidates(cv::Mat &frame, std::vector<std::vector<cv::Point>> &candidates);
