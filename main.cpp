#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <iostream>
#include <iterator>
#include <sstream>
#include <fstream>
#include <vector>
#include <cmath>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <algorithm>
#include <cstring>

using namespace std;
using namespace cv;

struct accumulator
{
	vector<vector<vector<double>>> accu;
};

struct CircleStruct
{
	int x, y, r, v;
};

bool compareByValue(const CircleStruct &a, const CircleStruct &b)
{
	return a.v > b.v;
}

double deg2rad(double degrees)
{
	return degrees * 4.0 * atan(1.0) / 180.0;
}

auto HoughTransform(unsigned char *img_data, int w, int h, accumulator accu)
{
	cout << "\rRecherche de cercles \n";
	for (int y = 0; y < h; y++)
	{
		// cout << (int)(y * 100 / (h)) << "%\n" << flush; // consomme trop de temps
		for (int x = 0; x < w; x++)
		{
			if (img_data[(y * w) + x] > 25)
			{
				for (int r = 1; r < sqrt(pow(w, 2) + pow(h, 2)); r++)
				{
					for (int t = 0; t < 360; t++)
					{
						int b = y - r * sin(t);
						int a = x - r * cos(t);
						if (a < w && b < h && a >= 0 && b >= 0)
						{
							accu.accu[a][b][r]++;
						}
					}
				}
			}
		}
	}

	return accu;
}

vector<CircleStruct> AccumulatorThreshold(accumulator accu, double threshold)
{
	vector<CircleStruct> circles;
	for (int x = 2; x < accu.accu.size() - 2; x += 4)
	{
		for (int y = 2; y < accu.accu[x].size() - 2; y += 4)
		{
			int max = 0;
			CircleStruct circleMax = {0, 0, 0};
			for (int r = 7; r < accu.accu[x][y].size() - 2; r += 4)
			{

				for (int xnear = x - 2; xnear < x + 2; xnear++)
				{
					for (int ynear = y - 2; ynear < y + 2; ynear++)
					{
						for (int rnear = r - 2; rnear < r + 2; rnear++)
						{
							if (accu.accu[xnear][ynear][rnear] > max)
							{
								max = accu.accu[xnear][ynear][rnear];
								circleMax = {xnear, ynear, rnear, max};
							}
						}
					}
				}
			}
			if (accu.accu[circleMax.x][circleMax.y][circleMax.r] > threshold)
			{
				// cout << circleMax.v << "\n";
				circles.push_back({circleMax.x, circleMax.y, circleMax.r, circleMax.v});
			}
		}
	}
	return circles;
}

Mat preprocessImage(Mat img)
{
	int ddepth = CV_16S;
	int ksize = 1;
	int scale = 1;
	int delta = 0;
	Mat src, src_gray, grad;

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

	return grad;
}

void showCircles(Mat img, vector<CircleStruct> circles, int number_of_circles)
{
	sort(circles.begin(), circles.end(), compareByValue);
	for (size_t i = 0; i < min((int)(circles.size()), number_of_circles); i++)
	{
		cout << circles[i].v << "\n";
		Point center(cvRound(circles[i].x), cvRound(circles[i].y));
		int radius = cvRound(circles[i].r);
		circle(img, center, radius, Scalar(0, 0, 255), 1, 8, 0);
		circle(img, center, 0, Scalar(0, 255, 0), 1, 8, 0);
	}
}

Mat HoughCirclesProcess(Mat img, int number_of_circles)
{
	Mat grad = preprocessImage(img); // Preprocessing

	// threshold(grad, grad, 25, 255, THRESH_BINARY);

	// Initialisation de l'accumulateur
	int r = (int)(sqrt(pow(grad.cols, 2) + pow(grad.rows, 2)));
	accumulator accu, accu2;
	accu.accu = vector<vector<vector<double>>>(grad.cols, vector<vector<double>>(grad.rows, vector<double>(r)));

	auto start = chrono::high_resolution_clock::now();

	accu = HoughTransform(grad.data, grad.cols, grad.rows, accu);	//Détection de cercles
	vector<CircleStruct> circles = AccumulatorThreshold(accu, 250); //Filtrage
	showCircles(img, circles, number_of_circles);

	auto finish = std::chrono::high_resolution_clock::now();
	chrono::duration<double> totalTime = finish - start;
	cout << "Temps total: " << (int)(totalTime.count() * 1000) << " ms" << std::endl;

	return img;
}

int main(int argc, char **argv)
{
	string input_path;
	int number_of_circles;

	char *p;

	if (argc < 2)
	{
		cout << "usage: " << argv[0] << " <input> <number_of_circles>" << std::endl;
		input_path = "images/four.png";
		number_of_circles = 9;
	}
	else
	{
		input_path = argv[1];
		// number_of_circles = std::stoi(argv[1]);
		errno = 0;
		long arg = strtol(argv[2], &p, 10);
		if (*p == '\0' && errno == 0)
		{
			if (arg > INT_MIN && arg < INT_MAX)
			{
				number_of_circles = arg;
			}
		}
	}

	string image_path = samples::findFile(input_path);
	Mat img = imread(image_path, IMREAD_COLOR);
	if (img.empty())
	{
		cout << "Could not read the image: " << image_path << endl;
		return 1;
	}

	if (img.cols > 150 && img.rows > 150)
	{
		// optimisation dans le cas d'une image plus grande
		auto originalImage = img;
		resize(img, img, Size(img.cols / 2, img.rows / 2), INTER_LINEAR);
	}

	img = HoughCirclesProcess(img, number_of_circles);

	// Affichage
	resize(img, img, Size(img.cols * 8, img.rows * 8), INTER_LINEAR);
	imshow("Display window", img);
	// resize(grad, grad, Size(grad.cols * 8, grad.rows * 8), INTER_LINEAR);
	// imshow("Display window", grad);
	waitKey(0);

	return 0;
}