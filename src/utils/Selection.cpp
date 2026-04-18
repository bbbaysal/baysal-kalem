#include "../widgets/DrawingWidget.h"
#include "../utils/Selection.h"
#include "../tools.h"

extern float scale;
#define handleSize (16 * scale)
#define hitArea (50 * scale)

MovableWidget::MovableWidget(QWidget *parent) : QWidget(parent) {
    crop = new QLabel(this);
    crop->setAttribute(Qt::WA_TransparentForMouseEvents);
}

#define SELECT_PADDING (int)(handleSize)

void MovableWidget::paintEvent(QPaintEvent *event) {
    (void)event;

    // Smart borders: Only show dashed line if in SELECTION mode and widget is active
    if (!isActiveSelection || drawing->getPen() != SELECTION) return;

    // Border and handles should be visible whenever the widget is active
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw 2px dashed border around the INNER content area
    painter.setPen(QPen(QColor(drawing->penColor), 2, Qt::DashLine));
    painter.drawRect(SELECT_PADDING, SELECT_PADDING, width() - 2*SELECT_PADDING, height() - 2*SELECT_PADDING);

    // Draw handles at 4 corners of the INNER content
    painter.setPen(QPen(Qt::white, 1));
    painter.setBrush(QBrush(QColor(drawing->penColor)));
    painter.drawRect(SELECT_PADDING - handleSize/2, SELECT_PADDING - handleSize/2, handleSize, handleSize); // TL
    painter.drawRect(width() - SELECT_PADDING - handleSize/2, SELECT_PADDING - handleSize/2, handleSize, handleSize); // TR
    painter.drawRect(SELECT_PADDING - handleSize/2, height() - SELECT_PADDING - handleSize/2, handleSize, handleSize); // BL
    painter.drawRect(width() - SELECT_PADDING - handleSize/2, height() - SELECT_PADDING - handleSize/2, handleSize, handleSize); // BR

    // Draw larger hollow center circle (çember)
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(drawing->penColor), 2, Qt::SolidLine));
    int circSize = (int)(20 * scale);
    painter.drawEllipse(QPoint(width()/2, height()/2), circSize, circSize);

    // Draw Clone (+) button at top-right
    painter.setBrush(QBrush(QColor(drawing->penColor)));
    painter.setPen(QPen(Qt::white, 2));
    int btnSize = SELECT_PADDING * 1.5;
    QRect cloneRect(width() - btnSize - 2, 2, btnSize, btnSize);
    painter.drawEllipse(cloneRect);
    // Draw "+" sign
    int margin = btnSize / 4;
    painter.drawLine(cloneRect.center().x() - btnSize/2 + margin, cloneRect.center().y(), cloneRect.center().x() + btnSize/2 - margin, cloneRect.center().y());
    painter.drawLine(cloneRect.center().x(), cloneRect.center().y() - btnSize/2 + margin, cloneRect.center().x(), cloneRect.center().y() + btnSize/2 - margin);
}

void MovableWidget::mousePressEvent(QMouseEvent *event) {
    initialGeometry = geometry();
    initialMousePos = event->globalPos();
    QPoint p = event->pos();
    
    int btnSize = SELECT_PADDING * 1.5;
    QRect cloneHitRect(width() - btnSize - 10, -10, btnSize + 20, btnSize + 20);

    // Adjust hit areas for SELECT_PADDING
    if (cloneHitRect.contains(p)) activeHandle = Clone;
    else if (QRect(SELECT_PADDING - handleSize, SELECT_PADDING - handleSize, handleSize*2, handleSize*2).contains(p)) activeHandle = TopLeft;
    else if (QRect(width() - SELECT_PADDING - handleSize, SELECT_PADDING - handleSize, handleSize*2, handleSize*2).contains(p)) activeHandle = TopRight;
    else if (QRect(SELECT_PADDING - handleSize, height() - SELECT_PADDING - handleSize, handleSize*2, handleSize*2).contains(p)) activeHandle = BottomLeft;
    else if (QRect(width() - SELECT_PADDING - handleSize, height() - SELECT_PADDING - handleSize, handleSize*2, handleSize*2).contains(p)) activeHandle = BottomRight;
    else activeHandle = DragMode;

    if (activeHandle == Clone) setCursor(Qt::PointingHandCursor);
    else if (activeHandle == TopLeft || activeHandle == BottomRight) setCursor(Qt::SizeFDiagCursor);
    else if (activeHandle == TopRight || activeHandle == BottomLeft) setCursor(Qt::SizeBDiagCursor);
    else setCursor(Qt::SizeAllCursor);
}

