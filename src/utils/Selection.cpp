#include "../widgets/DrawingWidget.h"
#include "../utils/Selection.h"
#include "../tools.h"

extern float scale;
#define handleSize (16 * scale)
#define hitArea (50 * scale)

MovableWidget::MovableWidget(QWidget *parent) : QWidget(parent) {
    crop = new QLabel(this);
    crop->setAttribute(Qt::WA_TransparentForMouseEvents);
    crop->hide(); // Hide QLabel, we will draw manually in paintEvent for rotation support
    rotationPivot = QPointF(0, 0);
}

#define SELECT_PADDING (int)(handleSize)
#define TOP_PADDING (int)(45 * scale)

void MovableWidget::paintEvent(QPaintEvent *event) {
    (void)event;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    // Content dimensions (assuming original non-rotated size is stored or implied)
    // Actually, we need to know the 'natural' size of the image to draw it correctly.
    // For now, let's assume the widget has been resized to fit the rotated content.
    // We need to find where the original (0,0) of the content is in current rotated space.
    
    // Pivot point in local coordinates: Bottom-middle of the CONTENT area
    // If the widget is sized to bit the rotated content, the pivot is NOT always at width/2.
    // BUT we can enforce it to be centered horizontally in the widget for simplicity if we wish,
    // or calculate its exact local position.
    
    // Better: We always know the pivot's GLOBAL position and we place the widget relative to it.
    // So in LOCAL coordinates, the pivot is at a known distance from the bottom.
    // Use the saved rotationPivot for stable rotation
    QPointF pivot = rotationPivot;

    // Draw rotated image
    if (!image.isNull()) {
        painter.save();
        painter.translate(pivot);
        painter.rotate(rotationAngle);
        painter.translate(-pivot);
        
        // The image itself has a 'natural' width/height which was its size at 0 degrees.
        // We need to find what that size was. 
        // Let's use the image's actual pixel size adjusted for devicePixelRatio.
        // Use current crop size for scaling the visual content
        int imgW = crop->width();
        int imgH = crop->height();

        // Draw it at its 'natural' position relative to the pivot
        // Pivot is at bottom-center of the content area
        QRectF targetRect(pivot.x() - imgW/2.0, pivot.y() - imgH, imgW, imgH);
        
        QPixmap pix = QPixmap::fromImage(image.scaled(imgW * devicePixelRatio(), 
                                                       imgH * devicePixelRatio(),
                                                       Qt::IgnoreAspectRatio, 
                                                       Qt::SmoothTransformation));
        pix.setDevicePixelRatio(devicePixelRatio());
        painter.drawPixmap(targetRect.toRect(), pix);
        
        painter.restore();
    }

    // Smart borders: Only show dashed line if in SELECTION mode and widget is active
    if (!isActiveSelection || drawing->getPen() != SELECTION) return;

    painter.save();
    painter.translate(pivot);
    painter.rotate(rotationAngle);
    painter.translate(-pivot);

    int imgW = crop->width();
    int imgH = crop->height();

    QRect contentRect(pivot.x() - imgW/2.0, pivot.y() - imgH, imgW, imgH);

    // Draw 2px dashed border
    painter.setPen(QPen(QColor(drawing->penColor), 2, Qt::DashLine));
    painter.drawRect(contentRect);

    // Draw handles at 4 corners
    painter.setPen(QPen(Qt::white, 1));
    painter.setBrush(QBrush(QColor(drawing->penColor)));
    painter.drawRect(contentRect.topLeft().x() - handleSize/2, contentRect.topLeft().y() - handleSize/2, handleSize, handleSize); // TL
    painter.drawRect(contentRect.topRight().x() - handleSize/2, contentRect.topRight().y() - handleSize/2, handleSize, handleSize); // TR
    painter.drawRect(contentRect.bottomLeft().x() - handleSize/2, contentRect.bottomLeft().y() - handleSize/2, handleSize, handleSize); // BL
    painter.drawRect(contentRect.bottomRight().x() - handleSize/2, contentRect.bottomRight().y() - handleSize/2, handleSize, handleSize); // BR

    // Draw larger hollow center circle (çember)
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(drawing->penColor), 2, Qt::SolidLine));
    int circSize = (int)(20 * scale);
    painter.drawEllipse(contentRect.center(), circSize, circSize);

    // Draw Clone (+) button (fixed at top-right of content)
    painter.setBrush(QBrush(QColor(drawing->penColor)));
    painter.setPen(QPen(Qt::white, 2));
    int btnSize = (int)(SELECT_PADDING * 1.5);
    QRect cloneRect(contentRect.right() - btnSize/2, contentRect.top() - btnSize/2, btnSize, btnSize);
    painter.drawEllipse(cloneRect);
    int margin = btnSize / 4;
    painter.drawLine(cloneRect.center().x() - btnSize/2 + margin, cloneRect.center().y(), cloneRect.center().x() + btnSize/2 - margin, cloneRect.center().y());
    painter.drawLine(cloneRect.center().x(), cloneRect.center().y() - btnSize/2 + margin, cloneRect.center().y(), cloneRect.center().y() + btnSize/2 - margin);

    // Draw rotation handle (30px circle ABOVE the tool)
    painter.setBrush(QBrush(QColor(drawing->penColor)));
    painter.setPen(QPen(Qt::white, 2));
    int rotateCircleSize = (int)(30 * scale);
    // Position it ~20px above the contentRect top edge
    QRect rotateRect(contentRect.center().x() - rotateCircleSize/2, contentRect.top() - rotateCircleSize - (int)(15 * scale), rotateCircleSize, rotateCircleSize);
    painter.drawEllipse(rotateRect);
    painter.setPen(QPen(Qt::white, 2));
    painter.drawArc(rotateRect.adjusted(5,5,-5,-5), 0, 270 * 16);

    painter.restore();
}

