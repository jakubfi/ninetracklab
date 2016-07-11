#include <cmath>

#include <QPainter>
#include <QString>
#include <QWidget>
#include <QLinearGradient>
#include <QTextDocument>
#include <QDebug>
#include <QApplication>
#include <QTime>

#include "tapeview.h"

// --------------------------------------------------------------------------
TapeView::TapeView(QWidget *parent) : QWidget(parent)
{
	td = NULL;
	offset = 0;
	scale = 1;
	ruler_height = 20;
	zooming = 0;
	measuring = 0;
	disp_edges = 1;
	disp_mouse = 1;
	disp_regions = 1;
	disp_signals = 1;
	disp_events = 1;
	disp_bytes = 0;
	in_edit = 0;

	setMouseTracking(true);

	pen_wave = QPen(QColor(255, 255 ,255), 1);
	pen_edge = QPen(QColor(255, 255, 255, 60), 1);
	pen_tick = QPen(QColor(230, 230 ,230), 1);
	brush_bg = QBrush(QColor(50, 50, 50));
	pen_bg = QPen(QColor(50, 50, 50), 1);
	brush_bg_ruler = QBrush(QColor(90, 90, 90));
	pen_trackname = QPen(QColor(252, 255, 179, 255));
	pen_mouse = QPen(QColor(255, 200, 200, 255));

	pen_zoom = QPen(QColor(230, 230, 230), 1);
	brush_zoom = QBrush(QColor(230, 230, 200, 80));

	pen_block = QPen(QColor(255, 255, 255, 0), 1);
	brush_block = QBrush(QColor(255, 255, 255, 50));
	pen_event = QPen(QColor(255, 255, 255, 120), 1);
	pen_measure = QPen(QColor(255, 255, 235, 255), 1);
	brush_measure = QBrush(QColor(255, 255, 235, 255));
	brush_mark = QBrush(QColor(100, 100, 225, 50));
}

// --------------------------------------------------------------------------
void TapeView::correctView()
{
	// left sample can't be below tape start
	if (offset < 0) {
		offset = 0;
	}

	// right sample can't be after tape end
	int missing_right = rightSample()-td->tape_len();
	if (missing_right > 0) {
		// can we scroll right?
		if (missing_right <= offset) {
			offset -= missing_right;
		// we can't scroll
		} else {
			zoomAll();
		}
	}

	if (scale <= 1) {
		emit editable(true);
	} else {
		emit editable(false);
	}
}

// --------------------------------------------------------------------------
void TapeView::zoomRegion(int left, int right)
{
	if (!td || !td->tape_loaded()) return;

	offset = left;
	scale = (double) (right-left) / geometry().width();
	correctView();
	update();
}

// --------------------------------------------------------------------------
void TapeView::zoomAround(int pos, double scale)
{
	if (!td || !td->tape_loaded()) return;

	this->scale = scale;
	offset = pos - toSampleLen(mouse_pos.x());
	correctView();
	update();
}

// --------------------------------------------------------------------------
void TapeView::zoom11()
{
	zoomAround(leftSample()+(rightSample()-leftSample())/2, 1);
}

// --------------------------------------------------------------------------
void TapeView::zoomAll()
{
	zoomRegion(0, td->tape_len());
}

// --------------------------------------------------------------------------
void TapeView::zoomIn()
{
	zoomAround(leftSample()+(rightSample()-leftSample())/2, scale*0.5);
}

// --------------------------------------------------------------------------
void TapeView::zoomOut()
{
	zoomAround(leftSample()+(rightSample()-leftSample())/2, scale*2);
}

// --------------------------------------------------------------------------
void TapeView::wheelEvent(QWheelEvent* e)
{
	if (!td || !td->tape_loaded()) return;
	if (in_edit) return;

	if (e->delta() < 0) {
		zoomAround(toSample(mouse_pos.x()), scale*2);
	} else {
		zoomAround(toSample(mouse_pos.x()), scale*0.5);
	}
}

// --------------------------------------------------------------------------
void TapeView::signalEdit(QPoint from, QPoint pos)
{
	quint16 *data = td->tape_data();
	int ch_height = (geometry().height()-ruler_height) / 9;

	int ch = (pos.y() - ruler_height) / ch_height;
	int v = (2*(pos.y() - ruler_height) / ch_height) % 2;
	int sample = toSample(pos.x());

	if (ch<0) return;

	for (int p=toSample(from.x()) ; p<=sample ; p++) {
		if (!v) {
			data[p] |= 1 << ch;
		} else {
			data[p] &= ~(1 << ch);
		}
	}

	update();
}

