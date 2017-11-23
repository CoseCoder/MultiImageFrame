#pragma once
#include <opencv2/opencv.hpp>
#include <QImage>
#include <vector>
using namespace cv;
using namespace std;

enum Mode {
	NORMAL_CLONE,
	MIXED_CLONE,
};

class PoissonImageEditing
{
public:
	PoissonImageEditing();

	PoissonImageEditing(const QImage &image1, QImage &image2);

	~PoissonImageEditing();

	// img1: 3-channel image, we wanna move something in it into img2.
	// img2: 3-channel image, destination image.
	// ROI: the position and size of the block we want to move in img1.
	// posX, posY: where we want to move the block to in img2
	// flag: ѡ��poisson�ںϵķ�ʽ
	QImage poissonBlending(QRect &ROI, int posX, int posY, int flag);

private:
	void QImage2cvMat(const QImage &image, Mat &img);
	QImage cvMat2QImage(const Mat& mat);

	// ��ʼ������
	void initVariables(const Mat &destination, const Mat &binaryMask);

	// binaryMaskΪͼ��destination���޸ĵ������mask
	void computeDerivatives(const Mat &destination, const Mat &patch, const Mat &binaryMask);
	void scalarProduct(Mat mat, float r, float g, float b);

	//�����ؽ� 
	void poisson(const Mat &destination);

	// ���룺I��������ͼƬ  wmask�����д��޸ĵ������mask  
	// �����cloned 
	void evaluate(const Mat &I, const Mat &wmask, const Mat &cloned);

	// ���ÿ��ٸ���Ҷ�任��Ⲵ�ɷ���
	void dst(const Mat& src, Mat& dest, bool invert = false);
	void idst(const Mat& src, Mat& dest);

	// ���룺imgΪ����ͼ��mod_diffΪɢ�ȣ�mod_diff��С������img����Χ�����ص�  
	// ����mod_diff�Ĵ�СΪ(w-2)*(h-2),����mod_diff����Χֵ(ɢ��)Ϊ0  
	// �����result  
	// ������Ⲵ�ɷ��̣�����dst��idst��
	void solve(const Mat &img, Mat& mod_diff, Mat &result);

	// ���ɷ������ ����ɢ��(laplacianX+laplacianY)�����߽�����ؼ����ؽ����   
	// ���룺imgΪ����ͼƬ��laplacianX+laplacianY Ϊɢ��  
	// �����result�ؽ����  
	void poissonSolver(const Mat &img, Mat &gxx, Mat &gyy, Mat &result);

	// ��Ŀ��ͼ����ݶ�mat���б任��ʹ֮������ԭͼ����ݶ�mat��ӣ�����ɢ��
	void arrayProduct(const Mat& lhs, const Mat& rhs, Mat& result) const;

	// ����x�����ݶȣ�һ��ƫ����
	void computeGradientX(const Mat &img, Mat &gx);

	// ����y�����ݶȣ�һ��ƫ����
	void computeGradientY(const Mat &img, Mat &gy);

	// ����x����ɢ�ȣ�����ƫ����
	void computeLaplacianX(const Mat &img, Mat &gxx);

	// ����x����ɢ�ȣ�����ƫ����
	void computeLaplacianY(const Mat &img, Mat &gyy);

	// ���в����ں�
	void clone(const Mat &destination, const Mat &patch, const Mat &binaryMask, Mat &cloned, int flag);

private:
	// ԭͼ���Ŀ��ͼ���Mat
	Mat img1_;
	Mat img2_;

	// output: ÿ��ͨ���ĺϳɽ������  
	// rbgx_channel, rgby_channel��gxx�� gyy ��ͨ�����  
	vector <Mat> rgbx_channel, rgby_channel, output;

	// binaryMaskFloat��ԭͼ���mask�� binaryMaskFloatInverted��binaryMaskFloatȡ���Ľ��  
	// destinationGradientX, destinationGradientY ��Ŀ��ͼ����ݶȣ�MaskFloatInverted������ݶȣ�
	// patchGradientX, patchGradientY ��ԭͼ����ݶȣ�binaryMaskFloat������ݶȣ�
	Mat destinationGradientX, destinationGradientY;
	Mat patchGradientX, patchGradientY;
	Mat binaryMaskFloat, binaryMaskFloatInverted;

	vector<float> filter_X, filter_Y;
};