void MovableWidget::updateRotationBounds() {
    // Save current global pivot before resizing
    QPointF pivotLocal = rotationPivot;
    QPointF pivotGlobal = mapToParent(pivotLocal.toPoint());

    // Use current unrotated content dimensions
    int imgW = crop->width();
    int imgH = crop->height();
    int rotateCircleSize = (int)(30 * scale);
    int rotateTopOffset = rotateCircleSize + (int)(15 * scale);

    // Define corner vertices relative to pivot (assuming bottom-center pivot)
    QPointF pts[5] = {
        QPointF(-imgW / 2.0, -imgH), QPointF(imgW / 2.0, -imgH),
        QPointF(-imgW / 2.0, 0), QPointF(imgW / 2.0, 0),
        QPointF(0, -imgH - rotateTopOffset - rotateCircleSize / 2.0)
    };

    // Calculate rotated bounds
    double rad = rotationAngle * M_PI / 180.0;
    double minX = 0, minY = 0, maxX = 0, maxY = 0;
    for (int i = 0; i < 5; i++) {
        double rx = pts[i].x() * cos(rad) - pts[i].y() * sin(rad);
        double ry = pts[i].x() * sin(rad) + pts[i].y() * cos(rad);
        if (i == 0 || rx < minX) minX = rx;
        if (i == 0 || rx > maxX) maxX = rx;
        if (i == 0 || ry < minY) minY = ry;
        if (i == 0 || ry > maxY) maxY = ry;
    }

    // New widget size (bounding box) with padding for handles
    int newW = (maxX - minX) + 2 * SELECT_PADDING + (int)(30 * scale);
    int newH = (maxY - minY) + 2 * SELECT_PADDING + (int)(30 * scale);
    setFixedSize(newW, newH);

    // Calculate new local pivot position relative to the new bounding box top-left
    QPointF pLocal(-minX + SELECT_PADDING + (int)(15 * scale), -minY + SELECT_PADDING + (int)(15 * scale));
    // Move widget so pivotGlobal stays at its canvas position
    move(pivotGlobal.x() - pLocal.x(), pivotGlobal.y() - pLocal.y());

    // Update the tracked local pivot
    rotationPivot = pLocal;
    
    // Ensure the child label (actual content) is correctly placed relative to the pivot in logical 0-deg space
    crop->setGeometry(pLocal.x() - imgW/2.0, pLocal.y() - imgH, imgW, imgH);
    
    update();
}

