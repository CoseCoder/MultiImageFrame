#include "PoissonImageEditing.h"

PoissonImageEditing::PoissonImageEditing()
{
}

PoissonImageEditing::PoissonImageEditing(const QImage & image1, QImage & image2)
{
	QImage2cvMat(image1, img1_);
	QImage2cvMat(image2, img2_);
}

PoissonImageEditing::~PoissonImageEditing()
{
}

QImage PoissonImageEditing::poissonBlending(QRect &ROI, int posX, int posY, int flag)
{
	Mat mask = Mat::zeros(img1_.size(), img1_.depth());
	Mat roimat = mask(Rect(ROI.x(), ROI.y(), ROI.width(), ROI.height()));
	roimat.setTo(255);
	Point p(posX, posY);

	Mat r(img2_.size(), CV_8UC3);

	int minx = INT_MAX, miny = INT_MAX, maxx = INT_MIN, maxy = INT_MIN;
	int h = mask.size().height;
	int w = mask.size().width;

	Mat gray = Mat(mask.size(), CV_8UC1);
	Mat dst_mask = Mat::zeros(img2_.size(), CV_8UC1);
	Mat cs_mask = Mat::zeros(img1_.size(), CV_8UC3);
	Mat cd_mask = Mat::zeros(img2_.size(), CV_8UC3);

	if (mask.channels() == 3)
		cv::cvtColor(mask, gray, COLOR_BGR2GRAY);
	else
		gray = mask;

	for (int i = 0; i < h; i++)
	{
		for (int j = 0; j < w; j++)
		{
			if (gray.at<uchar>(i, j) == 255)
			{
				minx = min(minx, i);
				maxx = max(maxx, i);
				miny = min(miny, j);
				maxy = max(maxy, j);
			}
		}
	}

	int lenx = maxx - minx;
	int leny = maxy - miny;

	Mat patch = Mat::zeros(Size(leny, lenx), CV_8UC3);

	int minxd = p.y;
	int maxxd = p.y + lenx;
	int minyd = p.x;
	int maxyd = p.x + leny;

	if (!(minxd >= 0 && minyd >= 0 && maxxd <= img2_.rows && maxyd <= img2_.cols))
		return cvMat2QImage(img2_);

	Rect roi_d(minyd, minxd, leny, lenx);
	Rect roi_s(miny, minx, leny, lenx);

	Mat destinationROI = dst_mask(roi_d);
	Mat sourceROI = cs_mask(roi_s);

	gray(roi_s).copyTo(destinationROI);
	img1_(roi_s).copyTo(sourceROI, gray(roi_s));
	img1_(roi_s).copyTo(patch, gray(roi_s));

	destinationROI = cd_mask(roi_d);
	cs_mask(roi_s).copyTo(destinationROI);

	clone(img2_, cd_mask, dst_mask, r, flag);

	return cvMat2QImage(r);
}

void PoissonImageEditing::QImage2cvMat(const QImage &image, Mat &img)
{
	Mat mat;
	switch (image.format())
	{
	case QImage::Format_ARGB32:
	case QImage::Format_ARGB32_Premultiplied:
	case QImage::Format_RGB32:
		mat = Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
		cv::cvtColor(mat, img, CV_BGRA2BGR);
		break;
	case QImage::Format_RGB888:
		mat = Mat(image.height(), image.width(), CV_8UC3, (void*)image.constBits(), image.bytesPerLine());
		cv::cvtColor(mat, img, CV_RGB2BGR);
		break;
	case QImage::Format_Grayscale8:
		img = Mat(image.height(), image.width(), CV_8UC1, (void*)image.constBits(), image.bytesPerLine());
		break;
	}
}

QImage PoissonImageEditing::cvMat2QImage(const Mat & mat)
{
	Mat rgb;
	QImage result;
	if (mat.channels() == 1)
	{
		cv::cvtColor(mat, rgb, CV_GRAY2RGB);
		result = QImage((const uchar*)(mat.data), mat.cols, mat.rows, mat.step,
			QImage::Format_Indexed8);
	}
	else
	{
		cv::cvtColor(mat, rgb, CV_BGR2RGB);
		// construct the QImage using the data of the mat, while do not copy the data  
		result = QImage((const uchar*)(rgb.data), rgb.cols, rgb.rows, rgb.step,//cols*rgb.channels(),
			QImage::Format_RGB888);
	}

	result.bits();
	return result;
}

