#ifndef HISTVIEW_H
#define HISTVIEW_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>

// --------------------------------------------------------------------------
class HistView : public QWidget
{
private:
	Q_OBJECT
	int *histogram;
	int min, max, step, nbuckets;
	QPen pen_bucket;
	QPen pen_ruler;
	QPen pen_mouse;
	QFont font;
	int mouse_x;
	int scale_y;

protected:
	void paintEvent(QPaintEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent* event);

public:
	explicit HistView(QWidget *parent = 0);
	~HistView();
	void setup(int mn, int mx, int st);
	void inc(int p);
	int get_mfp();

};

#endif

// vim: tabstop=4 shiftwidth=4 autoindent
