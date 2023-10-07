#include <QApplication>
#include <QCommandLineParser>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QGuiApplication::setApplicationDisplayName(QMainWindow::tr("Image Viewer"));

    QCommandLineParser commandLineParser;
    commandLineParser.addHelpOption();
    commandLineParser.addPositionalArgument(QMainWindow::tr("[file]"), QMainWindow::tr("Image file to open."));
    commandLineParser.process(QCoreApplication::arguments());

    MainWindow w;

    if (!commandLineParser.positionalArguments().isEmpty() && !w.loadFile(commandLineParser.positionalArguments().front())) {
        return -1;
    }

    w.show();
    return a.exec();
}
