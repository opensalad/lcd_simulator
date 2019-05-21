#include "pin_widget.h"

#include <lcd_core.h>
#include <metric_macros.h>

#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QMimeData>

#include <QPaintEvent>
#include <QPainter>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QUuid>
#include <QtMath>

#define DRAG_DISTANCE 5

#define LOGICAL_HIGH QColor(202, 244, 65)
#define LOGICAL_LOW QColor(244, 172, 65)
#define LOGICAL_Z QColor(244, 223, 65)

#ifdef ZOOM_FACTOR
#define FONT_SIZE 12
#else
#define FONT_SIZE 8
#endif

qreal distance(QPoint p1, QPoint p2)
{
	return qSqrt((p1.x() - p2.x()) * (p1.x() - p2.x()) + (p1.y() - p2.y()) * (p1.y() - p2.y()));
}

PinWidget::PinWidget(QWidget* parent)
	: PinWidget(QString("pin_%1").arg(QUuid::createUuid().toString(QUuid::Id128)), parent)
{}

PinWidget::PinWidget(Pin* pin, QWidget* parent)
	: QWidget(parent),
	__pin(pin),
	__hover(false)
{
	__pin->setParent(this);
	init();
}

PinWidget::PinWidget(QString const& pin_name, QWidget* parent)
	: QWidget(parent),
	__pin(new Pin(this)),
	__hover(false)
{
	init();
}

void PinWidget::init()
{
	setAcceptDrops(true);
	setAttribute(Qt::WA_NoSystemBackground);
	setCursor(Qt::PointingHandCursor);
	setFixedSize(NORMALIZE_X(PIN_WIDTH), NORMALIZE_Y(PIN_HEIGHT));
	setToolTip(__pin->getName());
	connect(__pin, &Pin::signalChanged, this, qOverload<>(&QWidget::repaint));
}

void PinWidget::mousePressEvent(QMouseEvent* event)
{
	__drag_options.__drag_started = true;
	__drag_options.__drag_started_position = event->pos();
	setMouseTracking(true);
}

void PinWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (!__drag_options.__drag_started)
		return;

	if (distance(__drag_options.__drag_started_position, event->pos()) < DRAG_DISTANCE)
		return;

	QDrag drag(this);
	QMimeData* mimeData = new QMimeData;
	mimeData->setData("application/json", __pin->serialize());
	drag.setMimeData(mimeData);
	drag.exec(Qt::MoveAction);
}

void PinWidget::mouseReleaseEvent(QMouseEvent* event)
{
	Q_UNUSED(event);
	__pin->setSignal(!__pin->getSignal());
	setMouseTracking(false);
}
void PinWidget::dragEnterEvent(QDragEnterEvent* event)
{
	qDebug() << "dragEnterEvent " << event->mimeData()->formats();
	const QMimeData* mime = event->mimeData();
	QJsonDocument doc = QJsonDocument::fromJson(mime->data("application/json"));
	QJsonObject obj = doc.object();
	if (obj.contains("type") && obj.value("type").toString() == "pin")
		event->accept();
}

void PinWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
	event->accept();
}

void PinWidget::dragMoveEvent(QDragMoveEvent* event)
{
	event->accept();
}

void PinWidget::dropEvent(QDropEvent* event)
{
	qDebug() << "dropEvent " << event->mimeData()->formats();
	const QMimeData* mime = event->mimeData();
	__pin->deserialize(mime->data("application/json"));
}

void PinWidget::enterEvent(QEvent* event)
{
	Q_UNUSED(event);
	__hover = true;
	update();
}

void PinWidget::leaveEvent(QEvent* event)
{
	Q_UNUSED(event);
	__hover = false;
	update();
}

void PinWidget::paintEvent(QPaintEvent* event)
{
	QPainter ptr(this);
	QPainterPath path;
	QFont font("Calibri", FONT_SIZE, QFont::Bold, false);
	ptr.setRenderHint(QPainter::HighQualityAntialiasing);
	ptr.setFont(font);
	path.moveTo(0, 0);
	path.lineTo(0, NORMALIZE_Y(PIN_HEIGHT));
	path.arcTo(QRectF(0, NORMALIZE_Y(PIN_HEIGHT) - NORMALIZE_Y(PIN_WIDTH), NORMALIZE_X(PIN_WIDTH), NORMALIZE_Y(PIN_WIDTH)), 180, 180);
	path.lineTo(NORMALIZE_X(PIN_WIDTH), 0);
	path.closeSubpath();
	ptr.setPen(Qt::NoPen);
	if(__hover)
		ptr.setBrush(QColor(247, 229, 96));
	else
		ptr.setBrush(QColor(244, 209, 66));
	ptr.drawPath(path);
	ptr.setPen(__pin->getSignal() ? Qt::darkGreen : Qt::darkRed);
	ptr.drawText(QRect(0, 0, NORMALIZE_X(PIN_WIDTH), NORMALIZE_Y(PIN_HEIGHT)), Qt::AlignCenter, QString(__pin->getSignal() ? '1' : '0'));
}
