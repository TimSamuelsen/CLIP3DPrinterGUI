#ifndef IMAGEPOPOUT_H
#define IMAGEPOPOUT_H

#include <QWidget>

namespace Ui {
class imagepopout;
}

class imagepopout : public QWidget
{
    Q_OBJECT

public:
    explicit imagepopout(QWidget *parent = nullptr);
    ~imagepopout();
    void getImageList(QStringList Images);


public slots:
    void showImage(QPixmap imagename);
private:
    Ui::imagepopout *ui;
};

#endif // IMAGEPOPOUT_H
