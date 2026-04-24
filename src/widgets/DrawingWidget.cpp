#include "DrawingWidget.h"
#include "WhiteBoard.h"

#ifdef QPRINTER
#include <QPrinter>
#endif
#ifdef LIBARCHIVE
#include "../utils/Archive.h"
#endif
#include <stdio.h>


#include <stdlib.h>
#include <locale.h>
#include <libintl.h>

#define _(String) gettext(String)

#include "../tools.h"

extern WhiteBoard *board;
extern QWidget * mainWidget;
extern DrawingWidget *drawing;
extern FloatingSettings *floatingSettings;
QString cache_main;
QString cache;

void removeDirectory(const QString &path);

/*
penType:
 - 0 eraser
 - 1 pen
 - 2 marker
*/

#include <QMap>

#ifdef QT5
#define points touchPoints
#define position pos
#endif


class CursorStorage {
public:
    void init(qint64 id){
        if(!images.contains(id)){
            labels[id] = new QLabel("");
            labels[id]->setStyleSheet(QString("background-color: none;"));
            images[id] = new QWidget(mainWidget);
            layouts[id] = new QVBoxLayout(images[id]);
            layouts[id]->addWidget(labels[id]);
            layouts[id]->setContentsMargins(0,0,0,0);
            layouts[id]->setSpacing(0);
            sizes[id] = 0;
        }
    }
    void setPosition(qint64 id, QPointF data) {
        init(id);
        //printf("%lld move\n", id);
        images[id]->move(QPoint(
            data.x() - sizes[id]/2,
            data.y() - sizes[id]/2
        ));
        images[id]->show();
        setCursor(id, sizes[id]);
    }

    void setCursor(qint64 id, int size){
        init(id);
        if(sizes[id] == size && lastDrawnPenType.contains(id) && lastDrawnPenType[id] == penType[id]){
            return;
        }
        sizes[id] = size;
        lastDrawnPenType[id] = penType[id];
        
        QIcon icon;
        if (penType[id] == ERASER) {
            icon = QIcon(":images/cursor.svg");
        } else {
            icon = QIcon(get_icon_by_id(CURSOR));
        }

        QPixmap pixmap = icon.pixmap(
            icon.actualSize(
                QSize(size, size)
            )
        );
        //printf("%lld resize %d\n", id, size);
        labels[id]->setFixedSize(size, size);
        images[id]->setFixedSize(size, size);
        labels[id]->setPixmap(pixmap);
    }

    void clear(){
        images.clear();
    }

    void hide(qint64 id){
        init(id);
        images[id]->hide();
    }
    QMap<qint64, bool> drawing;
    QMap<qint64, int> penType;
    QMap<qint64, QWidget*> images;
    QMap<qint64, QLabel*> labels;
    QMap<qint64, QVBoxLayout*> layouts;
    QMap<qint64, int> sizes;
    QMap<qint64, int> lastDrawnPenType;
};
CursorStorage curs;

class ImageStorage {
public:
    int last_image_num = 1;
    int image_count = 0;
    int pageType = TRANSPARENT;
    int overlayType = BLANK;
    int removed = 0;
    ImageStorage(){
        last_image_num = 1;
        image_count = 0;
        pageType = TRANSPARENT;
        overlayType = BLANK;
        removed = 0;
    }
    void saveValue(qint64 id, QImage data) {
        QDir dir;
        dir.mkpath(cache_path);
        bool res = saveImageToFile(data, cache_path+QString::number(id));
        printf("saveValue id=%lld path=%s success=%d\n", id, qPrintable(cache_path+QString::number(id)), res);
        updateGui();

        if(id > history){
            remove(id - history);
            removed++;
        }
    }

    void clear(){
        image_count = 0;
        last_image_num = 1;
        removed = 0;
        updateGui();
        removeDirectory(cache_path);
    }