void MovableWidget::mousePressEvent(QMouseEvent *event) {
    initialGeometry = geometry();
    initialMousePos = event->globalPos();
    initialContentSize = crop->size();
    initialRotationPivot = rotationPivot;
    QPoint p = event->pos();
    
    // Use the saved rotationPivot for relative hit detection
    QPointF pivot = rotationPivot;
    
    // Exact mapping for handles in rotated space
    QTransform trans;
    trans.translate(pivot.x(), pivot.y());
    trans.rotate(rotationAngle);
    trans.translate(-pivot.x(), -pivot.y());
    QPoint localP = trans.inverted().map(p);

    int imgW = crop->width();
    int imgH = crop->height();

    QRect contentRect(pivot.x() - imgW/2.0, pivot.y() - imgH, imgW, imgH);

    int btnSize = (int)(SELECT_PADDING * 1.5);
    QRect cloneHitRect(contentRect.right() - btnSize/2 - 10, contentRect.top() - btnSize/2 - 10, btnSize + 20, btnSize + 20);
    
    int rotateCircleSize = (int)(30 * scale);
    QRect rotateHitRect(contentRect.center().x() - rotateCircleSize/2 - 10, contentRect.top() - rotateCircleSize - (int)(15 * scale) - 10, rotateCircleSize + 20, rotateCircleSize + 20);

    if (rotateHitRect.contains(localP)) activeHandle = Rotate;
    else if (cloneHitRect.contains(localP)) activeHandle = Clone;
    else if (QRect(contentRect.left() - handleSize, contentRect.top() - handleSize, handleSize*2, handleSize*2).contains(localP)) activeHandle = TopLeft;
    else if (QRect(contentRect.right() - handleSize, contentRect.top() - handleSize, handleSize*2, handleSize*2).contains(localP)) activeHandle = TopRight;
    else if (QRect(contentRect.left() - handleSize, contentRect.bottom() - handleSize, handleSize*2, handleSize*2).contains(localP)) activeHandle = BottomLeft;
    else if (QRect(contentRect.right() - handleSize, contentRect.bottom() - handleSize, handleSize*2, handleSize*2).contains(localP)) activeHandle = BottomRight;
    else activeHandle = DragMode;

    if (activeHandle == Rotate || activeHandle == Clone) setCursor(Qt::PointingHandCursor);
    else if (activeHandle == TopLeft || activeHandle == BottomRight) setCursor(Qt::SizeFDiagCursor);
    else if (activeHandle == TopRight || activeHandle == BottomLeft) setCursor(Qt::SizeBDiagCursor);
    else setCursor(Qt::SizeAllCursor);
}

