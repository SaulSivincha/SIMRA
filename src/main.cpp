#include "simra/window.hpp"
#include <QApplication>
int main(int argc,char** argv){QApplication app(argc,argv);simra::MainWindow window;window.show();return app.exec();}
