#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <iostream>
#include <iterator>
#include <sstream>
#include <fstream>
#include <vector>
#include <cmath>

using namespace std;
using namespace cv;

struct accumulator
{
	vector<vector<vector<double>>> accu;
};

struct CircleStruct
{
	int x, y, r;
};

double deg2rad(double degrees)
{
	return degrees * 4.0 * atan(1.0) / 180.0;
}

auto HoughTransform(unsigned char *img_data, int w, int h, accumulator accu)
{
	double center_x = w / 2;
	double center_y = h / 2;

	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			if (img_data[(y * w) + x] > 250)
			{
				for (int t = 0; t < 360; t++)
				{
					double r = sqrt(pow(center_x - x, 2) + pow(center_y - y, 2));
					int b = y - r * sin(deg2rad(t));
					int a = x - r * cos(deg2rad(t));
					if (a < x && b < y && a >= 0 && b >= 0)
					{
						accu.accu[a][b][r]++;
					}
				}
			}
		}
	}

	return accu;
}

vector<CircleStruct> AccumulatorThreshold(accumulator accu, accumulator accu2, double threshold)
{
	vector<CircleStruct> circles;
	for (int x = 0; x < accu.accu.size() - 1; x++)
	{
		for (int y = 0; y < accu.accu[x].size() - 1; y++)
		{
			for (int r = 0; r < accu.accu[x][y].size() - 1; r++)
			{
				if (accu.accu[x][y][r] > threshold)
				{
					cout << accu.accu[x][y][r] << "\n";
					accu2.accu[x][y][r] = accu.accu[x][y][r];

					circles.push_back({x, y, r});
				}
			}
		}
	}
	return circles;
}

int main(int argc, char **argv)
{
	Mat src, src_gray, grad;
	int ddepth = CV_16S;
	int ksize = 1;
	int scale = 1;
	int delta = 0;

	string image_path = samples::findFile("images/four.png");
	Mat img = imread(image_path, IMREAD_COLOR);
	if (img.empty())
	{
		cout << "Could not read the image: " << image_path << endl;
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

	threshold(grad, grad, 25, 255, THRESH_BINARY);

	int r = (int)(sqrt(pow(grad.cols / 2, 2) + pow(grad.rows, 2)));
	accumulator accu, accu2;
	accu.accu = vector<vector<vector<double>>>(grad.cols, vector<vector<double>>(grad.rows, vector<double>(r)));
	accu = HoughTransform(grad.data, grad.cols, grad.rows, accu);

	//Initialize the accumulator (H[a,b,r]) to all zeros
	// Find the edge image using any edge detector
	// For r= 0 to diagonal image length
	// For each edge pixel (x,y) in the image
	// For Θ = 0 to 360
	// a = x – r*cosΘ
	// b = y – r*sinΘ
	// H[a,b,r] = H[a,b,r] +1
	// Find the [a,b,r] value(s), where H[a,b,r] is above a suitable threshold value
	//https://theailearner.com/tag/hough-transform-opencv/

	accu2.accu = vector<vector<vector<double>>>(grad.cols, vector<vector<double>>(grad.rows, vector<double>(r)));
	vector<CircleStruct> circles = AccumulatorThreshold(accu, accu2, 30);

	for (size_t i = 0; i < circles.size(); i++)
	{
		Point center(cvRound(circles[i].x), cvRound(circles[i].y));
		int radius = cvRound(circles[i].r);
		circle(img, center, radius, Scalar(0, 0, 255), 2, 8, 0);
	}

	resize(img, img, Size(img.cols * 8, img.rows * 8), INTER_LINEAR);
	imshow("Display window", img);
	// resize(grad, grad, Size(grad.cols * 8, grad.rows * 8), INTER_LINEAR);
	// imshow("Display window", grad);
	waitKey(0);

	return 0;
}