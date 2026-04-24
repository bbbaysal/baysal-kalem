#ifndef SELECTION_H
#define SELECTION_H

#include <QWidget>
#include <QMouseEvent>

class MovableWidget : public QWidget {
public:
    enum Handle { None, TopLeft, TopRight, BottomLeft, BottomRight, DragMode, Clone, Rotate };
    float rotationAngle = 0.0;
    QImage image;
    bool isActiveSelection = true;
    QLabel* crop;
    QPointF rotationPivot;
    explicit MovableWidget(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void updateRotationBounds();

private:
    QRect initialGeometry;
    QPoint initialMousePos;
    QSize initialContentSize;
    QPointF initialRotationPivot;
    Handle activeHandle = None;
};

#endif