// --------------------------------------------------------------------------
void TapeView::mousePressEvent(QMouseEvent *event)
{
	if (!td || !td->tape_loaded()) return;

	if (in_edit) {
		mouse_edit_start = event->pos();
		signalEdit(mouse_edit_start, event->pos());
		return;
	}

	if (QApplication::queryKeyboardModifiers() & Qt::ControlModifier) {
		mouse_measure_start = event->pos();
		measuring = 1;
	} else {
		if (event->buttons() & Qt::LeftButton) {
			mouse_drag_start = event->pos();
			mouse_drag_pixels = 0;
		} else if (event->buttons() & Qt::RightButton) {
			mouse_zoom_start = mouse_zoom_end = event->pos();
			zooming = 1;
		}
	}
}

// --------------------------------------------------------------------------
void TapeView::mouseReleaseEvent(QMouseEvent *event)
{
	if (!td || !td->tape_loaded()) return;
	if (in_edit) return;

	if (!(event->buttons() & Qt::LeftButton)) {
		mouse_drag_pixels = 0;
		if (measuring) {
			measuring = 0;
			update();
		}
	}

	if (zooming && !(event->buttons() & Qt::RightButton)) {
		int b = mouse_zoom_start.x();
		int e = mouse_zoom_end.x();
		if (b < 0) b = 0;
		if (e < 0) e = 0;
		if (b > e) {
			int s;
			s = b;
			b = e;
			e = s;
		}
		zoomRegion(toSample(b), toSample(e));
		mouse_zoom_start = mouse_zoom_end;
		zooming = 0;
		update();
	}
}

// --------------------------------------------------------------------------
void TapeView::wave_zoom(QMouseEvent *event)
{
	mouse_zoom_end = event->pos();
}

// --------------------------------------------------------------------------
void TapeView::wave_drag(QMouseEvent *event)
{
	offset += toSampleLen(mouse_drag_pixels);
	mouse_drag_pixels = mouse_pos.x() - mouse_drag_start.x();
	int delta = toSampleLen(mouse_drag_pixels);
	offset -= delta;

	// don't allow moving the tape outside its length
	if (rightSample() > td->tape_len()) {
		offset = td->tape_len() - viewSamples();
	}
	if (offset < 0) {
		offset = 0;
	}
}

// --------------------------------------------------------------------------
void TapeView::mouseMoveEvent(QMouseEvent *event)
{
	if (!td || !td->tape_loaded()) return;

	if (in_edit) {
		if (event->buttons() & Qt::LeftButton) {
			signalEdit(mouse_edit_start, event->pos());
			mouse_edit_start = event->pos();
		}
		return;
	}
	mouse_pos = event->pos();

	if (measuring) {
	} else if (event->buttons() & Qt::LeftButton) {
		wave_drag(event);
	} else if (event->buttons() & Qt::RightButton) {
		wave_zoom(event);
	}

	update();
}

// --------------------------------------------------------------------------
void TapeView::drawRuler(QPainter &painter)
{
	painter.fillRect(0, 0, rect().width(), ruler_height, brush_bg_ruler);
	painter.setPen(pen_tick);
	font.setPixelSize(9);
	painter.setFont(font);
	font.setBold(0);

	QString str;

	int min = leftSample();
	int max = rightSample();
	int region = max - min;
	int decpos = QString::number(region).length() - 1;
	int tick_l = pow(10, decpos);
	int tick_s = tick_l / 10;
	int width = rect().width();
	int lastpos = -1;

	for (int i=0 ; i<width ; i++) {
		int sample_pos = toSample(i);
		if (sample_pos == lastpos) continue; // don't draw in place
		lastpos = sample_pos;
		if (sample_pos % tick_l < scale) {
			str = QString::number(sample_pos, 'd', 0);
			if (sample_pos < 10000) {
				str = QString::number(sample_pos, 'd', 0);
			} else if (sample_pos < 1000000) {
				str = QString::number(sample_pos/1000, 'd', 0) + 'k';
			} else {
				str = QString::number(sample_pos/1000000, 'd', 0) + 'M';
			}
			painter.drawText(i-3, 1, 100, 10, Qt::AlignLeft, str);
			painter.drawLine(i, ruler_height-8, i, ruler_height-1);
		} else if ((tick_s > 0) && (sample_pos % tick_s < scale)) {
			painter.drawLine(i, ruler_height*0.90, i, ruler_height-1);
		}
	}
}