void MovableWidget::mouseMoveEvent(QMouseEvent *event) {
    if (activeHandle == None) return;

    if (activeHandle == Rotate) {
        QPointF pivotLocal = rotationPivot;
        QPoint p = event->pos();
        double angle = atan2(p.y() - pivotLocal.y(), p.x() - pivotLocal.x()) * 180.0 / M_PI;
        rotationAngle = angle + 90.0;
        updateRotationBounds();
        return;
    }

    QPoint delta = event->globalPos() - initialMousePos;
    
    if (activeHandle == DragMode) {
        move(initialGeometry.topLeft() + delta);
    } else {
        // Rotated Resizing Logic
        // Transform the mouse movement into the widget's local rotated space
        double rad = -rotationAngle * M_PI / 180.0;
        double lx = delta.x() * cos(rad) - delta.y() * sin(rad);
        double ly = delta.x() * sin(rad) + delta.y() * cos(rad);
        
        int x1 = 0, y1 = 0;
        int x2 = initialContentSize.width();
        int y2 = initialContentSize.height();

        if (activeHandle == TopLeft) { x1 += lx; y1 += ly; }
        else if (activeHandle == TopRight) { x2 += lx; y1 += ly; }
        else if (activeHandle == BottomLeft) { x1 += lx; y2 += ly; }
        else if (activeHandle == BottomRight) { x2 += lx; y2 += ly; }

        QRect contentRect = QRect(QPoint(x1, y1), QPoint(x2, y2)).normalized();
        
        // Preserve aspect ratio for tools (non-drawing selections)
        if (this != drawing->cropWidget && !image.isNull()) {
            double aspect = (double)image.width() / image.height();
            int newW = contentRect.width();
            int newH = contentRect.height();
            if (newW / (double)newH > aspect) {
                newH = newW / aspect;
            } else {
                newW = newH * aspect;
            }
            // Adjust based on handle
            if (activeHandle == TopLeft) {
                contentRect.setTopLeft(QPoint(contentRect.right() - newW, contentRect.bottom() - newH));
            } else if (activeHandle == TopRight) {
                contentRect.setTopRight(QPoint(contentRect.left() + newW, contentRect.bottom() - newH));
            } else if (activeHandle == BottomLeft) {
                contentRect.setBottomLeft(QPoint(contentRect.right() - newW, contentRect.top() + newH));
            } else if (activeHandle == BottomRight) {
                contentRect.setBottomRight(QPoint(contentRect.left() + newW, contentRect.top() + newH));
            }
        }

        if (contentRect.width() > 10 && contentRect.height() > 10) {
            // Update the unrotated content size
            crop->setFixedSize(contentRect.width(), contentRect.height());
            
            // Re-calculate bounding box and position to keep rotation pivot stable
            updateRotationBounds();

            // Update the pixmap inside
            QPixmap scaledPix = QPixmap::fromImage(image.scaled(contentRect.width() * devicePixelRatio(), 
                                                               contentRect.height() * devicePixelRatio(), 
                                                               Qt::IgnoreAspectRatio, 
                                                               Qt::SmoothTransformation));
            scaledPix.setDevicePixelRatio(devicePixelRatio());
            crop->setPixmap(scaledPix);
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
    
    // Set widget size with SELECT_PADDING and TOP_PADDING
    cropWidget->setFixedSize(w + 2*SELECT_PADDING, h + SELECT_PADDING + TOP_PADDING);
    cropWidget->rotationPivot = QPointF(cropWidget->width() / 2.0, cropWidget->height() - SELECT_PADDING);
    
    QPixmap pixmap = QPixmap::fromImage(cropWidget->image);
    cropWidget->crop->setPixmap(pixmap);
    cropWidget->rotationAngle = 0.0;
    cropWidget->crop->setGeometry(SELECT_PADDING, TOP_PADDING, w, h);
    cropWidget->raise();
    cropWidget->show();
    cropWidget->update(); 
}

void DrawingWidget::createMagnifierFromPixmap(QPixmap pix) {
    clearMagnifier(); // Clear any existing magnifier snippet
    magnifierWidget->image = pix.toImage();
    int w = pix.width() / mainWidget->devicePixelRatio();
    int h = pix.height() / mainWidget->devicePixelRatio();
    
    magnifierWidget->setFixedSize(w + 2*SELECT_PADDING, h + SELECT_PADDING + TOP_PADDING);
    magnifierWidget->rotationPivot = QPointF(magnifierWidget->width() / 2.0, magnifierWidget->height() - SELECT_PADDING);
    
    QPixmap pixmap = QPixmap::fromImage(magnifierWidget->image);
    magnifierWidget->crop->setPixmap(pixmap);
    magnifierWidget->rotationAngle = 0.0;
    magnifierWidget->crop->setGeometry(SELECT_PADDING, TOP_PADDING, w, h);
    magnifierWidget->crop->setFixedSize(w, h);
    int widgetW = w + 2*SELECT_PADDING;
    int widgetH = h + SELECT_PADDING + TOP_PADDING;
    int centerX = (mainWidget->width() - widgetW) / 2;
    int centerY = (mainWidget->height() - widgetH) / 2;
    magnifierWidget->move(centerX, centerY);
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
    if(!magnifierWidget) return;
    magnifierWidget->isActiveSelection = selected; // Update flag first!
    if(!selected) {
        magnifierWidget->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        magnifierWidget->stackUnder(background);
        magnifierWidget->update();
    }
    if(magnifierWidget->isHidden()) return;
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
        setCoordinateSelected(false);
        coordinateWidget->hide();
        return;
    }
    QIcon icon(":images/overlay/coordinate_plane.svg");
    coordinateWidget->image = icon.pixmap(icon.actualSize(QSize(400*scale, 400*scale))).toImage();
    int w = coordinateWidget->image.width() / mainWidget->devicePixelRatio();
    int h = coordinateWidget->image.height() / mainWidget->devicePixelRatio();
    
    int widgetW = w + 2*SELECT_PADDING;
    int widgetH = h + SELECT_PADDING + TOP_PADDING;
    coordinateWidget->setFixedSize(widgetW, widgetH);
    coordinateWidget->rotationPivot = QPointF(widgetW / 2.0, widgetH - SELECT_PADDING);
    coordinateWidget->crop->setPixmap(QPixmap::fromImage(coordinateWidget->image));
    coordinateWidget->rotationAngle = 0.0;
    coordinateWidget->crop->setGeometry(SELECT_PADDING, TOP_PADDING, w, h);
    coordinateWidget->crop->setFixedSize(w, h);
    int centerX = (mainWidget->width() - widgetW) / 2;
    int centerY = (mainWidget->height() - widgetH) / 2;
    coordinateWidget->move(centerX, centerY);
    coordinateWidget->show();
    setCoordinateSelected(true);
}

void DrawingWidget::toggleRulerTool() {
    if (!rulerWidget->isHidden()) {
        setRulerSelected(false);
        rulerWidget->hide();
        return;
    }
    QIcon icon(":images/overlay/ruler.svg");
    rulerWidget->image = icon.pixmap(icon.actualSize(QSize(600*scale, 200*scale))).toImage();
    int w = rulerWidget->image.width() / mainWidget->devicePixelRatio();
    int h = rulerWidget->image.height() / mainWidget->devicePixelRatio();
    
    int widgetW = w + 2*SELECT_PADDING;
    int widgetH = h + SELECT_PADDING + TOP_PADDING;
    rulerWidget->setFixedSize(widgetW, widgetH);
    rulerWidget->rotationPivot = QPointF(widgetW / 2.0, widgetH - SELECT_PADDING);
    rulerWidget->crop->setPixmap(QPixmap::fromImage(rulerWidget->image));
    rulerWidget->rotationAngle = 0.0;
    rulerWidget->crop->setGeometry(SELECT_PADDING, TOP_PADDING, w, h);
    rulerWidget->crop->setFixedSize(w, h);
    int centerX = (mainWidget->width() - widgetW) / 2;
    int centerY = (mainWidget->height() - widgetH) / 2;
    rulerWidget->move(centerX, centerY);
    rulerWidget->show();
    setRulerSelected(true);
}

void DrawingWidget::toggleProtractorTool() {
    if (!protractorWidget->isHidden()) {
        setProtractorSelected(false);
        protractorWidget->hide();
        return;
    }
    QIcon icon(":images/overlay/aciolcer1.svg");
    protractorWidget->image = icon.pixmap(icon.actualSize(QSize(400*scale, 400*scale))).toImage();
    int w = protractorWidget->image.width() / mainWidget->devicePixelRatio();
    int h = protractorWidget->image.height() / mainWidget->devicePixelRatio();
    
    int widgetW = w + 2*SELECT_PADDING;
    int widgetH = h + SELECT_PADDING + TOP_PADDING;
    protractorWidget->setFixedSize(widgetW, widgetH);
    protractorWidget->rotationPivot = QPointF(widgetW / 2.0, widgetH - SELECT_PADDING);
    protractorWidget->crop->setPixmap(QPixmap::fromImage(protractorWidget->image));
    protractorWidget->rotationAngle = 0.0;
    protractorWidget->crop->setGeometry(SELECT_PADDING, TOP_PADDING, w, h);
    protractorWidget->crop->setFixedSize(w, h);
    int centerX = (mainWidget->width() - widgetW) / 2;
    int centerY = (mainWidget->height() - widgetH) / 2;
    protractorWidget->move(centerX, centerY);
    protractorWidget->show();
    setProtractorSelected(true);
}

void DrawingWidget::setCoordinateSelected(bool selected) {
    if(!coordinateWidget) return;
    coordinateWidget->isActiveSelection = selected; // Update flag first!
    if(!selected) {
        coordinateWidget->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        coordinateWidget->stackUnder(background);
        coordinateWidget->update();
    }
    if(coordinateWidget->isHidden()) return;
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
    if(!rulerWidget) return;
    rulerWidget->isActiveSelection = selected; // Update flag first!
    if(!selected) {
        rulerWidget->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        rulerWidget->stackUnder(background);
        rulerWidget->update();
    }
    if(rulerWidget->isHidden()) return;
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
    if(!protractorWidget) return;
    protractorWidget->isActiveSelection = selected; // Update flag first!
    if(!selected) {
        protractorWidget->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        protractorWidget->stackUnder(background);
        protractorWidget->update();
    }
    if(protractorWidget->isHidden()) return;
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
    cropWidget->rotationAngle = 0.0;
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

    cropWidget->setFixedSize(cropRect.width() + 2*SELECT_PADDING, cropRect.height() + SELECT_PADDING + TOP_PADDING);
    cropWidget->rotationPivot = QPointF(cropWidget->width() / 2.0, cropWidget->height() - SELECT_PADDING);
    QPixmap pixmap = QPixmap::fromImage(cropWidget->image);
    cropWidget->crop->setPixmap(pixmap);
    cropWidget->crop->setGeometry(SELECT_PADDING, TOP_PADDING, cropRect.width(), cropRect.height());
    cropWidget->crop->setFixedSize(cropRect.width(), cropRect.height());
    cropWidget->move(topLeft.x() - SELECT_PADDING, topLeft.y() - TOP_PADDING);
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
    
    double dpr = mainWidget->devicePixelRatio();
    painter.begin(&image);
    // Use DestinationOver to put the selection BEHIND existing strokes
    painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
    
    QPointF pivot = cropWidget->rotationPivot;
    // Pivot in absolute canvas pixel coordinates
    QPointF globalPivotPixels = (cropWidget->pos() + pivot) * dpr;

    painter.save();
    // Translate the painter to the pivot point in pixel space
    painter.translate(globalPivotPixels);
    // Apply the rotation
    painter.rotate(cropWidget->rotationAngle);
    
    // Draw the image relative to the pivot.
    // We use the crop's local position (x,y) relative to the pivot.
    // crop->x() is SELECT_PADDING, crop->y() is TOP_PADDING.
    painter.drawImage(
        (cropWidget->crop->x() - pivot.x()) * dpr,
        (cropWidget->crop->y() - pivot.y()) * dpr,
        cropWidget->image.scaled(
            cropWidget->crop->width() * dpr,
            cropWidget->crop->height() * dpr,
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation
        )
    );
    painter.restore();
    painter.end();
    clearSelection();
    update();
}

