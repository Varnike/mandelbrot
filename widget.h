#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

class Widget : public QWidget
{
	Q_OBJECT

public:
	Widget(QWidget *parent = nullptr);
	~Widget();

	void run();
	void mandl_sse();
	void mandl_nosse();

protected:
	void paintEvent(QPaintEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;

signals:
	void img_rendered();

private:
	const int vidX = 800;
	const int vidY = 600;

	const int max_test_cycles = 100;

	const float  dx    = 1/800.f, dy = 1/800.f;
	const float ROI_X = -1.325f, ROI_Y = 0.f;
	const int max_cycles  = 256;

	float xC = 0.f, yC = 0.f, scale = 1.f;
	QImage image; //(QSize(800, 600), QImage::Format_ARGB32);

	bool draw_flag = 0;
};
#endif // WIDGET_H
