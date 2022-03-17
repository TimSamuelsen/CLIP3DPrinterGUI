#include "printmonitoring.h"
#include "ui_printmonitoring.h"

printmonitoring::printmonitoring(QWidget* parent) :
  QWidget(parent),
  ui(new Ui::printmonitoring) {
  ui->setupUi(this);
}

printmonitoring::~printmonitoring() {
  delete ui;
}



#if 0
/*******************************************Live Value Monitoring********************************************/
/**
 * @brief MainWindow::on_LiveValueList1_activated
 * @param arg1
 * Currently Not In Use
 */
void MainWindow::on_LiveValueList1_activated(const QString& arg1) {
  PrintToTerminal("LV1: " + arg1);
}

/**
 * @brief MainWindow::on_LiveValueList2_activated
 * @param arg1
 * Currently Not In Use
 */
void MainWindow::on_LiveValueList2_activated(const QString& arg1) {
  PrintToTerminal("LV2: " + arg1);
}

/**
 * @brief MainWindow::on_LiveValueList3_activated
 * @param arg1
 * Currently Not In Use
 */
void MainWindow::on_LiveValueList3_activated(const QString& arg1) {
  PrintToTerminal("LV3: " + arg1);
}

/**
 * @brief MainWindow::on_LiveValueList4_activated
 * @param arg1
 * Currently Not In Use
 */
void MainWindow::on_LiveValueList4_activated(const QString& arg1) {
  PrintToTerminal("LV4: " + arg1);
}

/**
 * @brief MainWindow::on_LiveValueList5_activated
 * @param arg1
 * Currently Not In Use
 */
void MainWindow::on_LiveValueList5_activated(const QString& arg1) {
  PrintToTerminal("LV5: " + arg1);
}

/**
 * @brief MainWindow::on_LiveValueList6_activated
 * @param arg1
 * Currently Not In Use
 */
void MainWindow::on_LiveValueList6_activated(const QString& arg1) {
  PrintToTerminal("LV6: " + arg1);
}
#endif