void PoissonImageEditing::computeGradientX(const Mat &img, Mat &gx)
{
	Mat kernel = Mat::zeros(1, 3, CV_8S);
	kernel.at<char>(0, 2) = 1;
	kernel.at<char>(0, 1) = -1;

	if (img.channels() == 3)
	{
		filter2D(img, gx, CV_32F, kernel);
	}
	else if (img.channels() == 1)
	{
		Mat tmp[3];
		for (int chan = 0; chan < 3; ++chan)
		{
			filter2D(img, tmp[chan], CV_32F, kernel);
		}
		merge(tmp, 3, gx);
	}
}

void PoissonImageEditing::computeGradientY(const Mat &img, Mat &gy)
{
	Mat kernel = Mat::zeros(3, 1, CV_8S);
	kernel.at<char>(2, 0) = 1;
	kernel.at<char>(1, 0) = -1;

	if (img.channels() == 3)
	{
		filter2D(img, gy, CV_32F, kernel);
	}
	else if (img.channels() == 1)
	{
		Mat tmp[3];
		for (int chan = 0; chan < 3; ++chan)
		{
			filter2D(img, tmp[chan], CV_32F, kernel);
		}
		merge(tmp, 3, gy);
	}
}

void PoissonImageEditing::computeLaplacianX(const Mat &img, Mat &laplacianX)
{
	Mat kernel = Mat::zeros(1, 3, CV_8S);
	kernel.at<char>(0, 0) = -1;
	kernel.at<char>(0, 1) = 1;
	filter2D(img, laplacianX, CV_32F, kernel);
}

void PoissonImageEditing::computeLaplacianY(const Mat &img, Mat &laplacianY)
{
	Mat kernel = Mat::zeros(3, 1, CV_8S);
	kernel.at<char>(0, 0) = -1;
	kernel.at<char>(1, 0) = 1;
	filter2D(img, laplacianY, CV_32F, kernel);
}

void PoissonImageEditing::dst(const Mat& src, Mat& dest, bool invert)
{
	Mat temp = Mat::zeros(src.rows, 2 * src.cols + 2, CV_32F);

	int flag = invert ? DFT_ROWS + DFT_SCALE + DFT_INVERSE : DFT_ROWS;

	src.copyTo(temp(Rect(1, 0, src.cols, src.rows)));

	for (int j = 0; j < src.rows; ++j)
	{
		float * tempLinePtr = temp.ptr<float>(j);
		const float * srcLinePtr = src.ptr<float>(j);
		for (int i = 0; i < src.cols; ++i)
		{
			tempLinePtr[src.cols + 2 + i] = -srcLinePtr[src.cols - 1 - i];
		}
	}

	Mat planes[] = { temp, Mat::zeros(temp.size(), CV_32F) };
	Mat complex;

	merge(planes, 2, complex);
	dft(complex, complex, flag);
	split(complex, planes);
	temp = Mat::zeros(src.cols, 2 * src.rows + 2, CV_32F);

	for (int j = 0; j < src.cols; ++j)
	{
		float * tempLinePtr = temp.ptr<float>(j);
		for (int i = 0; i < src.rows; ++i)
		{
			float val = planes[1].ptr<float>(i)[j + 1];
			tempLinePtr[i + 1] = val;
			tempLinePtr[temp.cols - 1 - i] = -val;
		}
	}

	Mat planes2[] = { temp, Mat::zeros(temp.size(), CV_32F) };

	merge(planes2, 2, complex);
	dft(complex, complex, flag);
	split(complex, planes2);

	temp = planes2[1].t();
	dest = Mat::zeros(src.size(), CV_32F);
	temp(Rect(0, 1, src.cols, src.rows)).copyTo(dest);
}

void PoissonImageEditing::idst(const Mat& src, Mat& dest)
{
	dst(src, dest, true);
}

