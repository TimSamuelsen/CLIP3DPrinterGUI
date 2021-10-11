#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <QWidget>
#include "PrintElements.h"

namespace Ui {
class graphics;
}

class graphics : public QWidget
{
    Q_OBJECT

public:
    explicit graphics(QWidget *parent = nullptr);
    ~graphics();
    void initPlot(PrintControls m_PrintControls, PrintSettings m_PrintSettings,
                  PrintScripts m_PrintScript);
    void updatePlot(PrintControls m_PrintControls, PrintSettings m_PrintSettings,
                    PrintScripts m_PrintScript);
    void initPrintScriptTable(PrintSettings m_PrintSettings, PrintScripts *pPrintScript);
    void PrintScriptTableEntry(QStringList Script, uint ColNum);
    void updatePrintScriptTable(PrintSettings m_PrintSettings, PrintScripts m_PrintScript);

Q_SIGNALS:
    void GraphicsPrint(QString);

private slots:
    void on_PrintScriptTable_cellChanged(int row, int column);

    void on_PrintScriptTable_cellPressed(int row, int column);

private:
    Ui::graphics *ui;
    QVector<double> qv_x, qv_y;
    double RemainingPrintTime;
    bool PSTableInitFlag = false;
    PrintScripts *gpPrintScript;
};

#endif // GRAPHICS_H