    QImage loadValue(qint64 id) {
        if(removed >= id) {
            id =  removed +1;
        }
        QString filePath = cache_path+QString::number(id);
        QFile file(filePath);
        if(file.exists()){
            printf("loadValue id=%lld EXISTS path=%s\n", id, qPrintable(filePath));
            return loadImageFromFile(cache_path+QString::number(id));
        } else {
            printf("loadValue id=%lld NOT_EXISTS path=%s\n", id, qPrintable(filePath));

            QPixmap pix = QPixmap(mainWidget->size()*mainWidget->devicePixelRatio());
            pix.fill(QColor("transparent"));
            pix.setDevicePixelRatio(mainWidget->devicePixelRatio());
            return pix.toImage();
        }
    }

    void remove(qint64 id){
        QString filePath = cache_path+QString::number(id);
        QFile file(filePath);
        if(file.exists()){
            file.remove();
        }
    }

    QString cache_path;
    void initCache() {
        cache_path = cache + generateRandomString(10) + "/";
        QDir dir;
        dir.mkpath(cache_path);
    }
};
ImageStorage images;


class PageStorage {
public:
    int last_page_num = 0;
    int page_count = 0;
    void saveValue(qint64 id, ImageStorage data) {
        values[id] = data;
    }

    void clear(){
        values.clear();
        last_page_num = 0;
        page_count = 0;
    }
#ifdef QPRINTER
    void savePdf(const QString& filename){
        saveValue(last_page_num, images);
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filename);
        printer.setFullPage(true);
        QSizeF imageSize(mainWidget->geometry().width(),mainWidget->geometry().height());
        QPageSize pageSize(imageSize, QPageSize::Point);
        printer.setPageSize(pageSize);
        printer.setResolution(72); // TODO: replace this
        QPainter painter(&printer);
        for(int i=0;i<=page_count;i++){
            QImage im = loadValue(i).loadValue(loadValue(i).last_image_num);
            im.setDevicePixelRatio(mainWidget->devicePixelRatio());
            painter.drawPixmap(0,0, board->backgrounds[i]);
            painter.drawImage(0,0, im);
            printer.newPage();
        }
        painter.end();
    }
#endif
#ifdef LIBARCHIVE
    void saveAll(const QString& filename){
        images.overlayType = board->getOverlayType();
        images.pageType = board->getType();
        values[last_page_num] = images;
        QString cfg = "[main]\n";
        cfg += "width="+QString::number(mainWidget->geometry().width())+"\n";
        cfg += "height="+QString::number(mainWidget->geometry().height())+"\n";
        for(int i=0;i<=page_count;i++){
            cfg += "[page"+QString::number(i)+"]\n";
            cfg += "overlay="+get_overlay_by_id(loadValue(i).overlayType)+"\n";
            cfg += "ratio="+QString::number(board->ratios[i])+"\n";
            cfg += "rotate="+QString::number(board->rotates[i])+"\n";
            cfg += "page="+get_overlay_by_id(loadValue(i).pageType)+"\n";
            archive_add(QString::number(i)+"/background", drawing->getOverlay(i));
            for(int j=1+loadValue(i).removed;j<=loadValue(i).image_count;j++){
                archive_add(QString::number(i)+"/"+QString::number(j-1-loadValue(i).removed), values[i].loadValue(j));
            }
        }
        archive_set_config(cfg);
#ifdef QPRINTER
        if(PDFMODE){
            archive_add_file("overlay.pdf", drawing->pdfPath);
        }
#endif
        archive_create(filename);
    }

    void loadArchive(const QString& filename){
        QMap<QString, QImage> archive = archive_load(filename);
        QString cfg = archive_get_config();
        clear();
        for (auto it = archive.begin(); it != archive.end(); ++it) {
            QString path = it.key();
            QStringList parts = path.split("/");
            QImage image = it.value();
            int page = parts[0].toInt();
            if(path.endsWith("background")){
                drawing->setOverlay(image, page);
                printf("Load: page bg: %d\n", page);
                continue;
            }
            int frame = parts[1].toInt();
            printf("Load: page: %d frame %d\n", page, frame);
            if(page > page_count){
                page_count = page;
            }
            if (!values.contains(page)) {
                ImageStorage data;
                values[page] = data;
                values[page].image_count = 0;
                values[page].last_image_num = 1;
            }
            image.setDevicePixelRatio(mainWidget->devicePixelRatio());
            values[page].saveValue(frame+1, image);
            values[page].image_count++;
            values[page].last_image_num = values[page].image_count;
        }
        QStringList list = cfg.split("\n");
        QString area = "main";
        int page = 0;
        for (const auto &str : list) {
            if(str.startsWith("[") && str.endsWith("]")) {
                area = str.mid(1,str.length()-2);
                if(area.startsWith("page")){
                    page = area.mid(4,str.length()-1).toInt();
                }
            } else if(str.startsWith("overlay")){
                values[page].overlayType = get_id_by_overlay(str.split("=")[1]);
                debug("Load: page: %d overlay %d\n", page, values[page].overlayType);
            } else if(str.startsWith("page")){
                values[page].pageType = get_id_by_overlay(str.split("=")[1]);
                debug("Load: page: %d page %d\n", page, values[page].pageType);
            } else if(str.startsWith("ratio")){
                board->ratios[page] = str.split("=")[1].toInt();
                debug("Load: page: %d ratio %d\n", page, board->ratios[page]);
            } else if(str.startsWith("rotate")){
                board->rotates[page] = str.split("=")[1].toInt();
                debug("Load: page: %d rotates %d\n", page, board->rotates[page]);
            }
        }
        images = values[0];
        board->setType(images.pageType);
        board->setOverlayType(images.overlayType);
        drawing->loadImage(images.last_image_num);
        drawing->update();
        drawing->goPage(0);
        updateGui();
    }
