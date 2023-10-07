#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QImage>
#include <QScrollBar>
#include <QPrinter>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool loadFile(const QString &fileName);

private slots:
    void on_actionFileOpen_triggered();
    void on_actionFileSave_triggered();
    void on_actionFilePrint_triggered();
    void on_actionFileExit_triggered();
    void on_actionEditCopy_triggered();
    void on_actionEditPaste_triggered();
    void on_actionViewZoomIn_triggered();
    void on_actionViewZoomOut_triggered();
    void on_actionViewSizeNormal_triggered();
    void on_actionViewSizeAdjust_triggered();
    void on_actionHelpAbout_triggered();
    void clipboardChanged();

    bool saveFile(const QString &fileName);
    void setImage(const QImage &newImage);
    void scaleImage(double factor);
    void adjustImage();
    void normalImage();

private:
    Ui::MainWindow *ui;
    QLabel *m_labelImage;
    QImage m_image;
    QPrinter printer;
    double m_scaleFactor = 1;

    void adjustScrollBar(QScrollBar *scrollBar, double factor);
   void updateActions();

protected:
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent* event) override;

};
#endif // MAINWINDOW_H
