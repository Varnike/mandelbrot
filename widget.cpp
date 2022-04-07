#include "widget.h"
#include "ui_widget.h"
#include <QPainter>
#include <math.h>
#include <QDebug>
#include <QImage>
#include <emmintrin.h>
#include <immintrin.h>
#include <QKeyEvent>
#include <QTime>

#define SSE_MODE

Widget::Widget(QWidget *parent)
	: QWidget(parent)
{
	setWindowTitle(tr("Mandelbrot"));
	resize(vidX, vidY);

	image = QImage(QSize(vidX, vidY), QImage::Format_RGB888);

	run();
}

Widget::~Widget()
{
}

void Widget::run()
{
	QTime time;
	time.start();

#ifdef  SSE_MODE
	mandl_sse();
#else
	mandl_nosse();
#endif


	repaint();
	double fps = max_test_cycles / ((double)time.elapsed()/1000.0);
	qDebug() << "FPS : " << fps << "SCALE : " << scale;
}

void Widget::mandl_sse()
{
	uchar *image_buff = image.bits();

	if (image_buff == nullptr)
		return;

	const __m256 r2_max= _mm256_set_ps(100.f, 100.f, 100.f, 100.f,
					    100.f, 100.f, 100.f, 100.f);
	const __m256 _255  = _mm256_set_ps(255.f, 255.f, 255.f, 255.f,
					   255.f, 255.f, 255.f, 255.f);
	const __m256 order = _mm256_set_ps(7.f, 6.f, 5.f, 4.f,
					   3.f, 2.f, 1.f, 0.f);
	const __m256 max_it= _mm256_set_ps(max_cycles, max_cycles, max_cycles, max_cycles,
					   max_cycles, max_cycles, max_cycles, max_cycles);

	for (int c_cnt = 0; c_cnt != max_test_cycles; c_cnt++) {
	for (int iy = 0; iy < vidY; iy++) {
		float x0 = ((          - (float)vidX/2) * dx) * scale + xC + ROI_X,
		      y0 = (((float)iy - (float)vidY/2) * dy) * scale + yC + ROI_Y;

		for (int ix = 0; ix < vidX; ix += 8, x0 += dx * 8 * scale) {
			__m256 dx_vec  = _mm256_set_ps(dx * scale, dx* scale, dx* scale, dx* scale,
							dx* scale, dx* scale, dx* scale, dx* scale);
			__m256 x0_vec  = _mm256_set_ps(x0, x0, x0, x0,
						       x0, x0, x0, x0);
			__m256 x_start = _mm256_add_ps(x0_vec, _mm256_mul_ps(order, dx_vec));
			__m256 y_start = _mm256_set_ps(y0, y0, y0, y0,
						       y0, y0, y0, y0);

			__m256 X = x_start, Y = y_start;
			__m256i pass_no = _mm256_setzero_si256();

			for (int n = 0; n < max_cycles; n++) {
				__m256 x2 = _mm256_mul_ps(X, X),
				       y2 = _mm256_mul_ps(Y, Y);

				__m256 r2 = _mm256_add_ps(x2, y2);
				__m256 cmp = _mm256_cmp_ps(r2, r2_max, _CMP_LE_OS);

				if ( !_mm256_movemask_ps(cmp))
					break;

				pass_no = _mm256_sub_epi32(pass_no, _mm256_castps_si256(cmp));

				__m256 xy = _mm256_mul_ps(X, Y);

				X = _mm256_add_ps(_mm256_sub_ps(x2, y2), x_start);
				Y = _mm256_add_ps(_mm256_add_ps(xy, xy), y_start);
			}

			__m256 I = _mm256_mul_ps(_mm256_sqrt_ps (_mm256_sqrt_ps (_mm256_div_ps (_mm256_cvtepi32_ps (pass_no), max_it))), _255);

			for (int i = 0; i < 8; i++) {
				int*   pn = (int*)   &pass_no;
				float* pI = (float*) &I;

				uchar color = pn[i];

				int flag = pI[i] < max_cycles;

				image_buff[iy * 3 * vidX + (ix + i) * 3] = color / 2 * flag;
				image_buff[iy * 3 * vidX + (ix + i) * 3 + 1] = color * flag;
				image_buff[iy * 3 * vidX + (ix + i) * 3 + 2] = color / 3 * flag;
			}
		}
	}
	}
}

void Widget::mandl_nosse()
{
	const float r2_max = 100.f;
	const int max_n = 256;

	uchar *image_buff = image.bits();

	for (int c_cnt = 0; c_cnt != max_test_cycles; c_cnt++) {


	for (int iy = 0; iy < vidY; iy++) {
		float x0 = ((          - (float)vidX/2) * dx) * scale + xC + ROI_X,
		      y0 = (((float)iy - (float)vidY/2) * dy) * scale + yC + ROI_Y;

		for (int ix = 0; ix < vidX; ix++, x0 += dx * scale) {

			float X = x0, Y = y0;
			int n = 0;

			for( ; n < max_cycles; n++) {
				float X2 = X * X;
				float Y2 = Y * Y;

				float r2 = X2 + Y2;

				if (r2 >= r2_max) break;


				Y = 2 * X * Y + y0;
				X = X2 - Y2 + x0;
			}
			float I = sqrtf (sqrtf ((float)n / (float)max_n)) * 255.f;

			int flag = n < max_n;

			image_buff[iy * vidX * 3 + ix * 3] = I / 2 * flag;
			image_buff[iy * vidX * 3 + ix * 3 + 1] = I / flag;
			image_buff[iy * vidX * 3 + ix * 3 + 2] = I / 3 * flag;
		}
	}
	}

}
void Widget::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);

	QRect target(0, 0, vidX, vidY);
	QPixmap pmap(vidX, vidY);
	pmap.convertFromImage(image);

	painter.drawPixmap(target, pmap, target);
}

void Widget::keyPressEvent(QKeyEvent *event)
{
	switch (event->key()) {
	case Qt::Key_Escape:
		break;
	case Qt::Key_Z:
		scale -= 0.0001;
		break;
	case Qt::Key_X:
		scale -= 0.00001;
		break;
	case Qt::Key_Plus:
		scale -= 0.05;
		break;
	case Qt::Key_Minus:
		scale += 0.05;
		break;
	case Qt::Key_Left:
		xC -= dx * scale * 10;
		break;
	case Qt::Key_Right:
		xC += dx * scale * 10;
		break;
	case Qt::Key_Down:
		yC += dy * scale * 10;
		break;
	case Qt::Key_Up:
		yC -= dy * scale * 10;
		break;
	default:
		QWidget::keyPressEvent(event);
		return;
	}

	run();
}