#endif

    ImageStorage loadValue(qint64 id) {
        if (id > page_count){
            page_count = id;
        }
        if (values.contains(id)) {
            return values[id];
        } else {
            ImageStorage imgs = ImageStorage();
            imgs.initCache();
            imgs.pageType = board->getType();
            return imgs;
        }
    }

private:
    QMap<qint64, ImageStorage> values;
};
PageStorage pages;

int curEventButtons = 0;

DrawingWidget::DrawingWidget(QWidget *parent): QWidget(parent) {
    if (cache_main.isEmpty()) {
        cache_main = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/baysal-kalem/";
        cache = cache_main + generateRandomString(5)+"/";
        images.initCache();
        // Clear all previous caches properly across app restarts
        removeDirectory(cache_main);
        QDir dir;
        dir.mkpath(images.cache_path);
    }
    initializeImage(size());
    penSize[PEN] = get_int("pen-size");
    penSize[ERASER] = get_int("eraser-size");
    penSize[MARKER] = get_int("marker-size");
    penType=PEN;
    penStyle=SPLINE;
    lineStyle=NORMAL;
    penColor = QColor(get_string("color"));
    setMouseTracking(true);
    setAttribute(Qt::WA_AcceptTouchEvents);
    num_of_press = 0;
    cropWidget = new MovableWidget(mainWidget);
    cropWidget->hide();

    magnifierWidget = new MovableWidget(mainWidget);
    magnifierWidget->stackUnder(this);
    magnifierWidget->hide();

    coordinateWidget = new MovableWidget(mainWidget);
    coordinateWidget->stackUnder(this);
    coordinateWidget->hide();

    rulerWidget = new MovableWidget(mainWidget);
    rulerWidget->stackUnder(this);
    rulerWidget->hide();

    protractorWidget = new MovableWidget(mainWidget);
    protractorWidget->stackUnder(this);
    protractorWidget->hide();

    //QScreen *screen = QGuiApplication::primaryScreen();
    fpressure = get_int((char*)"pressure") / 100.0;

    setFocusPolicy(Qt::StrongFocus);
    // removeDirectory(cache); is removed to track cache properly and not delete it during execution
}

void DrawingWidget::addPoint(int id, QPointF data) {
    if(geo.size(id) == 0) {
        geo.saveValue(id, 0, data);
        return;
    }
    if((lineStyle ==  NORMAL && penStyle == SPLINE) || penType == ERASER){
        geo.saveValue(id, 1, geo.load(id).loadValue(0));
        geo.saveValue(id, 0, data);
    } else {
        geo.addValue(id, data);
    }
}


void DrawingWidget::setOverlay(QImage img, int page){
    saveImageToFile(img, cache+QString::number(page));
}

QImage DrawingWidget::getOverlay(int page){
    return loadImageFromFile(cache+QString::number(page));
}

