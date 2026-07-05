#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowTitle("Cliente Supervisor (Consumidor de Dados)");
    w.show();
    return a.exec();
}