void PoissonImageEditing::solve(const Mat &img, Mat& mod_diff, Mat &result)
{
	const int w = img.cols;
	const int h = img.rows;

	Mat res;
	dst(mod_diff, res);

	for (int j = 0; j < h - 2; j++)
	{
		float * resLinePtr = res.ptr<float>(j);
		for (int i = 0; i < w - 2; i++)
		{
			resLinePtr[i] /= (filter_X[i] + filter_Y[j] - 4);
		}
	}

	idst(res, mod_diff);

	unsigned char *  resLinePtr = result.ptr<unsigned char>(0);
	const unsigned char * imgLinePtr = img.ptr<unsigned char>(0);
	const float * interpLinePtr = NULL;

	//first col
	for (int i = 0; i < w; ++i)
		result.ptr<unsigned char>(0)[i] = img.ptr<unsigned char>(0)[i];

	for (int j = 1; j < h - 1; ++j)
	{
		resLinePtr = result.ptr<unsigned char>(j);
		imgLinePtr = img.ptr<unsigned char>(j);
		interpLinePtr = mod_diff.ptr<float>(j - 1);

		//first row
		resLinePtr[0] = imgLinePtr[0];

		for (int i = 1; i < w - 1; ++i)
		{
			//saturate cast is not used here, because it behaves differently from the previous implementation
			//most notable, saturate_cast rounds before truncating, here it's the opposite.
			float value = interpLinePtr[i - 1];
			if (value < 0.)
				resLinePtr[i] = 0;
			else if (value > 255.0)
				resLinePtr[i] = 255;
			else
				resLinePtr[i] = static_cast<unsigned char>(value);
		}

		//last row
		resLinePtr[w - 1] = imgLinePtr[w - 1];
	}

	//last col
	resLinePtr = result.ptr<unsigned char>(h - 1);
	imgLinePtr = img.ptr<unsigned char>(h - 1);
	for (int i = 0; i < w; ++i)
		resLinePtr[i] = imgLinePtr[i];
}

void PoissonImageEditing::poissonSolver(const Mat &img, Mat &laplacianX, Mat &laplacianY, Mat &result)
{
	const int w = img.cols;
	const int h = img.rows;

	Mat lap = Mat(img.size(), CV_32FC1);

	lap = laplacianX + laplacianY;

	Mat bound = img.clone();

	rectangle(bound, Point(1, 1), Point(img.cols - 2, img.rows - 2), Scalar::all(0), -1);
	Mat boundary_points;
	Laplacian(bound, boundary_points, CV_32F);

	boundary_points = lap - boundary_points;

	Mat mod_diff = boundary_points(Rect(1, 1, w - 2, h - 2));

	solve(img, mod_diff, result);
}

void PoissonImageEditing::initVariables(const Mat &destination, const Mat &binaryMask)
{
	destinationGradientX = Mat(destination.size(), CV_32FC3);
	destinationGradientY = Mat(destination.size(), CV_32FC3);
	patchGradientX = Mat(destination.size(), CV_32FC3);
	patchGradientY = Mat(destination.size(), CV_32FC3);

	binaryMaskFloat = Mat(binaryMask.size(), CV_32FC1);
	binaryMaskFloatInverted = Mat(binaryMask.size(), CV_32FC1);

	//init of the filters used in the dst
	const int w = destination.cols;
	filter_X.resize(w - 2);
	for (int i = 0; i < w - 2; ++i)
		filter_X[i] = 2.0f * cos(static_cast<float>(CV_PI) * (i + 1) / (w - 1));

	const int h = destination.rows;
	filter_Y.resize(h - 2);
	for (int j = 0; j < h - 2; ++j)
		filter_Y[j] = 2.0f * cos(static_cast<float>(CV_PI) * (j + 1) / (h - 1));
}

void PoissonImageEditing::computeDerivatives(const Mat& destination, const Mat &patch, const Mat &binaryMask)
{
	initVariables(destination, binaryMask);

	computeGradientX(destination, destinationGradientX);
	computeGradientY(destination, destinationGradientY);

	computeGradientX(patch, patchGradientX);
	computeGradientY(patch, patchGradientY);

	Mat Kernel(Size(3, 3), CV_8UC1);
	Kernel.setTo(Scalar(1));
	erode(binaryMask, binaryMask, Kernel, Point(-1, -1), 3);

	binaryMask.convertTo(binaryMaskFloat, CV_32FC1, 1.0 / 255.0);
}

void PoissonImageEditing::scalarProduct(Mat mat, float r, float g, float b)
{
	vector <Mat> channels;
	split(mat, channels);
	multiply(channels[0], b, channels[0]);
	multiply(channels[1], g, channels[1]);
	multiply(channels[2], r, channels[2]);
	merge(channels, mat);
}