void DrawingWidget::addImage(QImage img){
    images.last_image_num++;
    images.image_count = images.last_image_num;
    images.saveValue(images.last_image_num, img.copy());
}

void DrawingWidget::initializeImage(const QSize &size) {
    QPixmap pix = QPixmap(size*mainWidget->devicePixelRatio());
    pix.setDevicePixelRatio(mainWidget->devicePixelRatio());
    pix.fill(QColor("transparent"));
    image = pix;
}

void DrawingWidget::resizeEvent(QResizeEvent *event) {
    initializeImage(event->size());
    loadImage(images.last_image_num);
    QWidget::resizeEvent(event);
}

void DrawingWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter p;
    p.begin(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.drawPixmap(0, 0, image);
    
    // Draw selection overlay if active
    if (isSelecting && penType == SELECTION) {
        p.setPen(Qt::NoPen);
        QColor semiTrans = penColor;
        semiTrans.setAlpha(127);
        p.setBrush(QBrush(semiTrans));
        p.drawRect(QRectF(last_begin, last_end).normalized());
    }
    
    p.end();
}


void DrawingWidget::clear() {
    image.fill(QColor("transparent"));
    addImage(image.toImage());
    update();
}

void DrawingWidget::clearUndoable() {
    clear();
}

void DrawingWidget::clearAll() {
    images.clear();
    pages.clear();
    goPage(0);
    update();
}



void DrawingWidget::selectionDraw(QPointF startPoint, QPointF endPoint) {
    last_begin = startPoint;
    last_end = endPoint;
    isSelecting = true;
    update();
}

#ifdef LIBARCHIVE
void DrawingWidget::saveAll(QString file){
    if (!file.isEmpty()) {
        #ifdef QPRINTER
        if(file.endsWith(".pdf")){
            pages.savePdf(file);
            return;
        }
        #endif
        if(!file.endsWith(".pen")){
            file += ".pen";
        }
        pages.saveAll(file);
    }
}

void DrawingWidget::loadArchive(const QString& filename){
    pages.loadArchive(filename);
}
#endif
void DrawingWidget::loadImage(int num){
    QImage img = images.loadValue(num);
    if(img.isNull()){
        return;
    }
    img = img.scaled(mainWidget->geometry().width(), mainWidget->geometry().height());
    QPainter p(&image);
    image.fill(QColor("transparent"));
    p.drawImage(QPointF(0,0), img);
    update();
}

void DrawingWidget::goNextPage(){
    goPage(pages.last_page_num+1);
}

void DrawingWidget::goPage(int num){
    int old = pages.last_page_num;
    images.overlayType = board->getOverlayType();
    images.pageType = board->getType();

    pages.saveValue(old, images);
    pages.last_page_num = num;

    images = pages.loadValue(pages.last_page_num);
    loadImage(images.last_image_num);
    board->backgrounds[old] = board->grab();
    board->backgrounds[old].setDevicePixelRatio(mainWidget->devicePixelRatio());

    board->setType(images.pageType);
    board->setOverlayType(images.overlayType);
    board->updateTransform();
    board->update();
    updateGui();
}

void DrawingWidget::goPreviousPage(){
    goPage(pages.last_page_num-1);
}

void DrawingWidget::goPrevious(){
    if(!isBackAvailable()){
        return;
    }
    images.last_image_num--;
    loadImage(images.last_image_num);
    updateGui();
}


void DrawingWidget::goNext(){
    if(!isNextAvailable()){
        return;
    }
    images.last_image_num++;
    loadImage(images.last_image_num);
    updateGui();
}

