#include "tools.h"
#include "tools/color.h"

#include <dirent.h>

QWidget *colorDialog;
QWidget *closeMenu;

QLabel *thicknessLabel;

QWidget *penTypeDialog;
QWidget *modeDialog;
QVBoxLayout *closeMenuLayout;
QWidget *bgMenu;


static QVBoxLayout *penSettingsLayout;
static QVBoxLayout *pageSettingsLayout;

extern void clearCache();
extern bool magnifyActive;
extern int previousBackgroundType;

void setupWidgets(){
    // Pen Settings Menu
    QWidget *penSettings = new QWidget();
    penSettingsLayout = new QVBoxLayout(penSettings);
    penSettingsLayout->setSpacing(padding);
    penSettingsLayout->setContentsMargins(padding, padding, padding, padding);
    penSettings->setStyleSheet(QString("background-color: none;"));
    floatingSettings->addPage(penSettings);


    // Pen button with menu
    toolButtons[PENMENU] = create_button(PEN, [=](){
        if(floatingSettings->current_page == 0 && drawing->getPen() != ERASER){
            floatingSettings->setHide();
            return;
        }
        if(drawing->getPen() != PEN || drawing->getPenStyle() != SPLINE){
            setPen(PEN);
            setPenStyle(SPLINE);
            floatingSettings->setHide();
            return;
        }
        floatingSettings->setPage(0);
        floatingWidget->moveAction();
    });

    // Eraser button with menu
    toolButtons[ERASERMENU] = create_button(ERASER, [=](){
        if(floatingSettings->current_page == 0 && drawing->getPen() == ERASER){
            floatingSettings->setHide();
            return;
        }
        if(drawing->getPen() != ERASER){
            setPen(ERASER);
            setPenStyle(SPLINE);
            floatingSettings->setHide();
            return;
        }
        floatingSettings->setPage(0);
        floatingWidget->moveAction();
    });

    toolButtons[SHAPEMENU] = create_button(SPLINE, [=](){
        if(floatingSettings->current_page == 0){
            floatingSettings->setHide();
            return;
        }
        if(drawing->getPen() != PEN && drawing->getPen() != MARKER){
            setPen(PEN);
        }
	      floatingSettings->setPage(0);
        floatingWidget->moveAction();
    });

    // Page settings menu
    QWidget *pageSettings = new QWidget();
    pageSettings->setStyleSheet(QString("background-color: none;"));
    pageSettingsLayout = new QVBoxLayout(pageSettings);
    pageSettingsLayout->setSpacing(padding);
    pageSettingsLayout->setContentsMargins(0, 0, 0, 0);
    floatingSettings->addPage(pageSettings);

    // Page menu button
    toolButtons[PAGEMENU] = create_button(PAGEMENU, [=](){
        if(floatingSettings->current_page == 1){
            floatingSettings->setHide();
            return;
        }
        floatingSettings->setPage(1);
        floatingWidget->moveAction();
    });

    floatingSettings->setHide();


    // Close Menu
    closeMenu = new QWidget();
    closeMenu->setStyleSheet(QString("background-color: #f3232323;"));
    closeMenuLayout = new QVBoxLayout(closeMenu);
    closeMenuLayout->setSpacing(padding);
    closeMenuLayout->setContentsMargins(0, 0, 0, 0);

    floatingSettings->addPage(closeMenu);
    QPushButton * closeButton = create_button(CLOSE, [=](){
        floatingSettings->setPage(2);
        floatingWidget->moveAction();
    });


    // Close Menu Buttons
    QPushButton *closeNo = create_button_text(_("No"), [=](){
        floatingSettings->setHide();
    });


    toolButtons[CLOSE] = create_button_text(_("Yes"), [=](){
    #ifdef ETAP19
        QStringList args3;
        QProcess p3;
        args3 << "-d" << "eta-disable-gestures@pardus.org.tr";
        p3.execute("gnome-shell-extension-tool", args3);

        QStringList args4;
        QProcess p4;
        args4 << "set" << "org.gnome.mutter" << "overlay-key" << "'SUPER_L'";
        p4.execute("gsettings", args4);
    #endif
        clearCache();
        enable_erc();
        exit(0);
    });

    // Tools Menu (Araçlar)
    QWidget *toolsMenu = new QWidget();
    toolsMenu->setStyleSheet(QString("background-color: none;"));
    QVBoxLayout *toolsMenuLayout = new QVBoxLayout(toolsMenu);
    toolsMenuLayout->setSpacing(padding);
    toolsMenuLayout->setContentsMargins(0, 0, 0, 0);

    floatingSettings->addPage(toolsMenu);

    QWidget *toolsDialog = new QWidget();
    toolsDialog->setStyleSheet(
    "QWidget {"
    "background-color: #f3232323;"
    "}"
    );

    QVBoxLayout *toolsDialogLayout = new QVBoxLayout(toolsDialog);
    
    QLabel *toolsLabel = new QLabel(_("Araçlar:"));
    toolsLabel->setStyleSheet(
        "background: none;"
        "color: #c0c0c0;"
        "padding-left: "+QString::number(8*scale)+"px;"
        "padding-top: "+QString::number(4*scale)+"px;"
    );
    toolsLabel->setAlignment(Qt::AlignLeft);
    toolsDialogLayout->addWidget(toolsLabel);

    QWidget *toolsGridDialog = new QWidget();
    toolsGridDialog->setStyleSheet("background: none; color: #c0c0c0;");
    QGridLayout *toolsGridLayout = new QGridLayout(toolsGridDialog);
    
    toolButtons[COORDINATE] = create_button(COORDINATE, [=](){
        drawing->toggleCoordinateTool();
        setPen(SELECTION);
        floatingSettings->setHide();
    });
    
    toolButtons[RULER] = create_button(RULER, [=](){
        drawing->toggleRulerTool();
        setPen(SELECTION);
        floatingSettings->setHide();
    });

    toolButtons[ACIOLCER] = create_button(ACIOLCER, [=](){
        drawing->toggleProtractorTool();
        setPen(SELECTION);
        floatingSettings->setHide();
    });
    
    toolsGridLayout->addWidget(toolButtons[COORDINATE], 0, 0, Qt::AlignCenter);
    toolsGridLayout->addWidget(toolButtons[RULER],      0, 1, Qt::AlignCenter);
    toolsGridLayout->addWidget(toolButtons[ACIOLCER],   1, 0, Qt::AlignCenter);

    toolsGridDialog->setFixedSize(
        400*scale,
        butsize*2+ padding*3
    );

    toolsDialogLayout->addWidget(toolsGridDialog);
    toolsMenuLayout->addWidget(toolsDialog);


/********** Main toolbar **********/

    toolButtons[CURSOR] = create_button(CURSOR, [=](){
        setPen(SELECTION);
        floatingSettings->setHide();
    });

    floatingWidget->addWidget(toolButtons[MINIFY], 6);
    floatingWidget->addWidget(toolButtons[PENMENU], 6);
    
    // Add Marker Button
    toolButtons[MARKERMENU] = create_button(MARKER, [=](){
        setPen(MARKER);
        setPenStyle(SPLINE);
        floatingSettings->setHide();
    });
    floatingWidget->addWidget(toolButtons[MARKERMENU], 6);
    
    floatingWidget->addWidget(toolButtons[ERASERMENU], 6);
    
    floatingWidget->addWidget(toolButtons[SHAPEMENU], 3);
    floatingWidget->addWidget(penButtons[SELECTION], 3);
    
    floatingWidget->addWidget(toolButtons[BACK], 3);
    floatingWidget->addWidget(toolButtons[NEXT], 3);

    // Color Grid (3x3)
    QColor gridColors[] = {
        QColor("#ff0000"), QColor("#0068ff"), QColor("#00bc3f"), // Red, Blue, Green
        QColor("#000000"), QColor("#FFFFFF"), QColor("#ff7f00"), // Black, White, Orange
        QColor("#7a00e5"), QColor("#ffff00"), QColor("#8B4513")  // Purple, Yellow, Brown
    };
    for(int i=0; i<9; i++) {
        QPushButton* cb = create_color_button(gridColors[i], false, "");
        cb->setFixedSize(butsize * 0.8, butsize * 0.8);
        floatingWidget->addWidget(cb, 2);
    }

    toolButtons[UTILMENU] = create_button(UTILMENU, [=](){
        if(floatingSettings->current_page == 3){
            floatingSettings->setHide();
            return;
        }
        floatingSettings->setPage(3);
        floatingWidget->moveAction();
    });

    floatingWidget->addWidget(toolButtons[CLEAR], 3);
    floatingWidget->addWidget(toolButtons[UTILMENU], 3);
    
    floatingWidget->addWidget(closeButton, 3);
    floatingWidget->addWidget(toolButtons[PAGEMENU], 3);

    toolButtons[BUYUTEC] = create_button(BUYUTEC, [=](){
        if(magnifyActive){
            // Toggle OFF
            magnifyActive = false;
            drawing->clearMagnifier();
            setPen(PEN); // Kalem moduna geri dön
            if(previousBackgroundType != -1){
                board->setType(previousBackgroundType);
            }
        } else {
            // Toggle ON
            magnifyActive = true;
            previousBackgroundType = board->getType();
            #ifdef screenshot
            floatingWidget->hide();
            floatingSettings->setHide();
            if(tool3) tool3->hide();
            ssWidget->showFullScreen();
            #endif
        }
    });
    floatingWidget->addWidget(toolButtons[BUYUTEC], 6);
    
    // Branding
    floatingWidget->addLabel("bbbaysal", QColor("red"));

    // disable minify if fual enabled
    toolButtons[MINIFY]->setEnabled(!get_bool("fuar"));

/*********** main menu done *********/

/********** desktop toolbar **********/

    if(desktopWidget){
        desktopWidget->num_of_rows = 1;
        desktopWidget->addWidget(toolButtons[UNMINIFY]);
    }
/********** desktop menu done **********/


/************************************/
/************ Pen Menu **************/

/********** Pen Settings **********/
    // color dialog
    int num_of_color = sizeof(colors) / sizeof(QColor);
    int rowsize = 7;
    colorDialog = new QWidget();
    colorDialog->setFixedSize(
        butsize*rowsize + padding*(rowsize+1),
        butsize*3 + padding*4
    );


/********** Overview **********/

    penSettingsLayout->addWidget(ov, Qt::AlignCenter);


/********** Thickness slider **********/

    QWidget *penSizeSettings = new QWidget();
    QVBoxLayout *penSizeSettingsLayout = new QVBoxLayout(penSizeSettings);
    penSettingsLayout->addWidget(penSizeSettings, Qt::AlignCenter);


    penSizeSettings->setStyleSheet(
    "QWidget {"
    "background-color: #f3232323;"
    "}"
    );


    thicknessLabel = new QLabel(_("Size: 10"));
    thicknessLabel->setStyleSheet(
        "background: none;"
        "color: #9e9e9e;"
        "padding-left: "+QString::number(8*scale)+"px;"
        "padding-top: "+QString::number(4*scale)+"px;"
    );
    thicknessLabel->setAlignment(Qt::AlignLeft);
    penSizeSettingsLayout->addWidget(thicknessLabel, Qt::AlignLeft);

    //thicknessLabel->setFixedSize(
    //    colorDialog->size().width(),
    //    butsize / 2
    //);

    thicknessSlider->setFixedSize(
        colorDialog->size().width() - 2*padding,
        butsize
    );
    penSizeSettingsLayout->addWidget(thicknessSlider, Qt::AlignHCenter | Qt::AlignVCenter);

    thicknessSlider->setStyleSheet(
         "QWidget {"
         "background: none;"
         "}"
         "QSlider::groove:horizontal {"
            "border: 1px solid #bbb;"
            "background: white;"
            "height: "+QString::number(16*scale)+"px;"
            "border-radius: "+QString::number(8*scale)+"px;"
        "}"
        "QSlider::handle:horizontal {"
            "background: #fff;"
            "width: "+QString::number(44*scale)+"px;"
            "border-radius: "+QString::number(8*scale)+"px;"
            "margin: -4px 0;"
        "}"
        "QSlider::handle:horizontal:hover {"
            "background: #ccc;"
        "}"
        "QSlider::sub-page:horizontal {"
            "background: #4a90e2;"
            "border-radius: 5px;"
        "}"
        "QSlider::add-page:horizontal {"
            "background: #4a4a4a;"
            "border-radius:5px;"
        "}"
    );


/********** penTypes **********/
    QWidget *penTypeMainWidget = new QWidget();
    QHBoxLayout *penTypeMainLayout = new QHBoxLayout(penTypeMainWidget);
    penTypeMainWidget->setStyleSheet("background: none;");

    QWidget *styleDialog = new QWidget();
    QGridLayout *styleLayout = new QGridLayout(styleDialog);

    styleLayout->addWidget(penButtons[PEN],           0, 0, Qt::AlignCenter);
    styleLayout->addWidget(penButtons[MARKER],        0, 1, Qt::AlignCenter);
    styleLayout->addWidget(penButtons[ERASER],        0, 2, Qt::AlignCenter);
    //styleLayout->addWidget(toolButtons[CLEAR],        0, 3, Qt::AlignCenter);

    penSettingsLayout->addWidget(styleDialog, Qt::AlignCenter);

    penTypeDialog = new QWidget();
    QGridLayout *penTypeLayout = new QGridLayout(penTypeDialog);
    penTypeLayout->addWidget(penButtons[NORMAL],   0, 0, Qt::AlignCenter);
    penTypeLayout->addWidget(penButtons[DOTLINE],  0, 1, Qt::AlignCenter);
    penTypeLayout->addWidget(penButtons[LINELINE], 0, 2, Qt::AlignCenter);


    penTypeMainLayout->addWidget(styleDialog);
    penTypeMainLayout->addWidget(penTypeDialog);

    penSizeSettingsLayout->addWidget(penTypeMainWidget, Qt::AlignCenter);

/********** penModes **********/
    modeDialog = new QWidget();
    QGridLayout *modeLayout = new QGridLayout(modeDialog);
    // spline
    modeLayout->addWidget(penButtons[SPLINE],     0, 0);
    modeLayout->addWidget(penButtons[LINE],       0, 1);
    modeLayout->addWidget(penButtons[CIRCLE],     0, 2);
    modeLayout->addWidget(penButtons[TRIANGLE],   0, 3);
    modeLayout->addWidget(penButtons[RECTANGLE],  0, 4);
    modeLayout->addWidget(penButtons[VECTOR],     0, 5);
    modeLayout->addWidget(penButtons[VECTOR2],    0, 6);

    modeLayout->addWidget(penButtons[LINE_ORTHO],    1, 1);
    modeLayout->addWidget(penButtons[VECTOR_ORTHO],  1, 5);
    modeLayout->addWidget(penButtons[VECTOR2_ORTHO], 1, 6);

    modeDialog->setStyleSheet("background-color: #f4141414;");

    penSizeSettingsLayout->addWidget(modeDialog, Qt::AlignCenter);


/********** Color selection options **********/
    // color selection

    colorDialog->setStyleSheet(
    "QWidget {"
    "background-color: #f3232323;"
    "}"
    );


    QGridLayout *gridLayout = new QGridLayout(colorDialog);

    // Create buttons for each color
    gridLayout->addWidget(toolButtons[COLORPICKER], 0, 0, Qt::AlignCenter);

    // Color button offset is 100
    for (int i = 0; i < num_of_color; i++) {
        toolButtons[i+100] = create_color_button(colors[i], true, "");
        gridLayout->addWidget(toolButtons[i+100], (i+1) / rowsize, (i+1) % rowsize, Qt::AlignCenter);
    }
    colorDialog->setLayout(gridLayout);
    penSettingsLayout->addWidget(colorDialog, Qt::AlignCenter);

    // sync all other widget widths
    int calculatedWidth = butsize*7 + padding*(7+1);
    ov->setFixedSize(
        calculatedWidth,
        calculatedWidth/2
    );
    penSizeSettings->setFixedWidth(calculatedWidth);

    // Ensure penSettings has a base size to avoid layout engine hangs
    penSettings->setMinimumSize(calculatedWidth + 2*padding, 600*scale);


/********** pen type **********/

    // resize color dialog
    colorDialog->setFixedHeight(
        butsize*3+ padding*4
    );


/************ Page Menu ************/

 /********** page number **********/

    QWidget *pageMenu = new QWidget();
    QVBoxLayout *pageMenuLayout = new QVBoxLayout(pageMenu);

    pageMenu->setStyleSheet(
        "QWidget {"
          "background-color: #f3232323;"
        "}"
    );


    QWidget *pageNumWidget = new QWidget();
    QHBoxLayout *pageNumLayout = new QHBoxLayout(pageNumWidget);


    pageNumWidget->setStyleSheet("background: none;");

    QLabel* pgLabel = new QLabel(_("Page:"));
    pgLabel->setStyleSheet(
        "background: none;"
        "color: #c0c0c0;"
        "padding-left: "+QString::number(8*scale)+"px;"
        "padding-top: "+QString::number(4*scale)+"px;"
    );

    pageMenuLayout->addWidget(pgLabel);

    pageNumLayout->addWidget(toolButtons[PREVPAGE], Qt::AlignLeft);
    pageNumLayout->addWidget(pageLabel, Qt::AlignLeft);
    pageNumLayout->addWidget(toolButtons[NEXTPAGE], Qt::AlignLeft);

    pageLabel->setFixedSize(butsize, butsize);

    pageLabel->setStyleSheet(
        "background: none;"
        "color: #c0c0c0;"
    );



    pageNumLayout->addWidget(penButtons[TRANSPARENT], Qt::AlignRight);
    pageNumLayout->addWidget(penButtons[BLACK], Qt::AlignRight);
    pageNumLayout->addWidget(penButtons[WHITE], Qt::AlignRight);

    pageMenuLayout->addWidget(pageNumWidget);
    pageSettingsLayout->addWidget(pageMenu);

/********** page type **********/

    bgMenu = new QWidget();
    QVBoxLayout *bgMenuLayout = new QVBoxLayout(bgMenu);

    bgMenu->setStyleSheet(
        "QWidget {"
          "background-color: #f3232323;"
        "}"
    );



    QLabel *bgLabel = new QLabel(_("Background:"));
    bgLabel->setStyleSheet(
        "background: none;"
        "color: #c0c0c0;"
        "padding-left: "+QString::number(8*scale)+"px;"
        "padding-top: "+QString::number(4*scale)+"px;"
    );
    bgLabel->setAlignment(Qt::AlignLeft);
    bgMenuLayout->addWidget(bgLabel);

    QWidget *pageDialog = new QWidget();
    QGridLayout *pageLayout = new QGridLayout(pageDialog);

    pageDialog->setStyleSheet(
        "background: none;"
        "color: #c0c0c0"
    );


    // spline
    pageLayout->addWidget(penButtons[BLANK],     0, 0, Qt::AlignCenter);
    pageLayout->addWidget(penButtons[SQUARES],   0, 1, Qt::AlignCenter);
    pageLayout->addWidget(penButtons[LINES],     0, 2, Qt::AlignCenter);
    pageLayout->addWidget(penButtons[ISOMETRIC], 0, 3, Qt::AlignCenter);
    pageLayout->addWidget(penButtons[MUSIC],     0, 4, Qt::AlignCenter);
    pageLayout->addWidget(penButtons[CUSTOM],    0, 5, Qt::AlignCenter);

    struct dirent *ep;
    DIR *dp = opendir (BGDIR);
    int i = 6;
    // custom overlay button offset is 200
    if (dp != NULL) {
        while ((ep = readdir (dp)) != NULL) {
            if ((ep->d_name)[0] == '.') {
                continue;
            }
            QString path = QString(BGDIR) + QString("/") + QString(ep->d_name);
            toolButtons[i+200] = create_button(0, [=](){
                drawing->setOverlay(QImage(path), drawing->getPageNum());
                board->setOverlayType(CUSTOM);
                board->rotates[drawing->getPageNum()] = 0;
                board->ratios[drawing->getPageNum()] = 100;
                board->updateTransform();
            });
            set_icon(path.toStdString().c_str(), toolButtons[i+200]);
            pageLayout->addWidget(toolButtons[i+200], i / 6, i % 6, Qt::AlignCenter);
            i++;
            //printf ("%s\n", ep->d_name);
        }
        closedir(dp);
    }
    pageDialog->setFixedSize(
        colorDialog->size().width(),
        butsize*2+ padding*3
    );
    bgMenuLayout->addWidget(pageDialog);

    pageSettingsLayout->addWidget(bgMenu);


/********** clear & screenshot **********/

    QWidget *miscDialog = new QWidget();
    QWidget *utilDialog = new QWidget();

    miscDialog->setStyleSheet(
    "QWidget {"
    "background-color: #f3232323;"
    "}"
    );


    utilDialog->setStyleSheet(
    "QWidget {"
    "background-color: #f3232323;"
    "}"
    );

    QGridLayout *miscLayout = new QGridLayout(miscDialog);
    miscLayout->addWidget(toolButtons[OVERLAYROTATEUP],    0, 0, Qt::AlignCenter);
    miscLayout->addWidget(toolButtons[OVERLAYSCALEUP],     0, 1, Qt::AlignCenter);
    miscLayout->addWidget(toolButtons[OVERLAYSCALEDOWN],   0, 2, Qt::AlignCenter);
    miscLayout->addWidget(toolButtons[OVERLAYROTATEDOWN],  0, 3, Qt::AlignCenter);

    pageSettingsLayout->addWidget(miscDialog);

    QGridLayout *utilLayout = new QGridLayout(utilDialog);
    utilLayout->addWidget(toolButtons[SAVE],       0, 0, Qt::AlignCenter);
    utilLayout->addWidget(toolButtons[OPEN],       0, 1, Qt::AlignCenter);
    utilLayout->addWidget(toolButtons[FULLSCREEN], 0, 2, Qt::AlignCenter);
    utilLayout->addWidget(toolButtons[ROTATE],     0, 3, Qt::AlignCenter);
    #ifdef screenshot
    utilLayout->addWidget(toolButtons[SCREENSHOT],  0, 4, Qt::AlignCenter);
    #endif

    pageSettingsLayout->addWidget(utilDialog);

    if(get_bool("fuar")){
        toolButtons[SAVE]->setEnabled(false);
        toolButtons[OPEN]->setEnabled(false);
        toolButtons[CLOSE]->setEnabled(false);
        toolButtons[SAVE]->setEnabled(false);
    }

/********** Close Menu **********/

    QWidget *closeMenuMain = new QWidget();
    QVBoxLayout *closeMenuMainLayout = new QVBoxLayout(closeMenuMain);

    QWidget *closeMenuConfirm = new QWidget();
    QHBoxLayout *closeMenuConfirmLayout = new QHBoxLayout(closeMenuConfirm);

    closeMenuLayout->addWidget(closeMenuMain);
    closeMenuLayout->addWidget(closeMenuConfirm);

    closeMenuMain->setStyleSheet(
        "background: none;"
    );

    closeMenuConfirm->setStyleSheet(
        "background: none;"
    );

    QLabel *closeIcon = new QLabel();
    QLabel *closeLabel = new QLabel(_("Are you sure you want to quit?"));

    closeIcon->setPixmap(QIcon(get_icon_by_id(CLOSE)).pixmap(QSize(butsize*2, butsize*2)));

    closeMenuMainLayout->addWidget(closeIcon, 0, Qt::AlignCenter);


    closeIcon->setStyleSheet(
        "background: none;"
        "padding-top: "+QString::number(butsize/2)+"px;"
        "padding-bottom: "+QString::number(butsize/2)+"px;"
    );

    closeLabel->setStyleSheet(
        "background: none;"
        "color: #c0c0c0;"
        "padding-left: "+QString::number(8*scale)+"px;"
        "padding-right: "+QString::number(8*scale)+"px;"
    );
    closeLabel->setAlignment(Qt::AlignLeft);
    closeMenuMainLayout->addWidget(closeLabel);

    closeNo->setStyleSheet(
        "background: #f4141414;"
        "color: #c0c0c0;"
        "padding-top: "+QString::number(butsize/3)+"px;"
        "padding-bottom: "+QString::number(butsize/3)+"px;"
    );

    toolButtons[CLOSE]->setStyleSheet(
        "background: #f4141414;"
        "color: #c0c0c0;"
        "padding-top: "+QString::number(butsize/3)+"px;"
        "padding-bottom: "+QString::number(butsize/3)+"px;"
    );

    srand(time(NULL));
    if(rand() % 2 == 0) {
        closeMenuConfirmLayout->addWidget(toolButtons[CLOSE]);
        closeMenuConfirmLayout->addWidget(closeNo);
    } else {
        closeMenuConfirmLayout->addWidget(closeNo);
        closeMenuConfirmLayout->addWidget(toolButtons[CLOSE]);
    }

/********** Finish him **********/
    updateGui();
    pageLabel->setStyleSheet(
    "color: #c0c0c0;"
    "font-size: "+QString::number(31*scale)+"px;"
    );
}
