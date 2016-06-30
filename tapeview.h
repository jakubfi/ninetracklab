#ifndef TAPEVIEW_H
#define TAPEVIEW_H

#include <QWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QTextDocument>

#include "tapedrive.h"
#include "blockstore.h"

// --------------------------------------------------------------------------
class TapeView : public QWidget
{
private:
	Q_OBJECT
	TapeDrive *td;
	QMap<unsigned, TapeChunk> *bs;

	int offset;
	double scale;
	QPoint mouse_pos;
	QPoint mouse_drag_start;
	int mouse_drag_pixels;
	QPoint mouse_zoom_start, mouse_zoom_end;
	QPoint mouse_measure_start, mouse_measure_end;
	QPoint mouse_edit_start;
	int ruler_height;
	int zooming;
	int measuring;
	int in_edit;

	QPen pen_wave;
	QPen pen_edge;
	QPen pen_tick;
	QPen pen_trackname;
	QPen pen_mouse;
	QPen pen_zoom;
	QBrush brush_bg;
	QPen pen_bg;
	QBrush brush_bg_ruler;
	QBrush brush_zoom;
	QFont font;
	QPen pen_block;
	QBrush brush_block;
	QPen pen_event;
	QPen pen_measure;
	QBrush brush_measure;
	QBrush brush_mark;

	int disp_mouse;
	int disp_edges;
	int disp_regions;
	int disp_signals;
	int disp_events;
	int disp_bytes;

	inline long toSample(int d) { return offset + d*scale; }
	inline long toSampleLen(int d) { return d*scale; }
	inline int toPos(long s) { return (s-offset) / scale; }
	inline int toLen(long s) { return s / scale; }
	inline long viewSamples() { return toSampleLen(geometry().width()); }

	void wave_drag(QMouseEvent *event);
	void wave_zoom(QMouseEvent *event);

	void drawRuler(QPainter &painter);
	void drawTracks(QPainter &painter, int ch_height);
	void drawTrackNames(QPainter &painter, int ch_height);
	void drawMouse(QPainter &painter);
	void drawZoomRect(QPainter &painter);
	void drawRegions(QPainter &painter, int ch_height);
	void drawMeasure(QPainter &painter);
	void signalEdit(QPoint from, QPoint pos);

	void correctView();

protected:
	void paintEvent(QPaintEvent *event);
	void wheelEvent(QWheelEvent* event);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);

public:
	explicit TapeView(QWidget *parent = 0);
	void useTapeDrive(TapeDrive *tapedrive) { td = tapedrive; }
	void useBlockStore(QMap<unsigned, TapeChunk> *blockstore) { bs = blockstore; }
	void zoomRegion(int left, int right);
	void zoomAround(int pos, double scale);
	long leftSample() { return offset; }
	long rightSample() { return toSample(geometry().width()); }

public slots:
	void show_signals(int v) { disp_signals = v; update(); }
	void show_mouse(int v) { disp_mouse = v; update(); }
	void show_edges(int v) { disp_edges = v; update(); }
	void show_regions(int v) { disp_regions = v; update(); }
	void show_events(int v) { disp_events = v; update(); }
	void show_bytes(int v) { disp_bytes = v; update(); }
	void zoom11();
	void zoomAll();
	void zoomIn();
	void zoomOut();
	void edit(bool v) { in_edit = v; update(); }
	void scroll(int pos);

signals:
   void editable(bool state);

};

#endif

// vim: tabstop=4 shiftwidth=4 autoindent