void DrawingWidget::setPen(int type){
    penType = type;
    extern bool magnifyActive;
    if(type != SELECTION && !magnifyActive){
        cropWidget->hide();
    }
    
    // Draw-through logic: 
    // If in Pen/Eraser mode, make the image "ghostly" (transparent to mouse)
    // If in Selection mode, make it interactive again.
    if(type == SELECTION) {
        cropWidget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
        // Büyüteç sadece aktif olarak seçildiyse önde durur
        if (magnifierWidget->isActiveSelection) setMagnifierSelected(true);
        else setMagnifierSelected(false);
        if (coordinateWidget->isActiveSelection) setCoordinateSelected(true);
        else setCoordinateSelected(false);
        if (rulerWidget->isActiveSelection) setRulerSelected(true);
        else setRulerSelected(false);
        if (protractorWidget->isActiveSelection) setProtractorSelected(true);
        else setProtractorSelected(false);
        cropWidget->raise(); // Asıl cropWidget daima en üstte olmalı!
    } else {
        cropWidget->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        // Kalem vb. kullanılırken büyüteç katmanı çizimin altında kalmalı ki çizimler gizlenmesin
        setMagnifierSelected(false);
        setCoordinateSelected(false);
        setRulerSelected(false);
        setProtractorSelected(false);
    }
}

int DrawingWidget::getPen(){
    return penType;
}
void DrawingWidget::setPenStyle(int type){
    penStyle = type;
}

int DrawingWidget::getPenStyle(){
    return penStyle;
}

void DrawingWidget::setLineStyle(int type){
    lineStyle = type;
}

int DrawingWidget::getLineStyle(){
    return lineStyle;
}

void DrawingWidget::eventHandler(int source, int type, int id, QPointF pos, float pressure){
    debug("source: %d type: %d id:%d pressure:%f\n", source, type, id, pressure);
    int ev_pen = penType;
    if(get_bool("eraser-mode") && ! is_etap){
        if(source & Qt::MiddleButton) {
            penType = MARKER;
        } else if(source & Qt::RightButton) {
            penType = ERASER;
        }
    }
    switch(type) {
        case PRESS:
            if (curs.drawing[id] && curs.drawing.contains(id)) {
                // pen type change during press
                // so release pen then repress
                if(curs.penType[id] != penType){
                    int new_pen = penType;
                    penType = curs.penType[id];
                    eventHandler(0, RELEASE, id, pos, pressure);
                    penType = new_pen;
                } else {
                    break;
                }
            }
            curs.penType[id] = penType;
            is_dragging[id] = true;
            curs.drawing[id] = true;
            if (num_of_press == 1 || id == -1){
                // Merge current selection if clicking outside, but stay in SELECTION mode
                // Merge current selection if clicking outside, but stay in SELECTION mode
                if (penType == SELECTION && hasSelection) {
                    if (!cropWidget->geometry().contains(pos.toPoint())) {
                        mergeSelection(); // "boşa tıklandığında sadece seçim iptal olsun" ve resmi tahtaya geri yapıştırsın
                        curs.drawing[id] = false; // Yeni bölge seçimi (sürükle-bırak dörtgeni) başlamasın
                        setPen(PEN);
                        updateGui();
                        return; // Finish handling this press
                    }
                }

                // If magnifier is active and we click outside it, cancel the image selection and switch to pen
                if (penType == SELECTION && !hasSelection) {
                    bool magActive = magnifierWidget && !magnifierWidget->isHidden() && magnifierWidget->isActiveSelection;
                    bool magContains = magActive && magnifierWidget->geometry().contains(pos.toPoint());
                    
                    bool coordActive = coordinateWidget && !coordinateWidget->isHidden() && coordinateWidget->isActiveSelection;
                    bool coordContains = coordActive && coordinateWidget->geometry().contains(pos.toPoint());
                    
                    bool rulerActive = rulerWidget && !rulerWidget->isHidden() && rulerWidget->isActiveSelection;
                    bool rulerContains = rulerActive && rulerWidget->geometry().contains(pos.toPoint());

                    bool protractorActive = protractorWidget && !protractorWidget->isHidden() && protractorWidget->isActiveSelection;
                    bool protractorContains = protractorActive && protractorWidget->geometry().contains(pos.toPoint());
                    
                    if ((magActive || coordActive || rulerActive || protractorActive) && !magContains && !coordContains && !rulerContains && !protractorContains) {
                        setMagnifierSelected(false);
                        setCoordinateSelected(false);
                        setRulerSelected(false);
                        setProtractorSelected(false);
                        curs.drawing[id] = false;
                        setPen(PEN);
                        updateGui();
                        return;
                    }
                }

                // Only merge and update background if we are NOT in SELECTION mode.
                if (penType != SELECTION) {
                    mergeSelection();
                    
                    if(penType != ERASER){
                        background->image = image.toImage();
                        background->update();
                        image.fill(QColor("transparent"));
                    }
                }
            }
            if(floatingSettings->isVisible()){
                floatingSettings->setHide();
            }
            geo.clear(id);
            addPoint(id, pos);
            if(penType == SELECTION) {
                break;
            }
            curs.setPosition(id, pos);
            curEventButtons = source;
            break;
        case MOVE:
            if (! curs.drawing[id]) {
                break;
            }
            switch(penType) {
                case SELECTION:
                    selectionDraw(geo.first(id), pos);
                    break;
                default:
                    if(penType == ERASER) {
                        curs.setPosition(id, pos);
                        curs.setCursor(id, penSize[penType]);
                    }
                    addPoint(id, pos);
                    painter.begin(&image);
                    drawFunc(id, pressure);
                    painter.end();
                    break;
            }
            break;
        case RELEASE:
            if (!curs.drawing[id]) {
                break;
            }
            is_dragging[id] = false;
            curs.drawing[id] = false;
            curs.hide(id);
            if(penType != ERASER && geo.size(id) < 2 && penType != SELECTION) {
                int fpenStype = penStyle;
                penStyle = LINE;
                addPoint(id, pos+QPointF(0,1));
                painter.begin(&image);
                drawLineToFunc(id, 1.0);
                painter.end();
                penStyle = fpenStype;
            }
            if(penType != ERASER && penStyle != SPLINE && penType != SELECTION && geo.size(id) >= 2) {
                painter.begin(&image);
                drawFunc(id, pressure);
                painter.end();
            }
            curEventButtons = 0;

            if(num_of_press == 0 || id == -1) {
                curs.clear();
                if(penType == SELECTION) {
                    isSelecting = false;
                    addPoint(id, pos);
                    createSelection(id);
                    update();
                }
                if(penType != ERASER){
                    background->applyImage(image.toImage());
                    image = QPixmap::fromImage(background->image);
                    background->image.fill(QColor("transparent"));
                }
                if(penType == SELECTION) {
                    break;
                }
                geo.clearAll();
                addImage(image.toImage());
            }
            if(penType != ERASER && penStyle != SPLINE){
                update();
            }
            break;
    }
    penType = ev_pen;
}

