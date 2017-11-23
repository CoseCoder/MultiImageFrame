#include "ImageWidget.h"
#include <QImage>
#include <QPainter>
#include <QtWidgets> 
#include <iostream>
#include "ChildWindow.h"
#include "PoissonImageEditing.h"

using std::cout;
using std::endl;

ImageWidget::ImageWidget(ChildWindow *relateWindow)
{
	image_ = new QImage();
	image_backup_ = new QImage();
	image_temp_ = new QImage();

	draw_status_ = kNone;
	is_choosing_ = false;
	is_pasting_ = false;
	is_poisson_ = false;
	is_mixpoisson_ = false;

	point_start_ = QPoint(0, 0);
	point_end_ = QPoint(0, 0);

	source_window_ = NULL;
}

ImageWidget::~ImageWidget(void)
{
	delete image_, image_backup_, image_temp_;
}

int ImageWidget::ImageWidth()
{
	return image_->width();
}

int ImageWidget::ImageHeight()
{
	return image_->height();
}

void ImageWidget::set_draw_status_to_choose()
{
	draw_status_ = kChoose;
}

void ImageWidget::set_draw_status_to_paste()
{
	draw_status_ = kPaste;
}

void ImageWidget::set_draw_status_to_poisson()
{
	draw_status_ = kPoisson;
}

void ImageWidget::set_draw_status_to_mixPoisson()
{
	draw_status_ = kMixPoisson;
}

QImage* ImageWidget::image()
{
	return image_;
}

void ImageWidget::set_source_window(ChildWindow* childWindow)
{
	source_window_ = childWindow;
}

void ImageWidget::paintEvent(QPaintEvent *paintEvent)
{
	QPainter painter;
	painter.begin(this);

	// Draw background
	painter.setBrush(Qt::lightGray);
	QRect back_rect(0, 0, width(), height());
	painter.drawRect(back_rect);

	// Draw image
	QRect rect = QRect(0, 0, image_->width(), image_->height());
	painter.drawImage(rect, *image_);

	// Draw choose region
	painter.setBrush(Qt::NoBrush);
	painter.setPen(Qt::red);
	painter.drawRect(point_start_.x(), point_start_.y(),
		point_end_.x() - point_start_.x(), point_end_.y() - point_start_.y());

	painter.end();
}

void ImageWidget::mousePressEvent(QMouseEvent *mouseEvent)
{
	if (Qt::LeftButton == mouseEvent->button())
	{
		switch (draw_status_)
		{
		case kChoose:
			is_choosing_ = true;
			point_start_ = point_end_ = mouseEvent->pos();
			break;

		case kPaste:
		{
			if (source_window_ == NULL)
				break;

			is_pasting_ = true;

			// Start point in object image
			int xpos = mouseEvent->pos().rx();
			int ypos = mouseEvent->pos().ry();

			// Start point in source image
			int xsourcepos = source_window_->imagewidget_->point_start_.rx();
			int ysourcepos = source_window_->imagewidget_->point_start_.ry();

			// Width and Height of rectangle region
			int w = source_window_->imagewidget_->point_end_.rx()
				- source_window_->imagewidget_->point_start_.rx() + 1;
			int h = source_window_->imagewidget_->point_end_.ry()
				- source_window_->imagewidget_->point_start_.ry() + 1;

			if (w < 0) {
				xsourcepos += w;
				w = -w;
			}
			if (h < 0) {
				ysourcepos += h;
				h = -h;
			}

			// Paste
			if ((xpos + w < image_->width()) && (ypos + h < image_->height()))
			{
				// Restore image
			//	*(image_) = *(image_backup_);
				// Paste
				for (int i = 0; i < w; i++)
				{
					for (int j = 0; j < h; j++)
					{
						image_->setPixel(xpos + i, ypos + j, source_window_->imagewidget_->image()->pixel(xsourcepos + i, ysourcepos + j));
					}
				}
			}
			break;
		}

		case kPoisson:
		case kMixPoisson:
		{
			if (source_window_ == NULL)
				break;

			is_poisson_ = true;

			// Start point in object image
			int xpos = mouseEvent->pos().rx();
			int ypos = mouseEvent->pos().ry();

			// Start point in source image
			int xsourcepos = source_window_->imagewidget_->point_start_.rx();
			int ysourcepos = source_window_->imagewidget_->point_start_.ry();

			// Width and Height of rectangle region
			int w = source_window_->imagewidget_->point_end_.rx()
				- source_window_->imagewidget_->point_start_.rx() + 1;
			int h = source_window_->imagewidget_->point_end_.ry()
				- source_window_->imagewidget_->point_start_.ry() + 1;

			if (w < 0) {
				xsourcepos += w;
				w = -w;
			}
			if (h < 0) {
				ysourcepos += h;
				h = -h;
			}

			if ((xpos + w < image_->width()) && (ypos + h < image_->height()))
			{
				PoissonImageEditing p(*source_window_->imagewidget_->image(), *image_);
				QImage t;
				if (kPoisson == draw_status_)
					t = p.poissonBlending(QRect(xsourcepos, ysourcepos, w, h), xpos, ypos, Mode::NORMAL_CLONE);
				else if (kMixPoisson == draw_status_)
					t = p.poissonBlending(QRect(xsourcepos, ysourcepos, w, h), xpos, ypos, Mode::MIXED_CLONE);
				*image_ = t;
			}

			update();
			break;
		}

		default:
			break;
		}
	}
}

