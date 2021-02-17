#ifndef DLP9000_H
#define DLP9000_H

#include "patternelement.h"
#include "mainwindow.h"


class DLP9000
{
public:
    bool InitProjector(void);
    void AddPatterns(QStringList fileNames, double ExposureTime, double DarkTime, int UVIntensity);
    int UpdatePatternMemory(int totalSplashImages, bool firmware );
    int uploadPatternToEVM(bool master, int splashImageCount, int splash_size, uint8_t* splash_block);
    void updateLUT();
    void startPatSequence();

private:
    bool m_dualAsic;
    uint32_t m_ptnWidth, m_ptnHeight;
    QString m_ptnImagePath;
    QList<PatternElement> m_elements;
};



#endif // DLP9000_H
