#include "mainwnd.h"
#include "ui_mainwnd.h"
#include <QLayout>


MainWnd::MainWnd(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWnd) {
    ui->setupUi(this);
}

MainWnd::~MainWnd() {
    delete ui;
}

void MainWnd::on_btnReset_clicked() {
    //
}