void ImageWidget::mouseMoveEvent(QMouseEvent *mouseEvent)
{
	switch (draw_status_)
	{
	case kChoose:
		// Store point position for rectangle region
		if (is_choosing_)
		{
			point_end_ = mouseEvent->pos();
		}
		break;

	case kPaste:
		// Paste rectangle region to object image
		if (is_pasting_)
		{
			// Start point in object image
			int xpos = mouseEvent->pos().rx();
			int ypos = mouseEvent->pos().ry();

			// Start point in source image
			int xsourcepos = source_window_->imagewidget_->point_start_.rx();
			int ysourcepos = source_window_->imagewidget_->point_start_.ry();

			// Width and Height of rectangle region
			int w = source_window_->imagewidget_->point_end_.rx()
				- source_window_->imagewidget_->point_start_.rx() + 1;
			int h = source_window_->imagewidget_->point_end_.ry()
				- source_window_->imagewidget_->point_start_.ry() + 1;

			if (w < 0) {
				xsourcepos += w;
				w = -w;
			}
			if (h < 0) {
				ysourcepos += h;
				h = -h;
			}

			// Paste
			if (((xpos > 0) || (ypos > 0)) && ((xpos + w < image_->width()) || (ypos + h < image_->height())))
			{
				// Restore image 
				*(image_) = *(image_temp_);

				if (xpos <= 0)
					xpos = 1;
				else if (ypos <= 0)
					ypos = 1;

				if (xpos + w >= image_->width())
					xpos = image_->width() - w - 1;
				else if (ypos + h >= image_->height())
					ypos = height() - h - 1;

				// Paste
				for (int i = 0; i < w; i++)
				{
					for (int j = 0; j < h; j++)
					{
						image_->setPixel(xpos + i, ypos + j, source_window_->imagewidget_->image()->pixel(xsourcepos + i, ysourcepos + j));
					}
				}
			}
		}
		break;

	case kPoisson:
	case kMixPoisson:
		if (is_poisson_ || is_mixpoisson_)
		{
			// Start point in object image
			int xpos = mouseEvent->pos().rx();
			int ypos = mouseEvent->pos().ry();

			// Start point in source image
			int xsourcepos = source_window_->imagewidget_->point_start_.rx();
			int ysourcepos = source_window_->imagewidget_->point_start_.ry();

			// Width and Height of rectangle region
			int w = source_window_->imagewidget_->point_end_.rx()
				- source_window_->imagewidget_->point_start_.rx() + 1;
			int h = source_window_->imagewidget_->point_end_.ry()
				- source_window_->imagewidget_->point_start_.ry() + 1;


			if (w < 0) {
				xsourcepos += w;
				w = -w;
			}
			if (h < 0) {
				ysourcepos += h;
				h = -h;
			}

			if (((xpos > 0) || (ypos > 0)) && ((xpos + w < image_->width()) || (ypos + h < image_->height())))
			{
				// Restore image 
				*(image_) = *(image_temp_);

				if (xpos <= 0)
					xpos = 1;
				else if (ypos <= 0)
					ypos = 1;

				if (xpos + w >= image_->width())
					xpos = image_->width() - w - 1;
				else if (ypos + h >= image_->height())
					ypos = height() - h - 1;


				PoissonImageEditing p(*source_window_->imagewidget_->image(), *image_);
				QImage t;
				if (kPoisson == draw_status_)
					t = p.poissonBlending(QRect(xsourcepos, ysourcepos, w, h), xpos, ypos, Mode::NORMAL_CLONE);
				else if (kMixPoisson == draw_status_)
					t = p.poissonBlending(QRect(xsourcepos, ysourcepos, w, h), xpos, ypos, Mode::MIXED_CLONE);
				*image_ = t;
			}

			update();
			break;
		}
	default:
		break;
	}

	update();
}

