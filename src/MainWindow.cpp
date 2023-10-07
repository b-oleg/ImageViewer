#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QDir>
#include <QFileDialog>
#include <QStandardPaths>
#include <QPainter>
#include <QClipboard>
#include <QMimeData>
#include <QColorSpace>
#include <QMessageBox>
#include <QImageReader>
#include <QImageWriter>
#include <QWheelEvent>
#include <QPrintDialog>

#define ZOOM_FACTOR_IN      1.25
#define ZOOM_FACTOR_OUT     0.8
#define ZOOM_FACTOR_MIN     0.1
#define ZOOM_FACTOR_MAX     10.0

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // label for image
    m_labelImage = new QLabel(this);
    m_labelImage->setScaledContents(false);
    m_labelImage->setBackgroundRole(QPalette::Dark);
    m_labelImage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_labelImage->setAlignment(Qt::AlignCenter);
    m_labelImage->setMargin(0);

    // scroll for label
    ui->scrollArea->setBackgroundRole(QPalette::Dark);
    ui->scrollArea->setWidget(m_labelImage);
    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->scrollArea->setWidgetResizable(true);

    // image in clipboard?
    clipboardChanged();
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &MainWindow::clipboardChanged);

    // about qt
    ui->menuHelp->addAction(tr("About &Qt"), this, &QApplication::aboutQt);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::loadFile(const QString &fileName)
{
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    const QImage newImage = reader.read();
    if (newImage.isNull()) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(), tr("Ошибка при открытии %1: %2").arg(QDir::toNativeSeparators(fileName), reader.errorString()));
        return false;
    }

    setImage(newImage);
    setWindowFilePath(fileName);
    const QString message = tr("Изображение \"%1\", Размер:%2x%3, Глубина цвета:%4бит").arg(QDir::toNativeSeparators(fileName)).arg(m_image.width()).arg(m_image.height()).arg(m_image.depth());
    statusBar()->showMessage(message);
    return true;
}

bool MainWindow::saveFile(const QString &fileName)
{
    QImageWriter writer(fileName);
    if (!writer.write(m_image)) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(), tr("Ошибка при сохранении %1: %2").arg(QDir::toNativeSeparators(fileName)), writer.errorString());
        return false;
    }
    const QString message = tr("Сохранен: \"%1\"").arg(QDir::toNativeSeparators(fileName));
    statusBar()->showMessage(message);
    return true;
}

void MainWindow::updateActions()
{
    ui->actionFileSave->setEnabled(!m_image.isNull());
    ui->actionFilePrint->setEnabled(!m_image.isNull());
    ui->actionEditCopy->setEnabled(!m_image.isNull());
    ui->actionViewZoomIn->setEnabled(!m_image.isNull());
    ui->actionViewZoomOut->setEnabled(!m_image.isNull());
    ui->actionViewSizeNormal->setEnabled(!m_image.isNull());
    ui->actionViewSizeAdjust->setEnabled(!m_image.isNull());
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        scaleImage((event->angleDelta().y() > 0) ? ZOOM_FACTOR_IN : ZOOM_FACTOR_OUT);
        event->accept();
    } else {
        QMainWindow::wheelEvent(event);
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    if (ui->actionViewSizeAdjust->isChecked()) adjustImage();
}

static void initImageFileDialog(QFileDialog &dialog, QFileDialog::AcceptMode acceptMode)
{
    static bool firstDialog = true;

    if (firstDialog) {
        firstDialog = false;
        const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
        dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last());
    }

    QStringList mimeTypeFilters;
    const QByteArrayList supportedMimeTypes = acceptMode == QFileDialog::AcceptOpen ? QImageReader::supportedMimeTypes() : QImageWriter::supportedMimeTypes();
    for (const QByteArray &mimeTypeName : supportedMimeTypes) mimeTypeFilters.append(mimeTypeName);
    mimeTypeFilters.sort();
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/jpeg");
    if (acceptMode == QFileDialog::AcceptSave) dialog.setDefaultSuffix("jpg");
}

void MainWindow::on_actionFileOpen_triggered()
{
    QFileDialog dialog(this, tr("Открыть файл"));
    initImageFileDialog(dialog, QFileDialog::AcceptOpen);
    while (dialog.exec() == QDialog::Accepted && !loadFile(dialog.selectedFiles().first())) {}
}

void MainWindow::on_actionFileSave_triggered()
{
    QFileDialog dialog(this, tr("Сохранить"));
    initImageFileDialog(dialog, QFileDialog::AcceptSave);
    while (dialog.exec() == QDialog::Accepted && !saveFile(dialog.selectedFiles().first())) {}
}

void MainWindow::on_actionFilePrint_triggered()
{
    Q_ASSERT(!m_labelImage->pixmap(Qt::ReturnByValue).isNull());
    QPrintDialog dialog(&printer, this);
    if (dialog.exec()) {
        QPainter painter(&printer);
        QPixmap pixmap = m_labelImage->pixmap(Qt::ReturnByValue);
        QRect rect = painter.viewport();
        QSize size = pixmap.size();
        size.scale(rect.size(), Qt::KeepAspectRatio);
        painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
        painter.setWindow(pixmap.rect());
        painter.drawPixmap(0, 0, pixmap);
    }
}

void MainWindow::on_actionFileExit_triggered()
{
    close();
}

