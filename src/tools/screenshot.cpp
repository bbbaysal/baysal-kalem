#include "../tools.h"

#include <QTimer>

#ifdef screenshot

extern bool magnifyActive;
extern int previousBackgroundType;

static void setCropScreenShot(QPixmap pix){
    if(pix.size().width() > 30 && pix.size().height() > 30){
        if(magnifyActive) {
            drawing->goNextPage(); // Go to a completely fresh page
            board->setType(WHITE);
            drawing->createMagnifierFromPixmap(pix);
            // Move to top middle
            int x = (mainWidget->width() - drawing->magnifierWidget->width()) / 2;
            int y = 50 * scale; 
            drawing->magnifierWidget->move(x, y);
            setPen(SELECTION);
        } else {
            drawing->goNextPage();
            drawing->setOverlay(pix.toImage(), drawing->getPageNum());
            board->setType(WHITE);
            board->setOverlayType(CUSTOM);
            board->rotates[drawing->getPageNum()] = 0;
            board->ratios[drawing->getPageNum()] = 100;
            board->updateTransform();
        }
    }
    setHideMainWindow(false);
    floatingWidget->show();
}

ScreenshotWidget* ssWidget;
#endif

void setupScreenShot(){
    #ifdef screenshot
    QTimer *ssTimer = new QTimer();
    QObject::connect(ssTimer, &QTimer::timeout, [=](){
        takeScreenshot();
        floatingWidget->show();
        ssTimer->stop();

    });
    toolButtons[SCREENSHOT] = create_button(SCREENSHOT, [=](){
        floatingWidget->hide();
        floatingSettings->setHide();
        ssTimer->start(500);

    });
    set_shortcut(toolButtons[SCREENSHOT], Qt::Key_F2, 0);

    ssWidget = new ScreenshotWidget();
    ssWidget->crop_signal = setCropScreenShot;

    toolButtons[SCREENSHOT_CROP] = create_button(SCREENSHOT_CROP, [=](){
        floatingWidget->hide();
        floatingSettings->setHide();
        ssWidget->showFullScreen();
        if(tool3){
            tool3->hide();
        }
    });
    set_shortcut(toolButtons[SCREENSHOT_CROP], Qt::Key_F2, Qt::ControlModifier);
    #endif
}