void MovableWidget::mouseMoveEvent(QMouseEvent *event) {
    if (activeHandle == None) return;

    QPoint delta = event->globalPos() - initialMousePos;
    
    if (activeHandle == DragMode) {
        move(initialGeometry.topLeft() + delta);
    } else {
        // Calculations for INNER content
        int x1 = initialGeometry.left() + SELECT_PADDING;
        int y1 = initialGeometry.top() + SELECT_PADDING;
        int x2 = initialGeometry.right() - SELECT_PADDING;
        int y2 = initialGeometry.bottom() - SELECT_PADDING;

        if (activeHandle == TopLeft) { x1 += delta.x(); y1 += delta.y(); }
        else if (activeHandle == TopRight) { x2 += delta.x(); y1 += delta.y(); }
        else if (activeHandle == BottomLeft) { x1 += delta.x(); y2 += delta.y(); }
        else if (activeHandle == BottomRight) { x2 += delta.x(); y2 += delta.y(); }

        QRect contentRect = QRect(QPoint(x1, y1), QPoint(x2, y2)).normalized();
        
        if (contentRect.width() > 10 && contentRect.height() > 10) {
            // Widget size includes SELECT_PADDING
            setFixedSize(contentRect.width() + 2*SELECT_PADDING, contentRect.height() + 2*SELECT_PADDING);
            move(contentRect.left() - SELECT_PADDING, contentRect.top() - SELECT_PADDING);
            
            // Update the image inside
            QPixmap scaledPix = QPixmap::fromImage(image.scaled(contentRect.width() * mainWidget->devicePixelRatio(), 
                                                               contentRect.height() * mainWidget->devicePixelRatio(), 
                                                               Qt::IgnoreAspectRatio, 
                                                               Qt::SmoothTransformation));
            scaledPix.setDevicePixelRatio(mainWidget->devicePixelRatio());
            crop->setPixmap(scaledPix);
            crop->setGeometry(SELECT_PADDING, SELECT_PADDING, contentRect.width(), contentRect.height());
        }
    }
}

void MovableWidget::mouseReleaseEvent(QMouseEvent *event) {
    (void)event;
    if (activeHandle == Clone) {
        QPixmap currentPix = QPixmap::fromImage(image);
        QPoint oldPos = pos();
        drawing->createSelectionFromPixmap(currentPix);
        // Move the new selection slightly offset
        drawing->cropWidget->move(oldPos.x() + 40*scale, oldPos.y() + 40*scale);
    }
    activeHandle = None;
    setCursor(Qt::ArrowCursor);
}

void DrawingWidget::createSelectionFromPixmap(QPixmap pix) {
    mergeSelection(); // Merge any existing selection before creating a new one
    hasSelection = true;
    cropWidget->image = pix.toImage();
    int w = pix.width() / mainWidget->devicePixelRatio();
    int h = pix.height() / mainWidget->devicePixelRatio();
    
    // Set widget size with SELECT_PADDING
    cropWidget->setFixedSize(w + 2*SELECT_PADDING, h + 2*SELECT_PADDING);
    
    QPixmap pixmap = QPixmap::fromImage(cropWidget->image);
    cropWidget->crop->setPixmap(pixmap);
    cropWidget->crop->setGeometry(SELECT_PADDING, SELECT_PADDING, w, h);
    cropWidget->raise();
    cropWidget->show();
    cropWidget->update(); 
}

void DrawingWidget::createMagnifierFromPixmap(QPixmap pix) {
    clearMagnifier(); // Clear any existing magnifier snippet
    magnifierWidget->image = pix.toImage();
    int w = pix.width() / mainWidget->devicePixelRatio();
    int h = pix.height() / mainWidget->devicePixelRatio();
    
    magnifierWidget->setFixedSize(w + 2*SELECT_PADDING, h + 2*SELECT_PADDING);
    
    QPixmap pixmap = QPixmap::fromImage(magnifierWidget->image);
    magnifierWidget->crop->setPixmap(pixmap);
    magnifierWidget->crop->setGeometry(SELECT_PADDING, SELECT_PADDING, w, h);
    magnifierWidget->show();
    setMagnifierSelected(true); // Will automatically raise and activate it!
}

void DrawingWidget::clearMagnifier() {
    magnifierWidget->setFixedSize(0,0);
    magnifierWidget->move(QPoint(-1,-1));
    magnifierWidget->image = QImage(QSize(0,0), QImage::Format_ARGB32);
    magnifierWidget->image.fill(QColor("transparent"));
    magnifierWidget->hide();
}

bool hasSelection = false;

void DrawingWidget::setMagnifierSelected(bool selected) {
    if(!magnifierWidget || magnifierWidget->isHidden()) return;
    magnifierWidget->isActiveSelection = selected;
    if(selected && penType == SELECTION) {
        magnifierWidget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
        magnifierWidget->raise();
        if(cropWidget) cropWidget->raise(); // keep standard crop above
    } else {
        magnifierWidget->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        magnifierWidget->stackUnder(background);
    }
    magnifierWidget->update();
}