void MainWindow::on_actionEditCopy_triggered()
{
    QGuiApplication::clipboard()->setImage(m_image);
    statusBar()->showMessage(tr("Изображение скопировано в буфера обмена."));
}

static QImage clipboardImage()
{
    if (const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData()) {
        if (mimeData->hasImage()) {
            const QImage image = qvariant_cast<QImage>(mimeData->imageData());
            if (!image.isNull()) return image;
        }
    }
    return QImage();
}

void MainWindow::on_actionEditPaste_triggered()
{
    const QImage newImage = clipboardImage();
    if (newImage.isNull()) {
        statusBar()->showMessage(tr("Нет изображения в буфере обмена"));
    } else {
        setImage(newImage);
        setWindowFilePath(QString());
        const QString message = tr("Изображение из буфера обмена. Размер: %1x%2, Глубина цвета: %3бит").arg(newImage.width()).arg(newImage.height()).arg(newImage.depth());
        statusBar()->showMessage(message);
    }
}

void MainWindow::setImage(const QImage &newImage)
{
    m_image = newImage;
    if (m_image.colorSpace().isValid()) m_image.convertToColorSpace(QColorSpace::SRgb);
    if (ui->actionViewSizeAdjust->isChecked()) {
        adjustImage();
    } else {
        normalImage();
    }
    updateActions();
}

void MainWindow::scaleImage(double factor)
{
    ui->actionViewSizeAdjust->setChecked(false);
    m_scaleFactor *= factor;
    if (m_scaleFactor == 1) {
        normalImage();
        return;
    }
    ui->scrollArea->setWidgetResizable(false);
    QSize newSize = factor * m_labelImage->pixmap(Qt::ReturnByValue).size();
    m_labelImage->resize(newSize);
    m_labelImage->setPixmap(QPixmap::fromImage(m_image).scaled(newSize, Qt::KeepAspectRatio));

    adjustScrollBar(ui->scrollArea->horizontalScrollBar(), factor);
    adjustScrollBar(ui->scrollArea->verticalScrollBar(), factor);

    ui->actionViewZoomIn->setEnabled(m_scaleFactor < ZOOM_FACTOR_MAX);
    ui->actionViewZoomOut->setEnabled(m_scaleFactor > ZOOM_FACTOR_MIN);
}

void MainWindow::adjustImage()
{
    if (m_image.isNull()) return;
    ui->scrollArea->setWidgetResizable(true);
    QSize newSize = ui->scrollArea->size();
    m_scaleFactor = double(m_labelImage->width()) / double(m_image.width());
    m_labelImage->setPixmap(QPixmap::fromImage(m_image).scaled(newSize, Qt::KeepAspectRatio));
    m_labelImage->adjustSize();
}

void MainWindow::normalImage()
{
    if (m_image.isNull()) return;
    ui->scrollArea->setWidgetResizable(false);
    m_scaleFactor = 1;
    QSize newSize = m_image.size();
    m_labelImage->resize(newSize);
    m_labelImage->setPixmap(QPixmap::fromImage(m_image).scaled(newSize, Qt::KeepAspectRatio));
    m_labelImage->adjustSize();
    ui->actionViewSizeNormal->setChecked(true);
}

void MainWindow::adjustScrollBar(QScrollBar *scrollBar, double factor)
{
    scrollBar->setValue(int(factor * scrollBar->value() + ((factor - 1) * scrollBar->pageStep()/2)));
}


void MainWindow::on_actionViewZoomIn_triggered()
{
    ui->actionViewSizeAdjust->setChecked(false);
    ui->actionViewSizeNormal->setChecked(false);
    scaleImage(ZOOM_FACTOR_IN);
}


void MainWindow::on_actionViewZoomOut_triggered()
{
    ui->actionViewSizeAdjust->setChecked(false);
    ui->actionViewSizeNormal->setChecked(false);
    scaleImage(ZOOM_FACTOR_OUT);
}

void MainWindow::on_actionViewSizeNormal_triggered()
{
    ui->actionViewSizeAdjust->setChecked(false);
    normalImage();
}

void MainWindow::on_actionViewSizeAdjust_triggered()
{
   if (ui->actionViewSizeAdjust->isChecked()) {
        ui->actionViewSizeNormal->setChecked(false);
        adjustImage();
   }
}

void MainWindow::on_actionHelpAbout_triggered()
{
    QMessageBox::about(this, tr("О программе..."),
                       tr("<p>The <b>Image Viewer</b> example shows how to combine QLabel "
                          "and QScrollArea to display an image. QLabel is typically used "
                          "for displaying a text, but it can also display an image. "
                          "QScrollArea provides a scrolling view around another widget. "
                          "If the child widget exceeds the size of the frame, QScrollArea "
                          "automatically provides scroll bars. </p><p>The example "
                          "demonstrates how QLabel's ability to scale its contents "
                          "(QLabel::scaledContents), and QScrollArea's ability to "
                          "automatically resize its contents "
                          "(QScrollArea::widgetResizable), can be used to implement "
                          "zooming and scaling features. </p><p>In addition the example "
                         "shows how to use QPainter to print an image.</p>"));
}

void MainWindow::clipboardChanged()
{
    ui->actionEditPaste->setEnabled(QApplication::clipboard()->mimeData()->hasImage());
}