// --------------------------------------------------------------------------
void TapeView::drawTracks(QPainter &painter, int ch_height)
{
	int wv_height = ch_height*0.6;
	int ch_spacing = (ch_height - wv_height) / 2;
	int view_height = geometry().height();
	int view_width = geometry().width();

	EdgeSens es = td->cfg.edge_sens;

	//QTime myTimer;
	//myTimer.start();

	for (int x=0 ; x<view_width ; x++) {

		int sample_first = toSample(x);
		int sample_last = toSample(x+1);

		// scan sample range beneath current x position for edges
		quint16 changed_edges = 0;
		quint16 sensed_edges = 0;
		int d=sample_first;
		int e0, e1;
		e1 = td->peek(d);
		while (d<sample_last) {
			e0 = e1;
			d++;
			e1 = td->peek(d);
			int delta = e0 ^ e1;
			changed_edges |= delta;
			if (es == EDGE_ANY) {
				sensed_edges |= delta;
			} else if (es == EDGE_RISING) {
				sensed_edges |= delta & e1;
			} else if (es == EDGE_FALLING) {
				sensed_edges |= delta & e0;
			}
			if (changed_edges == 0b111111111) {
				break;
			}
		};

		// draw signal
		if (disp_signals) {
			painter.setPen(pen_wave);
			for (int ch=0 ; ch<9 ; ch++) {
				int ch_base_u = ruler_height + (ch+1) * ch_height - ch_spacing;
				if ((changed_edges >> ch) & 1) {
					painter.drawLine(x, ch_base_u - wv_height, x, ch_base_u);
				} else {
					painter.drawPoint(x, ch_base_u - ((e1>>ch)&1)*wv_height);
				}
			}
		}

		// draw edges
		if (disp_edges) {
			if (sensed_edges) {
				painter.setPen(pen_edge);
				painter.drawLine(x, ruler_height, x, view_height);
			}
		}
	}
	//int nMilliseconds = myTimer.elapsed();
	//qDebug() << nMilliseconds;
}

// --------------------------------------------------------------------------
void TapeView::drawTrackNames(QPainter &painter, int ch_height)
{
	int text_height = ch_height/2.5;
	int text_offset = (ch_height - text_height) / 2;

	font.setPixelSize(text_height);
	font.setBold(1);
	painter.setFont(font);
	painter.setPen(pen_trackname);

	const char *track_names[] = {"0", "1", "2", "3", "4", "5", "6", "7", "P"};
	for (int i=0 ; i<9 ; i++) {
		int text_pos = i*ch_height + text_offset;
		painter.drawText(10, ruler_height + text_pos, 300, ch_height, Qt::AlignLeft, track_names[i]);
	}
}

// --------------------------------------------------------------------------
void TapeView::drawMouse(QPainter &painter)
{
	painter.setPen(pen_mouse);

	painter.drawLine(mouse_pos.x(), ruler_height-2, mouse_pos.x(), geometry().height());

	if (!measuring) {
		font.setBold(1);
		font.setPixelSize(9);
		painter.setFont(font);
		long pos = toSample(mouse_pos.x());
		QString num = QString::number(pos);
		painter.drawText(mouse_pos.x()-3, 9, 100, 10, Qt::AlignLeft, num);
	}
}

