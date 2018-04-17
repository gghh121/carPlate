#include "MyForm.h"
#include <opencv2/opencv.hpp>


using namespace GHplaterecognition;
using namespace cv;


//求取二值化阈值
int otsu(IplImage* frame)
{	
#define GrayScale 256//frame灰度级
	int width = frame->width;
	int height = frame->height;
	int pixelCount[GrayScale] = { 0 };
	float pixelPro[GrayScale] = { 0 };
	int i, j, pixelSum = width * height, threshold = 0;
	uchar* data = (uchar*)frame->imageData;

	//统计每个灰度级中像素的个数  
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			pixelCount[(int)data[i * width + j]]++;
		}
	}

	//计算每个灰度级的像素数目占整幅图像的比例  
	for (i = 0; i < GrayScale; i++)
	{
		pixelPro[i] = (float)pixelCount[i] / pixelSum;
	}

	//遍历灰度级[0,255],寻找合适的threshold 
	float w0, w1, u0tmp, u1tmp, u0, u1, dataTmp, dataMax = 0;
	for (i = 0; i < GrayScale; i++)
	{
		w0 = w1 = u0tmp = u1tmp = u0 = u1 = dataTmp = 0;
		for (j = 0; j < GrayScale; j++)
		{
			if (j <= i)   //背景部分  
			{
				w0 += pixelPro[j];
				u0tmp += j * pixelPro[j];
			}
			else   //前景部分  
			{
				w1 += pixelPro[j];
				u1tmp += j * pixelPro[j];
			}
		}
		u0 = u0tmp / w0;
		u1 = u1tmp / w1;
		dataTmp = (float)(w0 *w1* pow((u0 - u1), 2));
		if (dataTmp > dataMax)
		{
			dataMax = dataTmp;
			threshold = i;
		}
	}
	return threshold;
}

