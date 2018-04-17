#include "MyForm.h"
#include <opencv2/opencv.hpp>


using namespace GHplaterecognition;
using namespace cv;


//��ȡ��ֵ����ֵ
int otsu(IplImage* frame)
{	
#define GrayScale 256//frame�Ҷȼ�
	int width = frame->width;
	int height = frame->height;
	int pixelCount[GrayScale] = { 0 };
	float pixelPro[GrayScale] = { 0 };
	int i, j, pixelSum = width * height, threshold = 0;
	uchar* data = (uchar*)frame->imageData;

	//ͳ��ÿ���Ҷȼ������صĸ���  
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			pixelCount[(int)data[i * width + j]]++;
		}
	}

	//����ÿ���Ҷȼ���������Ŀռ����ͼ��ı���  
	for (i = 0; i < GrayScale; i++)
	{
		pixelPro[i] = (float)pixelCount[i] / pixelSum;
	}

	//�����Ҷȼ�[0,255],Ѱ�Һ��ʵ�threshold 
	float w0, w1, u0tmp, u1tmp, u0, u1, dataTmp, dataMax = 0;
	for (i = 0; i < GrayScale; i++)
	{
		w0 = w1 = u0tmp = u1tmp = u0 = u1 = dataTmp = 0;
		for (j = 0; j < GrayScale; j++)
		{
			if (j <= i)   //��������  
			{
				w0 += pixelPro[j];
				u0tmp += j * pixelPro[j];
			}
			else   //ǰ������  
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

	//����ͼƬ
	IplImage* srcImg = cvLoadImage(pstrImageName, CV_LOAD_IMAGE_ANYCOLOR);

	//ɫ�ʿռ�ת��ΪHSV
	IplImage* colorImg = cvCreateImage(cvGetSize(srcImg), srcImg->depth, 3);
	cvCvtColor(srcImg, colorImg, CV_BGR2HSV);

	//�����ͨ���ֱ��Ƶ�������ͨ����
	IplImage* imgH = cvCreateImage(cvGetSize(srcImg), srcImg->depth, 1);
	IplImage* imgS = cvCreateImage(cvGetSize(srcImg), srcImg->depth, 1);
	IplImage* imgV = cvCreateImage(cvGetSize(srcImg), srcImg->depth, 1);
	cvSplit(colorImg, imgH, imgS, imgV, NULL);
	
	//ѡȡ����ͨ��������ֵ��Χ���ڷ�Χ�ڵ�����1��������0������ɫ���ж�ֵ����
	cvInRangeS(imgH, cvScalar(94, 0, 0, 0), cvScalar(115, 0, 0, 0), imgH);   
	cvInRangeS(imgS, cvScalar(90, 0, 0, 0), cvScalar(255, 0, 0, 0), imgS);   
	cvInRangeS(imgV, cvScalar(36, 0, 0, 0), cvScalar(255, 0, 0, 0), imgV);
	
	//H,S,V����ͨ���ֱ�λ���룬�����õĵ�ͨ��ͼ�񱣴浽imgHsvBinnary��
    IplImage* tempImg = cvCreateImage(cvGetSize(srcImg), srcImg->depth, 1);
	IplImage* binnaryHSVImg = cvCreateImage(cvGetSize(srcImg), srcImg->depth, 1);
	cvAnd(imgH, imgS, tempImg);
	cvAnd(tempImg, imgV, binnaryHSVImg);
	
	//�����㴦��
	int values[2] = { 255,255 };
	int rows = 2, cols = 1, anchor_x = 0, anchor_y = 1;
	IplConvKernel *element = cvCreateStructuringElementEx(cols, rows, anchor_x, anchor_y, CV_SHAPE_CUSTOM, values);//�Զ����
	cvDilate(binnaryHSVImg, binnaryHSVImg, element, 2);    //����
	cvErode(binnaryHSVImg, binnaryHSVImg, element, 2);     //��θ�ʴ��2�Σ�����������
	
	//��ʾHSVͼ��
	//cvNamedWindow("HSVImg", 1);
	//cvShowImage("HSVImg", binnaryHSVImg);

	//��ֵ�ָ�
	IplImage* grayImg = cvCreateImage(cvGetSize(srcImg), srcImg->depth, 1);
	cvCvtColor(srcImg, grayImg, CV_BGR2GRAY);//ת��ͼ��Ϊ�Ҷ�ͼ��
	int Thresold = otsu(grayImg);
	IplImage* binaryImg = cvCreateImage(cvGetSize(grayImg), srcImg->depth, 1);
	cvThreshold(grayImg, binaryImg, Thresold, 255, CV_THRESH_OTSU); //��ֵ������
    cvSmooth(binaryImg, binaryImg, CV_MEDIAN, 3, 3);	//��ֵ�˲�
	//������ֵͼ����
	cvNamedWindow("BinImg", 2);
	cvShowImage("BinImg", binaryImg);

	//���ƶ�λ
	//�ж�λ�����ݳ��Ƶ������ͼ���������ж�λ��
	int hop_num = 8; //�ַ����������������ֵ
	int num = 0;      //��������Ĵ���
	int begin = 0;    //�����Ƿ�ʼ
	int mark_Row[2] = { 0 }, k = 0;//��һ�α�ǳ��ƵĿ�ʼ���������
	int mark_Row1[2] = { 0 };   //�ڶ��α��
    //��һ�ζ�λ
	for (int i = srcImg->height - 1; i >= 0; i--)
	{
		num = 0;
		for (int j = 0; j<srcImg->width - 1; j++)
		{
			if (cvGet2D(binnaryHSVImg, i, j).val[0] != cvGet2D(binnaryHSVImg, i, j + 1).val[0])  //�������е�ֵ��������Ϊһ������
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
	cvLine(srcImg,cvPoint(0,mark_Row[0]),cvPoint(srcImg->width,mark_Row[0]),CV_RGB(255,255,0));   //��ԭͼ�л�������ǵ�����
	cvLine(srcImg,cvPoint(0,mark_Row[1]),cvPoint(srcImg->width,mark_Row[1]),CV_RGB(255,255,0));
	//�ж�λ
	int mark_col[2] = { 0 }, mark_col1[2] = { 0 }, num_col = 0, k_col = 0;
	int a[100] = { 0 }, Thresold_col = 7;
	for (int j = 0; j<srcImg->width; j++)
	{
		num_col = 0;
		for (int i = mark_Row[1]; i<mark_Row[0]; i++)    //ֻɨ���Ѿ���ǵ�����֮���ͼ��
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
	if (license_Width1 / license_Height1<3)    //���ݳ��ƵĿ�Ⱥ͸߶ȱȶԳ��������������
	{
		int real_height1 = license_Width1 / 3;    //���ƵĿ�Ⱥ͸߶ȱȴ��Ϊ3:1
		mark_Row[1] = mark_Row[0] - real_height1;
		license_Height1 = real_height1;
	}
	
	//��һ�ζ�λ����
	cvNamedWindow("SRCImg", 3);
	cvShowImage("SRCImg", srcImg);

	//�ڶ��ζ�λ���ڵ�һ�ζ�λ�Ļ���֮�ϣ�
	k = 0;
	for (int i = mark_Row[0]; i>mark_Row[1]; i--)
	{
		num = 0;
		for (int j = mark_col[0]; j<mark_col[1]; j++)
		{
			if (cvGet2D(binnaryHSVImg, i, j).val[0] != cvGet2D(binnaryHSVImg, i, j + 1).val[0])  //�������е�ֵ��������Ϊһ������
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
		for (int i = mark_Row1[1]; i<mark_Row1[0]; i++)  //ֻɨ���Ѿ���ǵ�����֮���ͼ��
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
	if (license_Width / license_Height<3)  //���ݿ�Ⱥ͸߶ȱ��ٴ�����
	{
		int real_height = license_Width / 3;  //���ƵĿ�Ⱥ͸߶ȱȴ��Ϊ3:1
		mark_Row1[1] = mark_Row1[0] - real_height;
		license_Height = real_height;
	}
	
	IplImage*SrcLicenseimg1;
	IplImage*SrcLicenseimg2;
	cvSetImageROI(srcImg, cvRect(mark_col1[0], mark_Row1[1], license_Width, license_Height)); //��������������ΪROI����
	cvSetImageROI(binaryImg, cvRect(mark_col1[0], mark_Row1[1], license_Width, license_Height));
	cvSetImageROI(binnaryHSVImg, cvRect(mark_col1[0], mark_Row1[1], license_Width, license_Height));
	imgLicense = cvCreateImage(cvGetSize(srcImg), srcImg->depth, srcImg->nChannels);  //������ʾ�ĳ���ͼƬ
	SrcLicenseimg1 = cvCreateImage(cvGetSize(binnaryHSVImg), binaryImg->depth, binaryImg->nChannels);
	SrcLicenseimg2 = cvCreateImage(cvGetSize(binnaryHSVImg), binnaryHSVImg->depth, binnaryHSVImg->nChannels);
	cvCopy(srcImg, imgLicense, 0);
	cvCopy(binaryImg, SrcLicenseimg1, 0); //���������򿽱�����Ӧ��ͼ����
	cvCopy(binnaryHSVImg, SrcLicenseimg2, 0);
	
	//��ʾ���ƵĶ�ֵ��ͼƬ
	cvNamedWindow("imgLicense", 6);
	cvShowImage("imgLicense", imgLicense);
	cvNamedWindow("SrcLicenseimg1",4);  
	cvShowImage("SrcLicenseimg1",SrcLicenseimg1);
	cvNamedWindow("SrcLicenseimg2",5);  
	cvShowImage("SrcLicenseimg2",SrcLicenseimg2);

	
	//�ַ��ָ�
	int nCharWidth = 45;
	int nSpace = 12;
	for (i = 0; i<7; i++)           //�õ�ÿ���ַ���˫�߽�
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
	for (i = 0; i<14; i++)        //����ÿ���ַ�������
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
	cvCopy(SrcLicenseimg1, pImgCharOne, NULL); //��ȡ��1���ַ�
	cvResetImageROI(SrcLicenseimg1);

	ROI_rect1.x = mark_col[2];
	ROI_rect1.y = 0;
	ROI_rect1.width = nCharWidth;
	ROI_rect1.height = license_Height;
	cvSetImageROI(SrcLicenseimg1, ROI_rect1);
	cvCopy(SrcLicenseimg1, pImgCharTwo, NULL); //��ȡ��2���ַ�
	cvResetImageROI(SrcLicenseimg1);

	ROI_rect1.x =mark_col[4];
	ROI_rect1.y = 0;
	ROI_rect1.width = nCharWidth;
	ROI_rect1.height = license_Height;
	cvSetImageROI(SrcLicenseimg1, ROI_rect1);
	cvCopy(SrcLicenseimg1, pImgCharThree, NULL); //��ȡ��3���ַ�
	cvResetImageROI(SrcLicenseimg1);

	ROI_rect1.x = mark_col[6];
	ROI_rect1.y = 0;
	ROI_rect1.width = nCharWidth;
	ROI_rect1.height = license_Height;
	cvSetImageROI(SrcLicenseimg1, ROI_rect1);
	cvCopy(SrcLicenseimg1, pImgCharFour, NULL); //��ȡ��4���ַ�
	cvResetImageROI(SrcLicenseimg1);

	ROI_rect1.x = mark_col[8];
	ROI_rect1.y = 0;
	ROI_rect1.width = nCharWidth;
	ROI_rect1.height = license_Height;
	cvSetImageROI(SrcLicenseimg1, ROI_rect1);
	cvCopy(SrcLicenseimg1, pImgCharFive, NULL); //��ȡ��5���ַ�
	cvResetImageROI(SrcLicenseimg1);

	ROI_rect1.x = mark_col[10];
	ROI_rect1.y = 0;
	ROI_rect1.width = nCharWidth;
	ROI_rect1.height = license_Height;
	cvSetImageROI(SrcLicenseimg1, ROI_rect1);
	cvCopy(SrcLicenseimg1, pImgCharSix, NULL); //��ȡ��6���ַ�
	cvResetImageROI(SrcLicenseimg1);

	ROI_rect1.x = mark_col[12];
	ROI_rect1.y = 0;
	ROI_rect1.width = nCharWidth;
	ROI_rect1.height = license_Height;
	cvSetImageROI(SrcLicenseimg1, ROI_rect1);
	cvCopy(SrcLicenseimg1, pImgCharSeven, NULL); //��ȡ��7���ַ�
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