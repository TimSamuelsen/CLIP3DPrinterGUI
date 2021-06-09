#ifndef DLP9000_H
#define DLP9000_H

#include "patternelement.h"
#include "mainwindow.h"

#define PTN_WIDTH_WQXGA     2560
#define PTN_HEIGHT_WQXGA    1600

class DLP9000
{
public:
    bool InitProjector(void);
    void AddPatterns(QStringList fileNames, double ExposureTime, double DarkTime, int PrintScript, int CurrentImage, QStringList ExposureTimeList, int ProjectionMode, int BitMode, bool InitialExposure);
    int UpdatePatternMemory(int totalSplashImages, bool firmware );
    int uploadPatternToEVM(bool master, int splashImageCount, int splash_size, uint8_t* splash_block);
    void updateLUT();
    void startPatSequence(void);
    void clearElements(void);
    void setIT6535Mode(int Mode);

private:
    bool m_dualAsic = true;
    uint32_t m_ptnWidth = PTN_WIDTH_WQXGA;
    uint32_t m_ptnHeight = PTN_HEIGHT_WQXGA;
    QString m_ptnImagePath;
    QList<PatternElement> m_elements;
    int calculateSplashImageDetails(int *totalSplashImages, bool firmware);
};



#endif // DLP9000_H
