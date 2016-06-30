#include <QPainter>

#include "histview.h"

// --------------------------------------------------------------------------
HistView::HistView(QWidget *parent) : QWidget(parent)
{
	scale_y = 1;
	histogram = NULL;
	pen_bucket = QPen(QColor(200, 200 ,200), 1);
	pen_ruler = QPen(QColor(230, 230 ,230), 1);
	pen_mouse = QPen(QColor(255, 155, 179, 255));
	setMouseTracking(true);
}

// --------------------------------------------------------------------------
HistView::~HistView()
{
	delete[] histogram;
}

// --------------------------------------------------------------------------
void HistView::setup(int mn, int mx, int st)
{
	min = mn;
	max = mx;
	step = st;
	nbuckets = (max-min) / step;
	delete[] histogram;
	histogram = new int[nbuckets]();
}

// --------------------------------------------------------------------------
void HistView::inc(int p)
{
	if ((p > 0) &&(p < nbuckets)) {
		histogram[p]++;
	}
}

// --------------------------------------------------------------------------
int HistView::get_mfp()
{
	int mfp = 0;
	int maxcount = 0;

	if (!histogram) {
		return 0;
	}

	for (int i=0 ; i<nbuckets; i++) {
		if (histogram[i] > maxcount) {
			maxcount = histogram[i];
			mfp = i;
		}
	}
	return mfp;
}

// --------------------------------------------------------------------------
void HistView::mouseMoveEvent(QMouseEvent *event)
{
	mouse_x = QWidget::mapFromGlobal(QCursor::pos()).x();

	if (!histogram) {
		return;
	}

	update();
}

// --------------------------------------------------------------------------
void HistView::wheelEvent(QWheelEvent* event)
{
	// zoom/shrink
	if ((event->delta() < 0) && (scale_y < 1025)) {
		scale_y *= 2;
	} else if ((event->delta() > 0) && (scale_y >= 2)) {
		scale_y /= 2;
	}

	update();
}

// --------------------------------------------------------------------------
void HistView::paintEvent(QPaintEvent *event)
{
	QPainter painter;
	painter.begin(this);
	font.setPixelSize(10);
	painter.setFont(font);

	if (!histogram) {
		painter.end();
		return;
	}

	int rulerh = 22;
	int toph = histogram[get_mfp()];
	int winheight = geometry().height();
	int height = winheight - rulerh;
	int width = geometry().width();
	int lastbucket = -1;

	for (int p=0 ; p<width ; p++) {
		int bucket = p*max/width;
		painter.setPen(pen_bucket);
		double bheight = (double) scale_y * histogram[bucket] * height/toph;
		if (bheight > height) {
			bheight = height;
		}
		painter.drawLine(p, height, p, height - bheight);
		if (bucket != lastbucket) {
			if (bucket%50 == 0) {
				painter.setPen(pen_ruler);
				painter.drawLine(p, height+1, p, height+10);
				painter.drawText(p, height+11, 100, 10, Qt::AlignLeft, QString::number(bucket));
			} else if (bucket%10 == 0) {
				painter.setPen(pen_ruler);
				painter.drawLine(p, height+1, p, height+6);
			} else if (bucket%5 == 0) {
				painter.setPen(pen_ruler);
				painter.drawLine(p, height+1, p, height+2);
			}
			lastbucket = bucket;
		}
	}

	font.setBold(true);
	painter.setPen(pen_mouse);
	painter.drawLine(mouse_x, 0, mouse_x, geometry().height());
	painter.drawText(mouse_x+10, 10, 100, 10, Qt::AlignLeft, QString("%1 = %2").arg(mouse_x*max/width).arg(histogram[mouse_x*max/width]));

	painter.end();
}

// vim: tabstop=4 shiftwidth=4 autoindent