void PoissonImageEditing::arrayProduct(const Mat& lhs, const Mat& rhs, Mat& result) const
{
	vector <Mat> lhs_channels;
	vector <Mat> result_channels;

	split(lhs, lhs_channels);
	split(result, result_channels);

	for (int chan = 0; chan < 3; ++chan)
	{
		multiply(lhs_channels[chan], rhs, result_channels[chan]);
	}

	merge(result_channels, result);
}

void PoissonImageEditing::poisson(const Mat &destination)
{
	Mat laplacianX = Mat(destination.size(), CV_32FC3);
	Mat laplacianY = Mat(destination.size(), CV_32FC3);

	laplacianX = destinationGradientX + patchGradientX;
	laplacianY = destinationGradientY + patchGradientY;

	computeLaplacianX(laplacianX, laplacianX);
	computeLaplacianY(laplacianY, laplacianY);

	split(laplacianX, rgbx_channel);
	split(laplacianY, rgby_channel);

	split(destination, output);

	for (int chan = 0; chan < 3; ++chan)
	{
		poissonSolver(output[chan], rgbx_channel[chan], rgby_channel[chan], output[chan]);
	}
}

void PoissonImageEditing::evaluate(const Mat &I, const Mat &wmask, const Mat &cloned)
{
	bitwise_not(wmask, wmask);

	wmask.convertTo(binaryMaskFloatInverted, CV_32FC1, 1.0 / 255.0);

	arrayProduct(destinationGradientX, binaryMaskFloatInverted, destinationGradientX);
	arrayProduct(destinationGradientY, binaryMaskFloatInverted, destinationGradientY);

	poisson(I);

	merge(output, cloned);
}

void PoissonImageEditing::clone(const Mat &destination, const Mat &patch, const Mat &binaryMask, Mat &cloned, int flag)
{
	const int w = destination.cols;
	const int h = destination.rows;
	const int channel = destination.channels();
	const int n_elem_in_line = w * channel;

	// 计算destination在x，y方向的梯度，获得结果为：destinationGradientX destinationGradientY  
	// 计算patch在x，y方向的梯度，获得结果为： patchGradientX patchGradientY  
	// 同时对binaryMask进行边界腐蚀，去除毛刺，让边界变得光滑一点，同时把binaryMask归一化为0~1得binaryMaskFloat  
	// 归一化后binaryMaskFloat只有数值0,1       1代表被拷贝区域的像素点  
	computeDerivatives(destination, patch, binaryMask);

	switch (flag)
	{
	case Mode::NORMAL_CLONE:
		arrayProduct(patchGradientX, binaryMaskFloat, patchGradientX);
		arrayProduct(patchGradientY, binaryMaskFloat, patchGradientY);
		break;

	case Mode::MIXED_CLONE:
	{
		AutoBuffer<int> maskIndices(n_elem_in_line);
		for (int i = 0; i < n_elem_in_line; ++i)
			maskIndices[i] = i / channel;

		for (int i = 0; i < h; i++)
		{
			float * patchXLinePtr = patchGradientX.ptr<float>(i);
			float * patchYLinePtr = patchGradientY.ptr<float>(i);
			const float * destinationXLinePtr = destinationGradientX.ptr<float>(i);
			const float * destinationYLinePtr = destinationGradientY.ptr<float>(i);
			const float * binaryMaskLinePtr = binaryMaskFloat.ptr<float>(i);

			for (int j = 0; j < n_elem_in_line; j++)
			{
				int maskIndex = maskIndices[j];

				if (abs(patchXLinePtr[j] - patchYLinePtr[j]) >
					abs(destinationXLinePtr[j] - destinationYLinePtr[j]))
				{
					patchXLinePtr[j] *= binaryMaskLinePtr[maskIndex];
					patchYLinePtr[j] *= binaryMaskLinePtr[maskIndex];
				}
				else
				{
					patchXLinePtr[j] = destinationXLinePtr[j]
						* binaryMaskLinePtr[maskIndex];
					patchYLinePtr[j] = destinationYLinePtr[j]
						* binaryMaskLinePtr[maskIndex];
				}
			}
		}
	}
	break;
	}

	evaluate(destination, binaryMask, cloned);
}