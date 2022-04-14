#include <stdio.h>
#include <opencv2/opencv.hpp>

int main(int argc, char** argv )
{
    if ( argc != 2 )
    {
        printf("usage: DisplayImage.out <Image_Path>\n");
        return -1;
    }
    cv::Mat colorMat, greyMat;
    colorMat = cv::imread( argv[1], 1 );
    if ( !colorMat.data )
    {
        printf("No image data \n");
        return -1;
    }
	cv::cvtColor(colorMat, greyMat, cv::COLOR_BGR2GRAY);
    cv::namedWindow("Display Image", cv::WINDOW_AUTOSIZE );
    cv::imshow("Display Image", greyMat);
    cv::waitKey(0);
    return 0;
}