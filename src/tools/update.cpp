#include "../tools.h"

int last_pen_type = 0;

void updateGui(){
    static bool updateLock = false;
    if (updateLock) return;
    updateLock = true;
    
    // hide or show elements
    colorDialog->setVisible(getPen() != ERASER || getPen() == SELECTION);
    ov->setVisible(getPen() != SELECTION);
    thicknessSlider->setVisible(getPen() != SELECTION);
    thicknessLabel->setVisible(getPen() != SELECTION);
    modeDialog->setVisible(getPen() != ERASER && getPen() != SELECTION);
    penTypeDialog->setVisible(getPen() != ERASER && getPen() != SELECTION);

    // pen and eraser menu
    toolButtons[PENMENU]->setStyleSheet("background-color: none;");
    toolButtons[MARKERMENU]->setStyleSheet("background-color: none;");
    toolButtons[ERASERMENU]->setStyleSheet("background-color: none;");
    
    extern bool magnifyActive;
    if(toolButtons[BUYUTEC] != nullptr){
        QString style = magnifyActive ? "background-color:"+drawing->penColor.name()+";" : "background-color: none;";
        toolButtons[BUYUTEC]->setStyleSheet(style);
    }

    if(toolButtons[COORDINATE] != nullptr){
        bool coordActive = drawing->coordinateWidget && !drawing->coordinateWidget->isHidden();
        QString style = coordActive ? "background-color:"+drawing->penColor.name()+";" : "background-color: none;";
        toolButtons[COORDINATE]->setStyleSheet(style);
    }

    if(toolButtons[RULER] != nullptr){
        bool rulerActive = drawing->rulerWidget && !drawing->rulerWidget->isHidden();
        QString style = rulerActive ? "background-color:"+drawing->penColor.name()+";" : "background-color: none;";
        toolButtons[RULER]->setStyleSheet(style);
    }

    set_icon(get_icon_by_id(PEN), toolButtons[PENMENU]);
    if(drawing->getPen() == PEN){
        toolButtons[PENMENU]->setStyleSheet("background-color:"+drawing->penColor.name()+";");
    } else if(drawing->getPen() == MARKER){
        toolButtons[MARKERMENU]->setStyleSheet("background-color:"+drawing->penColor.name()+";");
    } else if (drawing->getPen() == ERASER){
        toolButtons[ERASERMENU]->setStyleSheet("background-color:"+drawing->penColor.name()+";");
    }
    set_icon(get_icon_by_id(drawing->getPenStyle()), toolButtons[SHAPEMENU]);
    // Update button backgrounds
    for (auto it = penButtons.begin(); it != penButtons.end(); ++it) {
        it.value()->setStyleSheet(QString("background-color: none;"));
    }
    int btns[] = {
        getPen(), drawing->getLineStyle(),
        drawing->getPenStyle(), board->getType(),
        board->getOverlayType()
    };
    for(int btn:btns){
        if(penButtons[btn] != nullptr){
            penButtons[btn]->setStyleSheet("background-color:"+drawing->penColor.name()+";");
        }
    }
    // Update pen size
    int value = drawing->penSize[getPen()];
    thicknessLabel->setText(QString(_("Size:"))+QString(" ")+QString::number(value));


    // update go back buttons
    toolButtons[BACK]->setEnabled(drawing->isBackAvailable());
    toolButtons[NEXT]->setEnabled(drawing->isNextAvailable());
    toolButtons[PREVPAGE]->setEnabled(drawing->getPageNum() > 0);
    toolButtons[NEXTPAGE]->setEnabled(drawing->getPageNum() < drawing->max);
    pageLabel->setText(QString::number(drawing->getPageNum()));

    if(drawing->cropWidget != nullptr) {
        drawing->cropWidget->update();
    }

    // update scale buttons
    toolButtons[OVERLAYSCALEDOWN]->setEnabled(board->ratios[drawing->getPageNum()] >= 30);
    toolButtons[OVERLAYSCALEUP]->setEnabled(board->ratios[drawing->getPageNum()] <= 200);
    // update overview
    ov->updateImage();
    updateLock = false;
}