void DrawingWidget::toggleCoordinateTool() {
    if (!coordinateWidget->isHidden()) {
        coordinateWidget->hide();
        return;
    }
    QIcon icon(":images/overlay/coordinate_plane.svg");
    coordinateWidget->image = icon.pixmap(icon.actualSize(QSize(400*scale, 400*scale))).toImage();
    int w = coordinateWidget->image.width() / mainWidget->devicePixelRatio();
    int h = coordinateWidget->image.height() / mainWidget->devicePixelRatio();
    
    coordinateWidget->setFixedSize(w + 2*SELECT_PADDING, h + 2*SELECT_PADDING);
    coordinateWidget->crop->setPixmap(QPixmap::fromImage(coordinateWidget->image));
    coordinateWidget->crop->setGeometry(SELECT_PADDING, SELECT_PADDING, w, h);
    coordinateWidget->move((mainWidget->width() - w)/2, (mainWidget->height() - h)/2);
    coordinateWidget->show();
    setCoordinateSelected(true);
}

void DrawingWidget::toggleRulerTool() {
    if (!rulerWidget->isHidden()) {
        rulerWidget->hide();
        return;
    }
    QIcon icon(":images/overlay/ruler.svg");
    rulerWidget->image = icon.pixmap(icon.actualSize(QSize(600*scale, 200*scale))).toImage();
    int w = rulerWidget->image.width() / mainWidget->devicePixelRatio();
    int h = rulerWidget->image.height() / mainWidget->devicePixelRatio();
    
    rulerWidget->setFixedSize(w + 2*SELECT_PADDING, h + 2*SELECT_PADDING);
    rulerWidget->crop->setPixmap(QPixmap::fromImage(rulerWidget->image));
    rulerWidget->crop->setGeometry(SELECT_PADDING, SELECT_PADDING, w, h);
    rulerWidget->move((mainWidget->width() - w)/2, (mainWidget->height() - h)/2);
    rulerWidget->show();
    setRulerSelected(true);
}

void DrawingWidget::toggleProtractorTool() {
    if (!protractorWidget->isHidden()) {
        protractorWidget->hide();
        return;
    }
    QIcon icon(":images/overlay/aciolcer.png");
    protractorWidget->image = icon.pixmap(icon.actualSize(QSize(400*scale, 400*scale))).toImage();
    int w = protractorWidget->image.width() / mainWidget->devicePixelRatio();
    int h = protractorWidget->image.height() / mainWidget->devicePixelRatio();
    
    protractorWidget->setFixedSize(w + 2*SELECT_PADDING, h + 2*SELECT_PADDING);
    protractorWidget->crop->setPixmap(QPixmap::fromImage(protractorWidget->image));
    protractorWidget->crop->setGeometry(SELECT_PADDING, SELECT_PADDING, w, h);
    protractorWidget->move((mainWidget->width() - w)/2, (mainWidget->height() - h)/2);
    protractorWidget->show();
    setProtractorSelected(true);
}

void DrawingWidget::setCoordinateSelected(bool selected) {
    if(!coordinateWidget || coordinateWidget->isHidden()) return;
    coordinateWidget->isActiveSelection = selected;
    if(selected && penType == SELECTION) {
        coordinateWidget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
        coordinateWidget->raise();
        if(cropWidget) cropWidget->raise();
    } else {
        coordinateWidget->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        coordinateWidget->stackUnder(background);
    }
    coordinateWidget->update();
}

void DrawingWidget::setRulerSelected(bool selected) {
    if(!rulerWidget || rulerWidget->isHidden()) return;
    rulerWidget->isActiveSelection = selected;
    if(selected && penType == SELECTION) {
        rulerWidget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
        rulerWidget->raise();
        if(cropWidget) cropWidget->raise();
    } else {
        rulerWidget->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        rulerWidget->stackUnder(background);
    }
    rulerWidget->update();
}

void DrawingWidget::setProtractorSelected(bool selected) {
    if(!protractorWidget || protractorWidget->isHidden()) return;
    protractorWidget->isActiveSelection = selected;
    if(selected && penType == SELECTION) {
        protractorWidget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
        protractorWidget->raise();
        if(cropWidget) cropWidget->raise();
    } else {
        protractorWidget->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        protractorWidget->stackUnder(background);
    }
    protractorWidget->update();
}


