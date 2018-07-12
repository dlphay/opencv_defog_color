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

//���㼫��Сֵ
void find_vhi_vlo(const Mat &img, double &vhi, double &vlo)
{
	int width = img.cols;  // ͼ�������
	int height = img.rows; // ͼ�������

	uchar min;

	vector<Mat> BGR;   //Mat����
	split(img, BGR);   // ��BGRͼ��ָ�ɵ�ɫͼ�񱣴���������
	//conver to CMY
	BGR[0] = 255 - BGR[0];  // ͼ��ת
	BGR[1] = 255 - BGR[1];// ͼ��ת
	BGR[2] = 255 - BGR[2];// ͼ��ת

	Mat CMY(img.size(), CV_8UC3); // ������ͼ
	merge(BGR, CMY);  // ����

	for (int i = 0; i < height; i++) // ����ͼ��Ѱ��3ͨ���ϵļ�Сֵ��
	{
		Vec3b *pImg = CMY.ptr<Vec3b>(i);
		for (int j = 0; j < width; j++)  // 
		{
			min = pImg[j][0];
			if (pImg[j][1] < min) min = pImg[j][1];
			if (pImg[j][2] < min) min = pImg[j][2];
			for (int k = 0; k < 3; k++)
			{
				pImg[j][k] -= min;   // 3ͨ����Ӧ������ֵ�ֱ��ȥ��Сֵ��


			}
		}
	}
	Mat HSV = Mat(CMY.size(), CV_8UC3);   // �½�ͼ��
	cvtColor(CMY, HSV, COLOR_BGR2HSV);   // BGRͨ��ת����HSVͨ��
	vector<Mat> vHSV; // Mat����
	split(HSV, vHSV);// ��HSVͼ��ָ�ɵ�ɫͼ�񱣴���������

	////find Vmin and Vmax

	double vMin = 1.0;
	double vMax = .0;
	for (int i = 0; i < height; i++)  // ����ͼ��Ѱ��3ͨ���ϵļ���ֵ�ͼ�Сֵ��
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
//v��������
//img��ͨ��
void quantizing_v(Mat &img, double vMax, double vMin)
{
	if (vMax == vMin)	return;   // ������ �˳�
	if (img.channels() != 1)	return;  // ������ �˳�

	int width = img.cols; // ͼ�������
	int height = img.rows;  // ͼ�������

	
	for (int i = 0; i < height; i++)  // ͼ������б�����ʵ��v��������
	{
		uchar *pImg = img.ptr<uchar>(i);
		for (int j = 0; j < width; j++)
		{
			double newPixel = ((double)pImg[j] / vdelta - vMin) / (vMax - vMin);
			int tmp = int(newPixel * 255);
			if (tmp >255)    // ����ɸѡ
				pImg[j] = 255;
			else if (tmp < 0)
				pImg[j] = 0;
			else
				pImg[j] = (uchar)tmp;
		}
	}
}

// ���� find_vhi_vlo����+quantizing_v���������������������
void color_enhance(const Mat &img, Mat &dst, double vMax, double vMin)
{
	double v;//v����

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

	for (int i = 0; i < height; i++) // ͼ������б�����ʵ��v��������
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

//������Բģ��
int initEllipseModel(Mat &skinEllipse)
{
	if (skinEllipse.channels() != 1)
		return -1;
	skinEllipse = Mat::zeros(Size(256, 256), CV_8UC1);
	ellipse(skinEllipse, Point(113, 155.6), Size(23.4, 15.2), 43.0, 0.0, 360.0, Scalar(255, 255, 255), -1);
	return 0;
}

//Ƥ��������
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

	find_vhi_vlo(src, vhi, vlo); // �ҵ�����ֵ��Сֵ
	Mat dst(src.size(), src.type());

	color_enhance(src, dst, vhi, vlo);  // v���������õ����
	imshow("enhance", dst);
	waitKey(0);
}