#define DEV_IDLE 0
#define DEV_MOUSE 1
#define DEV_TABLET 2

static int last_device = 0;
bool DrawingWidget::event(QEvent *ev) {
    switch (ev->type()) {
        case QEvent::TouchBegin:
        case QEvent::TouchEnd:
        case QEvent::TouchUpdate: {
            if(last_device != DEV_MOUSE && last_device != DEV_IDLE) {
                break;
            }
            QTouchEvent *touchEvent = static_cast<QTouchEvent*>(ev);
            QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->points();
            for(auto it = curs.images.begin(); it != curs.images.end(); it++){
                curs.hide(it.key());
            }
            int touch_cnt=0;
            foreach(const QTouchEvent::TouchPoint &touchPoint, touchPoints) {
                if ((Qt::TouchPointState)touchPoint.state() != Qt::TouchPointReleased) {
                    touch_cnt++;
                }
            }
            num_of_press = touch_cnt;
            if(num_of_press == 0){
                last_device = DEV_IDLE;
            } else {
                last_device = DEV_MOUSE;
            }
            foreach(const QTouchEvent::TouchPoint &touchPoint, touchPoints) {
                QPointF pos = touchPoint.position();
                if ((Qt::TouchPointState)touchPoint.state() == Qt::TouchPointPressed) {
                    eventHandler(Qt::LeftButton, PRESS, touchPoint.id(), pos, touchPoint.pressure());
                } else if ((Qt::TouchPointState)touchPoint.state() == Qt::TouchPointReleased) {
                    eventHandler(Qt::LeftButton, RELEASE, touchPoint.id(), pos, touchPoint.pressure());
                } else {
                    eventHandler(Qt::LeftButton, MOVE, touchPoint.id(), pos, touchPoint.pressure());
                }
            }
            return true;
        }
        case QEvent::TabletPress: {
            if(last_device != DEV_IDLE){
                break;
            }
            last_device = DEV_TABLET;
            num_of_press=1;
            QTabletEvent *tabletEvent = static_cast<QTabletEvent*>(ev);
            eventHandler(tabletEvent->buttons(), PRESS, -2, tabletEvent->position(), tabletEvent->pressure());
            return true;
        }
        case QEvent::TabletRelease: {
            if(last_device != DEV_TABLET){
                break;
            }
            last_device = DEV_IDLE;
            num_of_press=0;
            QTabletEvent *tabletEvent = static_cast<QTabletEvent*>(ev);
            eventHandler(tabletEvent->buttons(), RELEASE, -2, tabletEvent->position(), tabletEvent->pressure());
            return true;
        }
        case QEvent::TabletMove: {
            if(last_device != DEV_TABLET) {
                break;
            }
            QTabletEvent *tabletEvent = static_cast<QTabletEvent*>(ev);
            eventHandler(tabletEvent->buttons(), MOVE, -2, tabletEvent->position(), tabletEvent->pressure());
            return true;
        }
        case QEvent::MouseButtonPress: {
            if(last_device != DEV_IDLE){
                break;
            }
            last_device = DEV_MOUSE;
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(ev);
            eventHandler(mouseEvent->buttons(), PRESS, -1, mouseEvent->position(), 1.0);
            return true;
        }
        case QEvent::MouseButtonRelease: {
            if(last_device != DEV_MOUSE){
                break;
            }
            last_device = DEV_IDLE;
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(ev);
            eventHandler(mouseEvent->buttons(), RELEASE, -1, mouseEvent->position(), 1.0);
            return true;
        }
        case QEvent::MouseMove: {
            if(last_device != DEV_MOUSE) {
                break;
            }
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(ev);
            eventHandler(mouseEvent->buttons(), MOVE, -1, mouseEvent->position(), 1.0);
            return true;
        }

        default:
            break;
    }
    return QWidget::event(ev);
}

