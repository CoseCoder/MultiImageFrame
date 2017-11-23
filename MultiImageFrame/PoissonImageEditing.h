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
	// flag: 选择poisson融合的方式
	QImage poissonBlending(QRect &ROI, int posX, int posY, int flag);

private:
	void QImage2cvMat(const QImage &image, Mat &img);
	QImage cvMat2QImage(const Mat& mat);

	// 初始化变量
	void initVariables(const Mat &destination, const Mat &binaryMask);

	// binaryMask为图像destination待修改的区域的mask
	void computeDerivatives(const Mat &destination, const Mat &patch, const Mat &binaryMask);
	void scalarProduct(Mat mat, float r, float g, float b);

	//泊松重建 
	void poisson(const Mat &destination);

	// 输入：I背景整张图片  wmask背景中待修改的区域的mask  
	// 输出：cloned 
	void evaluate(const Mat &I, const Mat &wmask, const Mat &cloned);

	// 利用快速傅里叶变换求解泊松方程
	void dst(const Mat& src, Mat& dest, bool invert = false);
	void idst(const Mat& src, Mat& dest);

	// 输入：img为背景图像、mod_diff为散度，mod_diff大小不包含img最外围的像素点  
	// 矩阵mod_diff的大小为(w-2)*(h-2),并且mod_diff最外围值(散度)为0  
	// 输出：result  
	// 快速求解泊松方程（调用dst和idst）
	void solve(const Mat &img, Mat& mod_diff, Mat &result);

	// 泊松方程求解 输入散度(laplacianX+laplacianY)，及边界点像素即可重建求解   
	// 输入：img为背景图片，laplacianX+laplacianY 为散度  
	// 输出：result重建结果  
	void poissonSolver(const Mat &img, Mat &gxx, Mat &gyy, Mat &result);

	// 将目标图像的梯度mat进行变换，使之可以与原图像的梯度mat相加，计算散度
	void arrayProduct(const Mat& lhs, const Mat& rhs, Mat& result) const;

	// 计算x方向梯度（一阶偏导）
	void computeGradientX(const Mat &img, Mat &gx);

	// 计算y方向梯度（一阶偏导）
	void computeGradientY(const Mat &img, Mat &gy);

	// 计算x方向散度（二阶偏导）
	void computeLaplacianX(const Mat &img, Mat &gxx);

	// 计算x方向散度（二阶偏导）
	void computeLaplacianY(const Mat &img, Mat &gyy);

	// 进行泊松融合
	void clone(const Mat &destination, const Mat &patch, const Mat &binaryMask, Mat &cloned, int flag);

private:
	// 原图像和目标图像的Mat
	Mat img1_;
	Mat img2_;

	// output: 每个通道的合成结果数组  
	// rbgx_channel, rgby_channel是gxx， gyy 分通道结果  
	vector <Mat> rgbx_channel, rgby_channel, output;

	// binaryMaskFloat是原图像的mask， binaryMaskFloatInverted是binaryMaskFloat取反的结果  
	// destinationGradientX, destinationGradientY 是目标图像的梯度（MaskFloatInverted区域的梯度）
	// patchGradientX, patchGradientY 是原图像的梯度（binaryMaskFloat区域的梯度）
	Mat destinationGradientX, destinationGradientY;
	Mat patchGradientX, patchGradientY;
	Mat binaryMaskFloat, binaryMaskFloatInverted;

	vector<float> filter_X, filter_Y;
};
