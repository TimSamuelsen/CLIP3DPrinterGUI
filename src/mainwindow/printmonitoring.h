#ifndef PRINTMONITORING_H
#define PRINTMONITORING_H

#include <QWidget>

namespace Ui {
class printmonitoring;
}

class printmonitoring : public QWidget {
  Q_OBJECT

 public:
  explicit printmonitoring(QWidget* parent = nullptr);
  ~printmonitoring();

 private:
  Ui::printmonitoring* ui;
};

#endif // PRINTMONITORING_H
