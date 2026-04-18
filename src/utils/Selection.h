#ifndef SELECTION_H
#define SELECTION_H

#include <QWidget>
#include <QMouseEvent>

class MovableWidget : public QWidget {
public:
    enum Handle { None, TopLeft, TopRight, BottomLeft, BottomRight, DragMode, Clone };
    QImage image;
    bool isActiveSelection = true;
    QLabel* crop;
    explicit MovableWidget(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QRect initialGeometry;
    QPoint initialMousePos;
    Handle activeHandle = None;
};

#endif