void ImageWidget::mouseReleaseEvent(QMouseEvent *mouseEvent)
{
	switch (draw_status_)
	{
	case kChoose:
		if (is_choosing_)
		{
			point_end_ = mouseEvent->pos();
			is_choosing_ = false;
		}

	case kPaste:
		if (is_pasting_)
		{
			is_pasting_ = false;
		}

	case kPoisson:
		if (is_poisson_)
		{
			is_poisson_ = false;
		}

	case kMixPoisson:
		if (is_mixpoisson_)
		{
			is_mixpoisson_ = false;
		}
	default:
		break;
	}
	draw_status_ = kNone;
	*(image_temp_) = *(image_);
	update();
}

void ImageWidget::Open(QString fileName)
{
	// Load file
	if (!fileName.isEmpty())
	{
		image_->load(fileName);
		*(image_backup_) = *(image_);
		*(image_temp_) = *(image_);
	}

	//	setFixedSize(image_->width(), image_->height());
	//	relate_window_->setWindowFlags(Qt::Dialog);
	//	relate_window_->setFixedSize(QSize(image_->width(), image_->height()));
	//	relate_window_->setWindowFlags(Qt::SubWindow);

		//image_->invertPixels(QImage::InvertRgb);
		//*(image_) = image_->mirrored(true, true);
		//*(image_) = image_->rgbSwapped();
	cout << "image size: " << image_->width() << ' ' << image_->height() << endl;

	update();
}

void ImageWidget::Save()
{
	SaveAs();
}

void ImageWidget::SaveAs()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image"), ".", tr("Images(*.bmp *.png *.jpg)"));
	if (fileName.isNull())
	{
		return;
	}

	image_->save(fileName);
}

void ImageWidget::Invert()
{
	for (int i = 0; i < image_->width(); i++)
	{
		for (int j = 0; j < image_->height(); j++)
		{
			QRgb color = image_->pixel(i, j);
			image_->setPixel(i, j, qRgb(255 - qRed(color), 255 - qGreen(color), 255 - qBlue(color)));
		}
	}

	// equivalent member function of class QImage
	// image_->invertPixels(QImage::InvertRgb);
	update();
}

void ImageWidget::Mirror(bool ishorizontal, bool isvertical)
{
	QImage image_tmp(*(image_));
	int width = image_->width();
	int height = image_->height();

	if (ishorizontal)
	{
		if (isvertical)
		{
			for (int i = 0; i < width; i++)
			{
				for (int j = 0; j < height; j++)
				{
					image_->setPixel(i, j, image_tmp.pixel(width - 1 - i, height - 1 - j));
				}
			}
		}
		else
		{
			for (int i = 0; i < width; i++)
			{
				for (int j = 0; j < height; j++)
				{
					image_->setPixel(i, j, image_tmp.pixel(i, height - 1 - j));
				}
			}
		}

	}
	else
	{
		if (isvertical)
		{
			for (int i = 0; i < width; i++)
			{
				for (int j = 0; j < height; j++)
				{
					image_->setPixel(i, j, image_tmp.pixel(width - 1 - i, j));
				}
			}
		}
	}

	// equivalent member function of class QImage
	//*(image_) = image_->mirrored(true, true);
	update();
}

void ImageWidget::TurnGray()
{
	for (int i = 0; i < image_->width(); i++)
	{
		for (int j = 0; j < image_->height(); j++)
		{
			QRgb color = image_->pixel(i, j);
			int gray_value = (qRed(color) + qGreen(color) + qBlue(color)) / 3;
			image_->setPixel(i, j, qRgb(gray_value, gray_value, gray_value));
		}
	}

	update();
}

void ImageWidget::Restore()
{
	*(image_) = *(image_backup_);
	*(image_temp_) = *(image_backup_);
	point_start_ = point_end_ = QPoint(0, 0);
	update();
}