// --------------------------------------------------------------------------
void TapeView::drawZoomRect(QPainter &painter)
{
	painter.setBrush(brush_zoom);
	painter.setPen(pen_zoom);

	painter.drawRect(mouse_zoom_start.x(), ruler_height, mouse_zoom_end.x()-mouse_zoom_start.x(), geometry().height()-ruler_height-1);
}
// --------------------------------------------------------------------------
void TapeView::drawRegions(QPainter &painter, int ch_height)
{
	// find first and last chunk to show within current view
	QMap<unsigned, TapeChunk>::const_iterator i, b, e;
	b = bs->lowerBound(leftSample());
	if (!bs->isEmpty() && (b != bs->begin())) {
		b--;
	}
	e = bs->lowerBound(rightSample());

	font.setPixelSize(9);
	font.setBold(true);
	painter.setFont(font);

	// display regions
	for (i=b ; i!=e ; i++) {
		if (disp_events) {
			// display events
			long last_row = -1;
			long last_evpar = -1;
			QList<TapeEvent> events = i.value().events;
			quint16 *data = i.value().data;
			for (int j=0 ; j<events.size() ; j++) {
				unsigned offset = events.at(j).offset;
				int xpos = toPos(offset);
				int type = events.at(j).type;
				if (type == C_ERROR) {
					if (xpos == last_evpar) continue;
					last_evpar = xpos;
				} else if (xpos == last_row) {
					continue;
				}
				last_row = xpos;
				if ((offset > leftSample()) && (offset < rightSample())) {
					if (type == C_ROW) {
						painter.setPen(pen_event);
					} else {
						painter.setPen(QPen(QColor(255,40,40, 180), 1));
					}
					painter.drawLine(toPos(offset), ruler_height, toPos(offset), geometry().height()-1);
				}
				if ((scale <= 2) && (disp_bytes)) {
					unsigned char b;
					if (j < i.value().bytes) {
						b = data[j];
					} else {
						b = ' ';
					}
					QString str;
					if ((b >= 32) && (b <= 126)) {
						painter.setPen(QPen(QColor(255,255,150)));
						str = QString(b);
					} else {
						painter.setPen(QPen(QColor(211,211,255)));
						str = QString::number(b, 16);
					}

					painter.drawText(toPos(offset)+1, geometry().height()-10, 30, 10, Qt::AlignLeft, str);
				}
			}
		}

		int start_sample = i.value().beg;
		int end_sample = i.value().end;
		int len_samples = i.value().len;

		// display region
		if (disp_regions) {
			if (start_sample < leftSample()) {
				len_samples -= leftSample()-start_sample;
				start_sample = leftSample();
			}
			painter.setPen(pen_block);
			if ((i.value().type == C_BLOCK) || (i.value().type == C_MARK)) {
				painter.setBrush(brush_block);
			} else {
				painter.setBrush(brush_mark);
			}
			painter.drawRect(toPos(start_sample), ruler_height, toLen(len_samples), geometry().height()-1);
		}

		// display hpar errors
		painter.setPen(QPen(QColor(255,40,40, 180), 1));
		int hpar_err = i.value().hpar_err;
		for (int ch=0 ; ch<9 ; ch++) {
			int ch_mid = ruler_height + (ch+1) * ch_height - (ch_height / 2);
			int ebit = (hpar_err >> ch) & 1;
			if (ebit) {
				painter.drawLine(toPos(start_sample), ch_mid, toPos(end_sample), ch_mid);
			}
		}
	}

}

// --------------------------------------------------------------------------
void TapeView::drawMeasure(QPainter &painter)
{
	int b = mouse_measure_start.x();
	int e = mouse_pos.x();
	int align = Qt::AlignRight;
	if (b > e) {
		int s;
		s = b;
		b = e;
		e = s;
		align = Qt::AlignLeft;
	}

	painter.setPen(pen_measure);
	painter.drawLine(b, ruler_height, b, geometry().height()-1);
	painter.drawLine(e, ruler_height, e, geometry().height()-1);

	painter.setBrush(brush_measure);

	long len = toSample(e) - toSample(b);
	QString num = QString::number(len);
	int width = e-b;
	if (width < num.length()*10) {
		width = num.length()*10;
	}
	painter.drawRect(b, 0, width, ruler_height);

	font.setBold(true);
	font.setPixelSize(12);
	painter.setFont(font);

	painter.setPen(pen_bg);
	painter.drawText(b, 4, width, 12, align, num);
}

// --------------------------------------------------------------------------
void TapeView::paintEvent(QPaintEvent *event)
{
	QPainter painter;
	painter.begin(this);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

	if (in_edit) {
		painter.fillRect(rect(), QBrush(QColor(90, 60, 60, 255)));
	}

	int ch_height = (geometry().height()-ruler_height) / 9;

	drawRuler(painter);

	if (td && td->tape_loaded()) {
		if (disp_signals || disp_edges) {
			drawTracks(painter, ch_height);
		}
	}

	if (zooming) {
		drawZoomRect(painter);
	}
	if (disp_regions || disp_events) {
		drawRegions(painter, ch_height);
	}

	drawTrackNames(painter, ch_height);

	if (disp_mouse) {
		drawMouse(painter);
	}

	if (measuring) {
		drawMeasure(painter);
	}

	painter.end();
}

// --------------------------------------------------------------------------
void TapeView::scroll(int pos)
{
	offset = pos * (td->tape_len() - toSampleLen(geometry().width())) / 1000;
	update();
}

// vim: tabstop=4 shiftwidth=4 autoindent