void DrawingWidget::createSelection(int source) {
    debug("source: %d\n", source);
    QPointF startPoint = geo.first(source);
    QPointF endPoint = geo.last(source);
    QPoint topLeft(qMin(startPoint.x(), endPoint.x()), qMin(startPoint.y(), endPoint.y()));
    QPoint bottomRight(qMax(startPoint.x(), endPoint.x()), qMax(startPoint.y(), endPoint.y()));
    QRect cropRect(topLeft, bottomRight);

    // Revert Smart Capture: Back to simple click rejection
    if (cropRect.width() < 5 || cropRect.height() < 5) {
        if (hasSelection) {
            mergeSelection(); // "boşa tıklandığında sadece seçim iptal olsun"
        }
        
        bool found = false;
        if (!magnifierWidget->isHidden() && magnifierWidget->geometry().contains(startPoint.toPoint())) {
            setMagnifierSelected(true);
            found = true;
        } 
        if (!coordinateWidget->isHidden() && coordinateWidget->geometry().contains(startPoint.toPoint())) {
            setCoordinateSelected(true);
            found = true;
        }
        if (!rulerWidget->isHidden() && rulerWidget->geometry().contains(startPoint.toPoint())) {
            setRulerSelected(true);
            found = true;
        }
        if (!protractorWidget->isHidden() && protractorWidget->geometry().contains(startPoint.toPoint())) {
            setProtractorSelected(true);
            found = true;
        }
        
        if (!found) {
            // Dışarı tıklarsak resmi de kilitleyelim ("yeni boş bölge seçimi yapmasın" - zira sürükleme de başlamamış)
            bool wasActive = (!magnifierWidget->isHidden() && magnifierWidget->isActiveSelection) || (!coordinateWidget->isHidden() && coordinateWidget->isActiveSelection) || (!rulerWidget->isHidden() && rulerWidget->isActiveSelection) || (!protractorWidget->isHidden() && protractorWidget->isActiveSelection);
            setMagnifierSelected(false);
            setCoordinateSelected(false);
            setRulerSelected(false);
            setProtractorSelected(false);
            if (wasActive) {
                setPen(PEN);
                updateGui();
            }
        }
        return;
    }

    // Seçim kutusu başlatılıyorsa, büyüteç yanlışlıkla seçilmesin diye kilitle!
    setMagnifierSelected(false);
    setCoordinateSelected(false);
    setRulerSelected(false);
    setProtractorSelected(false);

    mergeSelection(); // Merge previous selection
    hasSelection = true;
    
    QPixmap pix = QPixmap(cropRect.size()*mainWidget->devicePixelRatio());
    pix.setDevicePixelRatio(mainWidget->devicePixelRatio());
    pix.fill(QColor("transparent"));
    cropWidget->image = pix.toImage();
    
    // Capture from the active drawing layer (image)
    painter.begin(&(cropWidget->image));
    painter.setPen(Qt::NoPen);
    painter.drawPixmap(
        topLeft.x()*-1,
        topLeft.y()*-1,
        image);
    painter.end();

    // Clear the area from the active drawing layer
    painter.begin(&image);
    painter.setBrush(QBrush(QColor("transparent")));
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.setPen(Qt::NoPen);
    painter.drawRect(cropRect);
    painter.end();

    cropWidget->setFixedSize(cropRect.width() + 2*SELECT_PADDING, cropRect.height() + 2*SELECT_PADDING);
    QPixmap pixmap = QPixmap::fromImage(cropWidget->image);
    cropWidget->crop->setPixmap(pixmap);
    cropWidget->crop->setGeometry(SELECT_PADDING, SELECT_PADDING, cropRect.width(), cropRect.height());
    cropWidget->move(topLeft.x() - SELECT_PADDING, topLeft.y() - SELECT_PADDING);
    cropWidget->raise();
    cropWidget->show();
    update();
}

void DrawingWidget::clearSelection() {
    if(!hasSelection){
        return;
    }
    hasSelection = false;
    cropWidget->setFixedSize(0,0);
    cropWidget->move(QPoint(-1,-1));
    cropWidget->image = QImage(QSize(0,0), QImage::Format_ARGB32);
    cropWidget->image.fill(QColor("transparent"));
    cropWidget->hide();
    addImage(image.toImage());
}

void DrawingWidget::mergeSelection() {
    if(!hasSelection) return;
    
    painter.begin(&image);
    // Use DestinationOver to put the selection BEHIND existing strokes
    painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
    painter.drawImage(
        cropWidget->x() + SELECT_PADDING,
        cropWidget->y() + SELECT_PADDING,
        cropWidget->image.scaled(
            (cropWidget->width() - 2*SELECT_PADDING)*mainWidget->devicePixelRatio(),
            (cropWidget->height() - 2*SELECT_PADDING)*mainWidget->devicePixelRatio()
        )
    );
    painter.end();
    clearSelection();
    update();
}