void opencv_helloworld(char *pstrImageName) {

	//载入图片
	IplImage* srcImg = cvLoadImage(pstrImageName, CV_LOAD_IMAGE_ANYCOLOR);

	//色彩空间转换为HSV
	IplImage* colorImg = cvCreateImage(cvGetSize(srcImg), srcImg->depth, 3);
	cvCvtColor(srcImg, colorImg, CV_BGR2HSV);

	//将多个通道分别复制到各个单通道中
	IplImage* imgH = cvCreateImage(cvGetSize(srcImg), srcImg->depth, 1);
	IplImage* imgS = cvCreateImage(cvGetSize(srcImg), srcImg->depth, 1);
	IplImage* imgV = cvCreateImage(cvGetSize(srcImg), srcImg->depth, 1);
	cvSplit(colorImg, imgH, imgS, imgV, NULL);
	
	//选取各个通道的像素值范围，在范围内的则置1，否则置0（按颜色进行二值化）
	cvInRangeS(imgH, cvScalar(94, 0, 0, 0), cvScalar(115, 0, 0, 0), imgH);   
	cvInRangeS(imgS, cvScalar(90, 0, 0, 0), cvScalar(255, 0, 0, 0), imgS);   
	cvInRangeS(imgV, cvScalar(36, 0, 0, 0), cvScalar(255, 0, 0, 0), imgV);
	
	//H,S,V三个通道分别按位求与，将所得的单通道图像保存到imgHsvBinnary中
    IplImage* tempImg = cvCreateImage(cvGetSize(srcImg), srcImg->depth, 1);
	IplImage* binnaryHSVImg = cvCreateImage(cvGetSize(srcImg), srcImg->depth, 1);
	cvAnd(imgH, imgS, tempImg);
	cvAnd(tempImg, imgV, binnaryHSVImg);
	
	//闭运算处理
	int values[2] = { 255,255 };
	int rows = 2, cols = 1, anchor_x = 0, anchor_y = 1;
	IplConvKernel *element = cvCreateStructuringElementEx(cols, rows, anchor_x, anchor_y, CV_SHAPE_CUSTOM, values);//自定义核
	cvDilate(binnaryHSVImg, binnaryHSVImg, element, 2);    //膨胀
	cvErode(binnaryHSVImg, binnaryHSVImg, element, 2);     //多次腐蚀（2次），消除噪声
	
	//显示HSV图像
	//cvNamedWindow("HSVImg", 1);
	//cvShowImage("HSVImg", binnaryHSVImg);

	//阈值分割
	IplImage* grayImg = cvCreateImage(cvGetSize(srcImg), srcImg->depth, 1);
	cvCvtColor(srcImg, grayImg, CV_BGR2GRAY);//转换图像为灰度图像
	int Thresold = otsu(grayImg);
	IplImage* binaryImg = cvCreateImage(cvGetSize(grayImg), srcImg->depth, 1);
	cvThreshold(grayImg, binaryImg, Thresold, 255, CV_THRESH_OTSU); //二值化处理
    cvSmooth(binaryImg, binaryImg, CV_MEDIAN, 3, 3);	//中值滤波
	//创建二值图窗口
	cvNamedWindow("BinImg", 2);
	cvShowImage("BinImg", binaryImg);

	//车牌定位
	//行定位（根据车牌的区域的图像特征进行定位）
	int hop_num = 8; //字符连续跳变次数的阈值
	int num = 0;      //计算跳变的次数
	int begin = 0;    //跳变是否开始
	int mark_Row[2] = { 0 }, k = 0;//第一次标记车牌的开始行与结束行
	int mark_Row1[2] = { 0 };   //第二次标记
    //第一次定位
	for (int i = srcImg->height - 1; i >= 0; i--)
	{
		num = 0;
		for (int j = 0; j<srcImg->width - 1; j++)
		{
			if (cvGet2D(binnaryHSVImg, i, j).val[0] != cvGet2D(binnaryHSVImg, i, j + 1).val[0])  //左右两列的值不等则视为一次跳变
			{
				num++;
			}
		}
		if (num>hop_num)
		{
			mark_Row[k] = i;
			k = 1;
		}
	}
	cvLine(srcImg,cvPoint(0,mark_Row[0]),cvPoint(srcImg->width,mark_Row[0]),CV_RGB(255,255,0));   //在原图中画出所标记的两行
	cvLine(srcImg,cvPoint(0,mark_Row[1]),cvPoint(srcImg->width,mark_Row[1]),CV_RGB(255,255,0));
	//列定位
	int mark_col[2] = { 0 }, mark_col1[2] = { 0 }, num_col = 0, k_col = 0;
	int a[100] = { 0 }, Thresold_col = 7;
	for (int j = 0; j<srcImg->width; j++)
	{
		num_col = 0;
		for (int i = mark_Row[1]; i<mark_Row[0]; i++)    //只扫描已经标记的两行之间的图像
			if (cvGet2D(binnaryHSVImg, i, j).val[0]>0)
				num_col++;
		if (num_col>Thresold_col)
		{
			mark_col[k_col] = j;
			k_col = 1;
		}
	}
	int i = 0;
	cvLine(srcImg,cvPoint(mark_col[0],0),cvPoint(mark_col[0], srcImg->height),CV_RGB(255,0,0));
	cvLine(srcImg,cvPoint(mark_col[1],0),cvPoint(mark_col[1], srcImg->height),CV_RGB(255,0,0));
	IplImage *imgLicense;
	int license_Width1 = (mark_col[1] - mark_col[0]);
	int license_Height1 = mark_Row[0] - mark_Row[1];
	if (license_Width1 / license_Height1<3)    //根据车牌的宽度和高度比对车牌区域进行修正
	{
		int real_height1 = license_Width1 / 3;    //车牌的宽度和高度比大概为3:1
		mark_Row[1] = mark_Row[0] - real_height1;
		license_Height1 = real_height1;
	}
	
	//第一次定位窗口
	cvNamedWindow("SRCImg", 3);
	cvShowImage("SRCImg", srcImg);

	//第二次定位（在第一次定位的基础之上）
	k = 0;
	for (int i = mark_Row[0]; i>mark_Row[1]; i--)
	{
		num = 0;
		for (int j = mark_col[0]; j<mark_col[1]; j++)
		{
			if (cvGet2D(binnaryHSVImg, i, j).val[0] != cvGet2D(binnaryHSVImg, i, j + 1).val[0])  //左右两列的值不等则视为一次跳变
			{
				num++;
			}
		}
		if (num>8)
		{
			mark_Row1[k] = i;
			k = 1;
		}
	}
	k_col = 0;
	for (int j = mark_col[0]; j<mark_col[1]; j++)
	{
		num_col = 0;
		for (int i = mark_Row1[1]; i<mark_Row1[0]; i++)  //只扫描已经标记的两行之间的图像
			if (cvGet2D(binnaryHSVImg, i, j).val[0] > 0)
			{
				num_col++;
				if (num_col > 6)
				{
					mark_col1[k_col] = j;
					k_col = 1;
				}
			}
	}
	int license_Width = (mark_col1[1] - mark_col1[0]);
	int license_Height = mark_Row1[0] - mark_Row1[1];
	if (license_Width / license_Height<3)  //根据宽度和高度比再次修正
	{
		int real_height = license_Width / 3;  //车牌的宽度和高度比大概为3:1
		mark_Row1[1] = mark_Row1[0] - real_height;
		license_Height = real_height;
	}
	
	IplImage*SrcLicenseimg1;
	IplImage*SrcLicenseimg2;
	cvSetImageROI(srcImg, cvRect(mark_col1[0], mark_Row1[1], license_Width, license_Height)); //将车牌区域设置为ROI区域
	cvSetImageROI(binaryImg, cvRect(mark_col1[0], mark_Row1[1], license_Width, license_Height));
	cvSetImageROI(binnaryHSVImg, cvRect(mark_col1[0], mark_Row1[1], license_Width, license_Height));
	imgLicense = cvCreateImage(cvGetSize(srcImg), srcImg->depth, srcImg->nChannels);  //用于显示的车牌图片
	SrcLicenseimg1 = cvCreateImage(cvGetSize(binnaryHSVImg), binaryImg->depth, binaryImg->nChannels);
	SrcLicenseimg2 = cvCreateImage(cvGetSize(binnaryHSVImg), binnaryHSVImg->depth, binnaryHSVImg->nChannels);
	cvCopy(srcImg, imgLicense, 0);
	cvCopy(binaryImg, SrcLicenseimg1, 0); //将车牌区域拷贝到相应的图像中
	cvCopy(binnaryHSVImg, SrcLicenseimg2, 0);
	
	//显示车牌的二值化图片
	cvNamedWindow("imgLicense", 6);
	cvShowImage("imgLicense", imgLicense);
	cvNamedWindow("SrcLicenseimg1",4);  
	cvShowImage("SrcLicenseimg1",SrcLicenseimg1);
	cvNamedWindow("SrcLicenseimg2",5);  
	cvShowImage("SrcLicenseimg2",SrcLicenseimg2);

	
	//字符分割
	int nCharWidth = 45;
	int nSpace = 12;
	for (i = 0; i<7; i++)           //得到每个字符的双边界
	{
		switch (i) {
		case 0:
		case 1:
			mark_col[i * 2] = i*nCharWidth + i*nSpace;
			mark_col[i * 2 + 1] = (i + 1)*nCharWidth + i*nSpace;
			break;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			mark_col[i * 2] = i*nCharWidth + i*nSpace + 22;
			mark_col[i * 2 + 1] = (i + 1)*nCharWidth + i*nSpace + 22;
			break;
		}

	}
	for (i = 0; i<14; i++)        //画出每个字符的区域
	{
		cvLine(SrcLicenseimg1, cvPoint(mark_col[i], 0), cvPoint(mark_col[i], license_Height), cvScalar(255, 0, 0), 1, 8, 0);
		//cout<<col[i*2]<<" "<<col[2*i+1]<<" ";
	}

	IplImage *pImgCharOne = cvCreateImage(cvSize(nCharWidth, license_Height), IPL_DEPTH_8U, 1);
	IplImage *pImgCharTwo = cvCreateImage(cvSize(nCharWidth, license_Height), IPL_DEPTH_8U, 1);
	IplImage *pImgCharThree = cvCreateImage(cvSize(nCharWidth, license_Height), IPL_DEPTH_8U, 1);
	IplImage *pImgCharFour = cvCreateImage(cvSize(nCharWidth, license_Height), IPL_DEPTH_8U, 1);
	IplImage *pImgCharFive = cvCreateImage(cvSize(nCharWidth, license_Height), IPL_DEPTH_8U, 1);
	IplImage *pImgCharSix = cvCreateImage(cvSize(nCharWidth, license_Height), IPL_DEPTH_8U, 1);
	IplImage *pImgCharSeven = cvCreateImage(cvSize(nCharWidth, license_Height), IPL_DEPTH_8U, 1);

	CvRect ROI_rect1;
	ROI_rect1.x = mark_col[0];
	ROI_rect1.y = 0;
	ROI_rect1.width = nCharWidth;
	ROI_rect1.height = license_Height;
	cvSetImageROI(SrcLicenseimg1, ROI_rect1);
	cvCopy(SrcLicenseimg1, pImgCharOne, NULL); //获取第1个字符
	cvResetImageROI(SrcLicenseimg1);

	ROI_rect1.x = mark_col[2];
	ROI_rect1.y = 0;
	ROI_rect1.width = nCharWidth;
	ROI_rect1.height = license_Height;
	cvSetImageROI(SrcLicenseimg1, ROI_rect1);
	cvCopy(SrcLicenseimg1, pImgCharTwo, NULL); //获取第2个字符
	cvResetImageROI(SrcLicenseimg1);

	ROI_rect1.x =mark_col[4];
	ROI_rect1.y = 0;
	ROI_rect1.width = nCharWidth;
	ROI_rect1.height = license_Height;
	cvSetImageROI(SrcLicenseimg1, ROI_rect1);
	cvCopy(SrcLicenseimg1, pImgCharThree, NULL); //获取第3个字符
	cvResetImageROI(SrcLicenseimg1);

	ROI_rect1.x = mark_col[6];
	ROI_rect1.y = 0;
	ROI_rect1.width = nCharWidth;
	ROI_rect1.height = license_Height;
	cvSetImageROI(SrcLicenseimg1, ROI_rect1);
	cvCopy(SrcLicenseimg1, pImgCharFour, NULL); //获取第4个字符
	cvResetImageROI(SrcLicenseimg1);

	ROI_rect1.x = mark_col[8];
	ROI_rect1.y = 0;
	ROI_rect1.width = nCharWidth;
	ROI_rect1.height = license_Height;
	cvSetImageROI(SrcLicenseimg1, ROI_rect1);
	cvCopy(SrcLicenseimg1, pImgCharFive, NULL); //获取第5个字符
	cvResetImageROI(SrcLicenseimg1);

	ROI_rect1.x = mark_col[10];
	ROI_rect1.y = 0;
	ROI_rect1.width = nCharWidth;
	ROI_rect1.height = license_Height;
	cvSetImageROI(SrcLicenseimg1, ROI_rect1);
	cvCopy(SrcLicenseimg1, pImgCharSix, NULL); //获取第6个字符
	cvResetImageROI(SrcLicenseimg1);

	ROI_rect1.x = mark_col[12];
	ROI_rect1.y = 0;
	ROI_rect1.width = nCharWidth;
	ROI_rect1.height = license_Height;
	cvSetImageROI(SrcLicenseimg1, ROI_rect1);
	cvCopy(SrcLicenseimg1, pImgCharSeven, NULL); //获取第7个字符
	cvResetImageROI(SrcLicenseimg1);

	cvNamedWindow("one", CV_WINDOW_AUTOSIZE);
	cvShowImage("one", pImgCharOne);
	cvNamedWindow("two", 1);
	cvShowImage("two", pImgCharTwo);
	cvNamedWindow("three", 1);
	cvShowImage("three", pImgCharThree);
	cvNamedWindow("four", 1);
	cvShowImage("four", pImgCharFour);
	cvNamedWindow("five", 1);
	cvShowImage("five", pImgCharFive);
	cvNamedWindow("six", 1);
	cvShowImage("six", pImgCharSix);
	cvNamedWindow("seven", 1);
	cvShowImage("seven", pImgCharSeven);
	


	cvWaitKey(-1);

	cvReleaseImage(&srcImg);
	//cvReleaseImage(&grayImg);
	cvReleaseImage(&binnaryHSVImg);
	//cvReleaseImage(&binaryImg);
	cvReleaseImage(&imgLicense);
	cvReleaseImage(&SrcLicenseimg1);
	cvReleaseImage(&SrcLicenseimg2);
	cvReleaseImage(&pImgCharOne);
	cvReleaseImage(&pImgCharTwo);
	cvReleaseImage(&pImgCharThree);
	cvReleaseImage(&pImgCharFour);
	cvReleaseImage(&pImgCharFive);
	cvReleaseImage(&pImgCharSix);
	cvReleaseImage(&pImgCharSeven);
	cvDestroyAllWindows();
}