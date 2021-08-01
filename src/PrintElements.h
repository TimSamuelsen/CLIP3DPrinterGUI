#ifndef PRINTELEMENTS_H
#define PRINTELEMENTS_H

#include <QStringList>
#include <string.h>

#include "mainwindow.h"

#define NormalTime
#define QuickTime
#define ON   1
#define OFF  0
//For selecting motion mode
#define CONTINUOUS  1
#define STEPPED     0
//For selecting projection mode
#define VIDEOPATTERN 1
#define POTF 0
//For selecting printer
#define CLIP30UM 0
#define ICLIP 1
#define PRE 1
#define POST 2

struct PrintScripts
{
    int PrintScript = OFF; //Default print script is set to off

    //Printscript is taken from a .csv file and stored in this variable
    QStringList ExposureScriptList;
    QStringList LEDScriptList;
    QStringList DarkTimeScriptList;
    QStringList InjectionVolumeScriptList;
    QStringList InjectionRateScriptList;
    QStringList StageVelocityScriptList;
    QStringList StageAccelerationScriptList;
    QStringList PumpHeightScriptList;
    QStringList LayerThicknessScriptList;

    //Used to determine which pattern should trigger a frame change in video pattern mode workaround
    QStringList FrameList;
};

struct PrintSettings
{
    int PrinterType = CLIP30UM;
    int ProjectionMode = POTF;
    Stage_t StageType;
    int BitMode = 1;

    double LayerThickness;
    double StageVelocity;
    double StageAcceleration;
    double StartingPosition;
    double MaxEndOfRun;
    double MinEndOfRun;
    double ExposureTime;
    double DarkTime;
    uint InitialExposure;
    int UVIntensity;
    int MaxImageUpload = 20;
};

/**
 * @brief The PrintControls struct
 * Used for variables that change during the print
 */
struct PrintControls
{
    bool InitialExposureFlag = true;
    uint nSlice = 0;
    uint layerCount = 0;
    int remainingImages;


    double TotalPrintTime = 0;
    double RemainingPrintTime;

    //Video Pattern specific
    int BitLayer = 1;
    int FrameCount = 0;
    int ReSyncFlag = 0;
    int ReSyncCount = 0;
};

struct InjectionSettings
{
    double InfusionRate;
    double InfusionVolume;
    double InitialVolume;

    bool ContinuousInjection = false;
    int InjectionDelayFlag;
    double InjectionDelayParam;
};

struct MotionSettings
{
    int MotionMode = STEPPED;
    bool inMotion;
    double PrintEnd;

    bool PumpingMode = 0;
    double PumpingParameter;
};

#endif // PRINTELEMENTS_H
