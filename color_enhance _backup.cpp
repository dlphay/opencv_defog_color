#include<iostream>
#include<opencv2/opencv.hpp>
using namespace std;
using namespace cv;

/*
Stretch color saturation to cover maximum possible range",
"This simple plug-in does an automatic saturation stretch.
For each channel in the image, it finds the minimum and maximum values...
it uses those values to stretch the individual histograms to the full range.
For some images it may do just what you want; for others it may not work that well.
This version differs from Contrast Autostretch in that it works in HSV space, and preserves hue.

*/

double vdelta = 200;
double stheta = 30;

void find_vhi_vlo(const Mat &img, double &vhi, double &vlo)
{
	int width = img.cols;
	int height = img.rows;

	uchar min;

	vector<Mat> BGR;
	split(img, BGR);
	//conver to CMY
	BGR[0] = 255 - BGR[0];
	BGR[1] = 255 - BGR[1];
	BGR[2] = 255 - BGR[2];

	Mat CMY(img.size(), CV_8UC3);
	merge(BGR, CMY);


	for (int i = 0; i < height; i++)
	{
		Vec3b *pImg = CMY.ptr<Vec3b>(i);
		for (int j = 0; j < width; j++)
		{
			min = pImg[j][0];
			if (pImg[j][1] < min) min = pImg[j][1];
			if (pImg[j][2] < min) min = pImg[j][2];
			for (int k = 0; k < 3; k++)
			{
				pImg[j][k] -= min;


			}
		}
	}

	Mat HSV = Mat(CMY.size(), CV_8UC3);
	//imshow("cmy",CMY);
	cvtColor(CMY, HSV, COLOR_BGR2HSV);


	vector<Mat> vHSV;

	split(HSV, vHSV);

	//
	////find Vmin and Vmax

	double vMin = 1.0;
	double vMax = .0;

	for (int i = 0; i < height; i++)
	{
		uchar *pImg = vHSV[2].ptr<uchar>(i);
		for (int j = 0; j < width; j++)
		{
			double v = (double)pImg[j] / 255.0;

			if (v > vMax)	vMax = v;
			if (v < vMin)	vMin = v;
		}
	}

	vhi = vMax;
	vlo = vMin;

}
//v线性量化
//img单通道
void quantizing_v(Mat &img, double vMax, double vMin)
{
	if (vMax == vMin)	return;
	if (img.channels() != 1)	return;

	int width = img.cols;
	int height = img.rows;

	double delta = 300.0;
	for (int i = 0; i < height; i++)
	{
		uchar *pImg = img.ptr<uchar>(i);
		for (int j = 0; j < width; j++)
		{
			//double oldPixel = (double)pImg[j];
			double newPixel = ((double)pImg[j] / 255 - vMin) / (vMax - vMin);
			int tmp = int(newPixel * delta);
			if (tmp >255)
				pImg[j] = 255;
			else if (tmp < 0)
				pImg[j] = 0;
			else
				pImg[j] = (uchar)tmp;

		}
	}
	//cout << "new img" << endl << img << endl;
}



