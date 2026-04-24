#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QColor>
#include <QSvgRenderer>
#include <QPixmap>

#include <math.h>

#include "../tools.h"

#include "OverView.h"
#include "DrawingWidget.h"


OverView::OverView(QWidget *parent) : QWidget(parent) {
    setStyleSheet(
    "background: none;");
}

void OverView::updateImage(){
    update();
}
void OverView::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    int penSize = drawing->penSize[getPen()];
    int penType = getPen();
    
    // Draw background 
    QPen pen(QColor("#f3232323"), 12*scale, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setBrush(QBrush(board->background));
    painter.setPen(pen);
    painter.drawRoundedRect(rect().adjusted(6*scale, 6*scale, -6*scale, -6*scale), 12*scale, 13*scale);
    
    int w = width() - 2*padding;
    int h = height() - 2*padding;

    if (w <= 2*padding || h <= 2*padding) {
        return;
    }

    QColor penColor = drawing->penColor;
    if (penType == MARKER) {
        penColor.setAlpha(127);
    }
    
    pen.setColor(penColor);
    pen.setWidth(penSize);
    painter.setPen(pen);

    if (penType == ERASER) {
        // Simple circle for eraser preview to be safe and fast
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(QRectF((width() - penSize)/2.0, (height() - penSize)/2.0, penSize, penSize));
    } else {
        // Draw the sine wave
        QPainterPath path;
        int xPrev = padding;
        double yPrev = ((h - 2*penSize) / 2.0) * sin(2.0 * M_PI * (double)padding / (double)w) + h / 2.0;
        
        for (int x = padding + 1; x <= w - padding; x += qMax(1, (int)scale)) {
            double y = ((h - 2*penSize) / 2.0) * sin(2.0 * M_PI * x / (double)w) + h / 2.0;
            path.moveTo(QPointF(xPrev, yPrev + padding));
            path.lineTo(QPointF(x, y + padding + (penSize/2.0)));
            xPrev = x;
            yPrev = y;
        }
        painter.drawPath(path);
    }
}
