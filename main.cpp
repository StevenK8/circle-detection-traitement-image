#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <iostream>
#include <iterator>
#include <sstream>
#include <fstream>

using namespace std;
using namespace cv;

int main(int argc, char **argv)
{
	Mat src, src_gray,grad;
	int ddepth = CV_16S;
	int ksize = 1;
   	int scale = 1;
  	int delta = 0;

	std::string image_path = samples::findFile("images/four.png");
	Mat img = imread(image_path, IMREAD_COLOR);
	if (img.empty())
	{
		std::cout << "Could not read the image: " << image_path << std::endl;
		return 1;
	}

    GaussianBlur(img, src, Size(3, 3), 0, 0, BORDER_DEFAULT);
    // Conversion de l'image en noir et blanc
    cvtColor(src, src_gray, COLOR_BGR2GRAY);
    Mat grad_x, grad_y;
    Mat abs_grad_x, abs_grad_y;
    Sobel(src_gray, grad_x, ddepth, 1, 0, ksize, scale, delta, BORDER_DEFAULT);
    Sobel(src_gray, grad_y, ddepth, 0, 1, ksize, scale, delta, BORDER_DEFAULT);
    // Reconversion en CV_8U
    convertScaleAbs(grad_x, abs_grad_x);
    convertScaleAbs(grad_y, abs_grad_y);
    addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad);

	// int[3] acc;

	threshold( grad, grad, 25, 255,THRESH_BINARY);

	resize(grad, grad, Size(grad.cols*8, grad.rows*8), INTER_LINEAR);
	imshow("Display window", grad);
	waitKey(0);
	return 0;
}