int DrawingWidget::getPageNum(){
    return pages.last_page_num;
}

bool DrawingWidget::isBackAvailable(){
    debug("last_page_num: %d image_count: %d\n", images.last_image_num, images.image_count );
    return images.last_image_num > images.removed +1;
}

bool DrawingWidget::isNextAvailable(){
    debug("last_page_num: %d image_count: %d\n", images.last_image_num, images.image_count );
    return images.last_image_num < images.image_count;
}


QColor convertColor(QColor color) {
    int tot =  color.red() + color.blue() + color.green();
    if (tot > 382) {
        return QColor(0,0,0, color.alpha());
    } else {
        return QColor(255, 255, 255, color.alpha());

    }
}

void removeDirectory(const QString &path) {
    QDir dir(path);
    if (!dir.exists())
        return;

    QFileInfoList fileInfoList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden);
    foreach (const QFileInfo &fi, fileInfoList) {
        if (fi.isDir()) {
            removeDirectory(fi.absoluteFilePath());
        } else {
            QFile::remove(fi.absoluteFilePath());
        }
    }

    dir.rmdir(path);
}

void qImageToFile(const QImage& image, const QString& filename) {
    QFileInfo fileInfo(filename);
    QString dirname = fileInfo.dir().absolutePath();
    QDir dir(dirname);
    if (!dir.exists(dirname)) {
        dir.mkpath(dirname);
    }
    QPixmap pixmap = QPixmap::fromImage(image);
      if (pixmap.save(filename, "PNG")) {
            debug("Image saved successfully as %s\n", filename.toStdString().c_str());
        } else {
            debug("Failed to save image at %s\n",filename.toStdString().c_str());
        }
}

QString generateRandomString(int length) {
    const QString characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    QString randomString;

    QRandomGenerator *generator = QRandomGenerator::global();
    for (int i = 0; i < length; ++i) {
        int index = generator->bounded(characters.length());
        randomString.append(characters[index]);
    }

    return randomString;
}

void clearCache(){
    debug("Clear cache:%s\n", cache_main.toStdString().c_str());
    removeDirectory(cache_main);
}