#include <QLabel>
#include "FloatingWidget.h"


#include "../tools.h"


extern float scale;

extern QMainWindow* tool2;

#define padding 8*scale


#ifdef QT5
#define globalPosition globalPos
#endif

FloatingWidget::FloatingWidget(QWidget *parent) : QWidget(parent) {
    tool = (QMainWindow*)parent;
    layout = new QGridLayout();
    new_x = get_int("cur-x");
    new_y = get_int("cur-y");
    layout->setSpacing(2 * scale);
    layout->setContentsMargins(4 * scale, 8 * scale, 4 * scale, 8 * scale);
    QString style = QString(
    "QWidget {"
        "border-radius: "+QString::number(13*scale)+"px;"
        "background-color: #f3232323;"
    "}");
    setLayout(layout);
    setStyleSheet(style);
    cur_height = padding;
    for(int i=0;i<num_of_rows;i++){
        QLabel *label = new QLabel();
        label->setFixedSize(1, 1); // Minimize dummy labels
        label->setStyleSheet(QString("background-color: none;"));
        widgets.insert(i, label);
        spans[label] = 1;
    }
}


void FloatingWidget::setMainWindow(QWidget *widget) {
    mainWindow = (QMainWindow*)widget;
}

void FloatingWidget::setSettings(QWidget *widget) {
    floatingSettings = (FloatingSettings*)widget;
}

void FloatingWidget::addWidget(QWidget *widget, int span) {
    if (span <= 0) span = num_of_rows; // Default to full width
    widgets.insert(num_of_item + num_of_rows, widget);
    spans[widget] = span;
    num_of_item++;
    setVertical(is_vertical);
    moveAction();
}

#define STR(x) #x
#define XSTR(x) STR(x)

void FloatingWidget::addLabel(QString text, QColor color) {
    brandingLabel = new QLabel(text);
    brandingLabel->setAlignment(Qt::AlignCenter);
    brandingLabel->setText("<div style='text-align: center;'><a href=\"https://bekirberginbaysal.blogspot.com/\" style=\"color: " + color.name() + "; text-decoration: none; font-size: " + QString::number(14 * scale) + "px;\">" + text + "</a><br><span style=\"font-size: " + QString::number(6 * scale) + "px; color: red; font-weight: normal; line-height: 100%;\">v4.2.5</span></div>");
    brandingLabel->setOpenExternalLinks(true);
    brandingLabel->setStyleSheet(QString("background: none; font-weight: bold;"));
    addWidget(brandingLabel, num_of_rows);
}
void FloatingWidget::setVertical(bool state) {
    // remove all items
    QLayoutItem *child;
    while ((child = layout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            layout->removeWidget(child->widget());
        } else {
            layout->removeItem(child);
        }
    }

    // add items with spans
    int current_column = 0;
    int current_row = 0;
    
    for (int i = 0; i < num_of_item + num_of_rows; i++) {
        QWidget *w = widgets[i];
        int span = spans.value(w, 1); // Dummy labels get span 1
        
        if (current_column + span > num_of_rows) {
            current_column = 0;
            current_row++;
        }
        
        if (state) {
            layout->addWidget(w, current_row, current_column, 1, span, Qt::AlignCenter);
        } else {
            layout->addWidget(w, current_column, current_row, span, 1, Qt::AlignCenter);
        }
        current_column += span;
    }
    
    int total_rows = current_row + 1;
    int height = total_rows * (butsize + padding) + padding;
    int width = 3 * (butsize * 0.85); 
    // tightened width to bring colors closer together
    
    if (state) {
        setFixedSize(width, height);
    } else {
        setFixedSize(height, width);
    }
    
    if (!is_wayland) {
        tool->setFixedSize(size());
    }
}


void FloatingWidget::moveAction(){
        debug("Move Action %d %d\n", new_x, new_y);
        if (new_x < 0) {
            new_x = 0;
        }if (new_y < 0) {
            new_y = 0;
        }
        int max_width = mainWindow->geometry().width();
        int max_height = mainWindow->geometry().height();
        if(!is_wayland){
            max_width = QGuiApplication::primaryScreen()->size().width();
            max_height = QGuiApplication::primaryScreen()->size().height();
        }
        if (new_x >  max_width- size().width()) {
            new_x = max_width - size().width();
        }if (new_y > max_height - size().height()) {
            new_y = max_height - size().height();
        }
        if(!is_wayland){
            tool->move(new_x, new_y);
        } else {
            move(new_x, new_y);
        }
        if(floatingSettings != NULL){
            new_xx = new_x+padding+size().width();
            if(new_xx  > max_width - floatingSettings->cur_width){
                new_xx = new_x - padding - floatingSettings->cur_width;
            }
            new_yy = new_y;
            if (new_yy > max_height - floatingSettings->cur_height) {
                new_yy = max_height - floatingSettings->cur_height;
            }
             if(!is_wayland){
                tool2->move(new_xx, new_yy);
            } else {
                floatingSettings->move(new_xx, new_yy);
            }
        }
}
void FloatingWidget::mousePressEvent(QMouseEvent *event) {
    offset_x = abs(event->globalPosition().x() - new_x);
    offset_y = abs(event->globalPosition().y() - new_y);
}

void FloatingWidget::mouseReleaseEvent(QMouseEvent *event) {
    (void)(event); // fix unused warning
    offset_x =-1;
    offset_y =-1;
    set_int("cur-x", new_x);
    set_int("cur-y", new_y);
}

void FloatingWidget::mouseMoveEvent(QMouseEvent *event) {
    if(offset_x < 0 || offset_y < 0){
        return;
    }
    if (event->buttons() & Qt::LeftButton) {
        new_x = event->globalPosition().x() - offset_x;
        new_y = event->globalPosition().y() - offset_y;
        moveAction();
        event->accept();
    }
}
