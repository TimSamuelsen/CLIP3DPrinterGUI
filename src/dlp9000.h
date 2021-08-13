#ifndef DLP9000_H
#define DLP9000_H

#include <QObject>
#include "patternelement.h"
#include "PrintElements.h"

#define PTN_WIDTH_WQXGA     2560
#define PTN_HEIGHT_WQXGA    1600

class DLP9000: public QObject
{
    Q_OBJECT

public:
    static DLP9000& Instance() {
        static DLP9000 myInstance;
        return myInstance;
    }

    DLP9000(DLP9000 const&) = delete;               //Copy construct
    DLP9000(DLP9000&&) = delete;                    //Move contstruct
    DLP9000& operator=(DLP9000 const&) = delete;    //Copy assign
    DLP9000& operator=(DLP9000 &&) = delete;        //Move assign

    bool InitProjector(void);
    void AddPatterns(QStringList fileNames, PrintSettings m_PrintSettings, PrintScripts m_PrintScripts, PrintControls m_PrintControls);
    int UpdatePatternMemory(int totalSplashImages, bool firmware);
    int uploadPatternToEVM(bool master, int splashImageCount, int splash_size, uint8_t* splash_block);
    void updateLUT(int ProjectionMode);
    void startPatSequence(void);
    void clearElements(void);
    void setIT6535Mode(int Mode);
    void SetLEDIntensity(PrintSettings dlp_PrintSettings, PrintScripts dlp_PrintScript);
    int PatternUpload(QStringList ImageList, PrintControls dlp_PrintControls, PrintSettings dlp_PrintSettings, PrintScripts dlp_PrintScript);
    void PatternDisplay(int DisplaySetting);

signals:
    void DLPPrintSignal(QString StringToPrint);
    void DLPError(QString ErrorString);

private:
    bool m_dualAsic = true;
    uint32_t m_ptnWidth = PTN_WIDTH_WQXGA;
    uint32_t m_ptnHeight = PTN_HEIGHT_WQXGA;
    QString m_ptnImagePath;
    QList<PatternElement> m_elements;
    int calculateSplashImageDetails(int *totalSplashImages, bool firmware, int ProjectionMode);

protected:
    DLP9000() {

    }

    ~DLP9000() {

    }

};
#endif // DLP9000_H
