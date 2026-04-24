#include "FloatingSettings.h"

#include <QMainWindow>

#include "../tools.h"

extern QMainWindow* tool2;

#include <QMap>
#include <QTimer>

class SettingsPages {
public:
    void addPage(qint64 id, QWidget *data) {
        values[id] = data;
    }

    QWidget * getPage(qint64 id) {
        if (values.contains(id)) {
            return values[id];
        } else {
            return NULL;
        }
    }

private:
    QMap<qint64, QWidget*> values;
};

SettingsPages settingsPages;

FloatingSettings::FloatingSettings(QWidget *parent) : QWidget(parent) {
    layout = new QVBoxLayout(this);
    setLayout(layout);
    QString style = QString(
        "QWidget {"
        "border-radius: "+QString::number(13*scale)+"px;"
        "color: #000000;"
        "font-size: "+QString::number(22*scale)+"px;"
        "background-color: #f3232323;"
        "}"
    );
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    setStyleSheet(style);
    cur_height = 400 * scale;
    cur_width = 400 * scale;
    resize(cur_width, cur_height);
}

void FloatingSettings::addPage(QWidget *widget) {
    settingsPages.addPage(num_of_item, widget);
    layout->addWidget(widget);
    widget->hide();
    num_of_item++;
}

void FloatingSettings::reload(){
    static bool reloadLock = false;
    if (reloadLock) return;
    reloadLock = true;
    
    if(num_of_item <= current_page || current_page < 0){
        reloadLock = false;
        return;
    }
    QWidget *page = settingsPages.getPage(current_page);
    if(!page) {
        reloadLock = false;
        return;
    }

    page->show();
    
    // Combine multiple metrics to find the real required size
    QSize hint = page->sizeHint();
    QSize minHint = page->minimumSizeHint();
    
    int new_w = qMax(hint.width(), minHint.width());
    int new_h = qMax(hint.height(), minHint.height());

    // Safety minimums for Page 0 (Pen Settings) which we know is ~400x600
    if (current_page == 0) {
        if (new_w < 350 * scale) new_w = 400 * scale;
        if (new_h < 500 * scale) new_h = 600 * scale;
    } else {
        if (new_w < 100 * scale) new_w = 200 * scale;
        if (new_h < 100 * scale) new_h = 100 * scale;
    }

    if(new_w != cur_width || new_h != cur_height) {
        cur_width = new_w;
        cur_height = new_h;
        
        // Use resize first, then setFixedSize to be more friendly to window managers
        resize(cur_width, cur_height);
        setFixedSize(cur_width, cur_height);
        
        if(tool2 != nullptr) {
            tool2->resize(cur_width, cur_height);
            tool2->setFixedSize(cur_width, cur_height);
        }
    }
    reloadLock = false;
}

void FloatingSettings::setHide(){
    current_page = -1;
    if(tool2 != nullptr) {
        tool2->hide();
    } else {
        hide();
    }
}

void FloatingSettings::setPage(int num){
    if(num_of_item < num){
        return;
    }
    if(current_page == num) {
        setHide();
        return;
    }
    current_page = num;
    for(int i=0;i<num_of_item;i++){
        QWidget *p = settingsPages.getPage(i);
        if(p) p->hide();
    }
    if(tool2 != nullptr) {
        tool2->show();
    } else {
        show();
    }
    reload();
}