void color_enhance(const Mat &img, Mat &dst, double vMax, double vMin)
{
	double v;//v量化

	int width = img.cols;
	int height = img.rows;

	uchar min;
	vector<Mat> BGR;
	split(img, BGR);
	//equalizeHist(BGR[2],BGR[2]);
	
	//conver to CMY
	BGR[0] = 255 - BGR[0];
	BGR[1] = 255 - BGR[1];
	BGR[2] = 255 - BGR[2];

	Mat CMY(img.size(), CV_8UC3);
	merge(BGR, CMY);

	//imshow("cmy1", CMY);

	Mat minMat(img.size(), CV_8UC1);

	for (int i = 0; i < height; i++)
	{
		Vec3b *pImg = CMY.ptr<Vec3b>(i);
		uchar *pMin = minMat.ptr<uchar>(i);

		for (int j = 0; j < width; j++)
		{
			min = pImg[j][0];
			if (pImg[j][1] < min) min = pImg[j][1];
			if (pImg[j][2] < min) min = pImg[j][2];
			pMin[j] = min;
			for (int k = 0; k < 3; k++)
			{
				pImg[j][k] -= min;

			}
		}
	}

	Mat HSV = Mat(CMY.size(), CV_8UC3);
	cvtColor(CMY, HSV, COLOR_BGR2HSV);
	//imshow("cmy2", CMY);

	vector<Mat> vHSV;

	//imshow("HSV", HSV);

	split(HSV, vHSV);

	//cout << "old img" << endl << vHSV[2] << endl;

	quantizing_v(vHSV[2], vMax, vMin);

	merge(vHSV, dst);
	//imshow("dst-HSV", dst);

	cvtColor(dst, dst, COLOR_HSV2BGR);
	//imshow("rgb", dst);


	for (int i = 0; i < height; i++)
	{
		Vec3b *pDst = dst.ptr<Vec3b>(i);
		uchar *pMin = minMat.ptr<uchar>(i);

		for (int j = 0; j < width; j++)
		{
			for (int k = 0; k<3; k++)
			{
				int tmp = pDst[j][k] + pMin[j];
				if (tmp > 255)
					pDst[j][k] = 0;
				else
					pDst[j][k] = 255 - tmp;
			}

		}
	}
	imshow("1", dst);
	/*vector<Mat> RGB;
	split(dst,RGB);
	
	RGB[2] -= 50;
	

	merge(RGB,dst);*/
	imshow("2", dst);

	//Mat test(dst.size(), CV_8UC3);

	//cvtColor(dst, test, COLOR_BGR2HSV);
	////imshow("color_enhance", dst);
	////imwrite("color_enhance.jpg", dst);

	///*vector<Mat> tmp;
	//split(test, tmp);*/

	////equalizeHist(tmp[0], tmp[0]);

	//for (int i = 0; i < height; i++)
	//{
	//	Vec3b *pDst = test.ptr<Vec3b>(i);

	//	for (int j = 0; j < width; j++)
	//	{
	//		int tmp = pDst[j][1] - 30;
	//		if (tmp < 0)
	//			pDst[j][1] = 0;
	//		else
	//			pDst[j][1] = tmp;

	//	}
	//}
	//cvtColor(test, dst, COLOR_HSV2BGR);

}
int main(){
	Mat src = imread("img3.jpg", 1);
	//Mat src(4, 4, CV_8UC3, Scalar(122, 50, 200));
	imshow("source", src);

	double vhi, vlo;

	find_vhi_vlo(src, vhi, vlo);
	Mat dst(src.size(), src.type());

	color_enhance(src, dst, vhi, vlo);

	imshow("color_enhance_s-30", dst);
	//imwrite("color_enhance_new.jpg", dst);

	/*for (int i = 0; i < dst.rows; i++)
	{
	Vec3b *pDst = dst.ptr<Vec3b>(i);

	for (int j = 0; j < dst.cols; j++)
	{
	for (int k = 0; k<3; k++)
	{
	int tmp = pDst[j][k] - 10;
	if (tmp < 0)
	pDst[j][k] = 0;
	else
	pDst[j][k] = tmp;
	}

	}
	}
	imshow("de_color",dst);*/
	//imwrite("test.jpg",dst);
	waitKey(0);


	//VideoCapture cap(1);
	//if (cap.isOpened())
	//{
	//	while (1)
	//	{
	//		Mat src;
	//		cap >> src;
	//		double vhi, vlo;
	//		find_vhi_vlo(src, vhi, vlo);

	//		color_enhance(src, src, vhi, vlo);
	//		imshow("color_enhance", src);
	//		waitKey(25);
	//	}
	//}
}
/*
VideoCapture cap(0);
if (cap.isOpened())
{
while (1)
{
cap >> src;
double vhi, vlo;
find_vhi_vlo(src, vhi, vlo);
//Mat dst(src.size(), src.type());

color_enhance(src, src, vhi, vlo);
imshow("color_enhance", src);
waitKey(33);
}
}
*/