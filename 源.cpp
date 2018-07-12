#include<iostream>
#include<opencv2/opencv.hpp>
#include<sstream>
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

double vdelta = 255;

//计算极大极小值
void find_vhi_vlo(const Mat &img, double &vhi, double &vlo)
{
	int width = img.cols;  // 图像的列数
	int height = img.rows; // 图像的行数

	uchar min;

	vector<Mat> BGR;   //Mat容器
	split(img, BGR);   // 将BGR图像分割成单色图像保存在容器中
	//conver to CMY
	BGR[0] = 255 - BGR[0];  // 图像反转
	BGR[1] = 255 - BGR[1];// 图像反转
	BGR[2] = 255 - BGR[2];// 图像反转

	Mat CMY(img.size(), CV_8UC3); // 创建新图
	merge(BGR, CMY);  // 拷贝

	for (int i = 0; i < height; i++) // 遍历图像，寻找3通道上的极小值。
	{
		Vec3b *pImg = CMY.ptr<Vec3b>(i);
		for (int j = 0; j < width; j++)  // 
		{
			min = pImg[j][0];
			if (pImg[j][1] < min) min = pImg[j][1];
			if (pImg[j][2] < min) min = pImg[j][2];
			for (int k = 0; k < 3; k++)
			{
				pImg[j][k] -= min;   // 3通道对应的像素值分别减去极小值。


			}
		}
	}
	Mat HSV = Mat(CMY.size(), CV_8UC3);   // 新建图像
	cvtColor(CMY, HSV, COLOR_BGR2HSV);   // BGR通道转换到HSV通道
	vector<Mat> vHSV; // Mat容器
	split(HSV, vHSV);// 将HSV图像分割成单色图像保存在容器中

	////find Vmin and Vmax

	double vMin = 1.0;
	double vMax = .0;
	for (int i = 0; i < height; i++)  // 遍历图像，寻找3通道上的极大值和极小值。
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
	if (vMax == vMin)	return;   // 不符合 退出
	if (img.channels() != 1)	return;  // 不符合 退出

	int width = img.cols; // 图像的列数
	int height = img.rows;  // 图像的行数

	
	for (int i = 0; i < height; i++)  // 图像的行列遍历，实现v线性量化
	{
		uchar *pImg = img.ptr<uchar>(i);
		for (int j = 0; j < width; j++)
		{
			double newPixel = ((double)pImg[j] / vdelta - vMin) / (vMax - vMin);
			int tmp = int(newPixel * 255);
			if (tmp >255)    // 条件筛选
				pImg[j] = 255;
			else if (tmp < 0)
				pImg[j] = 0;
			else
				pImg[j] = (uchar)tmp;
		}
	}
}

// 上面 find_vhi_vlo函数+quantizing_v函数等于下面这个函数。
void color_enhance(const Mat &img, Mat &dst, double vMax, double vMin)
{
	double v;//v量化

	int width = img.cols;
	int height = img.rows;

	uchar min;
	
	vector<Mat> BGR;
	split(img, BGR);
	
	BGR[0] = 255 - BGR[0];
	BGR[1] = 255 - BGR[1];
	BGR[2] = 255 - BGR[2];

	Mat CMY(img.size(), CV_8UC3);
	merge(BGR, CMY);
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

	vector<Mat> vHSV;

	
	split(HSV, vHSV);
	quantizing_v(vHSV[2], vMax, vMin);

	merge(vHSV, dst);

	cvtColor(dst, dst, COLOR_HSV2BGR);

	for (int i = 0; i < height; i++) // 图像的行列遍历，实现v线性量化
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
}

//构建椭圆模型
int initEllipseModel(Mat &skinEllipse)
{
	if (skinEllipse.channels() != 1)
		return -1;
	skinEllipse = Mat::zeros(Size(256, 256), CV_8UC1);
	ellipse(skinEllipse, Point(113, 155.6), Size(23.4, 15.2), 43.0, 0.0, 360.0, Scalar(255, 255, 255), -1);
	return 0;
}

//皮肤检测程序
int skin_detect(const Mat &src, Mat &dstMask, const Mat &skinEllipse)
{
	if (!src.data)
		return -1;
	Mat ycbcr;
	cvtColor(src, ycbcr, CV_BGR2YCrCb);

	dstMask = Mat::zeros(src.size(), CV_8UC1);

	
	for (int i = 0; i < ycbcr.rows; i++)
	{
		const Vec3b* pSrc = ycbcr.ptr<Vec3b>(i);
		uchar *pMask = dstMask.ptr<uchar>(i);
		for (int j = 0; j < ycbcr.cols; j++)
		{
			uchar cr = pSrc[j][1];
			uchar cb = pSrc[j][2];

			if (skinEllipse.at<uchar>(cr, cb) > 0)
				pMask[j] = 255;
		}
	}
	return 0;
}

int main(){
	Mat src;
	Mat src1;
	string file("input6");
	int n = 1;
	string type = ".jpg";
	stringstream ss;
	ss << n;
	string ifile = file + type;


	src = imread(ifile, 1);
	if (!src.data)
		return 0;
	imshow("source", src);
	double vhi, vlo;

	find_vhi_vlo(src, vhi, vlo); // 找到极大值极小值
	Mat dst(src.size(), src.type());

	color_enhance(src, dst, vhi, vlo);  // v线性量化得到结果
	imshow("enhance", dst);
	waitKey(0);
}