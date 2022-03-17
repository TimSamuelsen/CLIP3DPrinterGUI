#ifndef PRINTELEMENTS_H
#define PRINTELEMENTS_H

#include <QStringList>
#include <string.h>
#include <QTime>

#define NormalTime
#define QuickTime
#define ON   1
#define OFF  0
//For selecting motion mode
#define CONTINUOUS  1
#define STEPPED     0
//For selecting projection mode
#define VIDEO 2
#define VIDEOPATTERN 1
#define POTF 0
//For selecting printer
#define CLIP30UM 0
#define ICLIP 1
#define PRE 1
#define POST 2

typedef enum StageType {
  STAGE_SMC,
  STAGE_GCODE,
} Stage_t;

typedef enum Parameter {
  EXPOSURE_TIME,
  LED_INTENSITY,
  DARK_TIME,
  LAYER_THICKNESS,
  STAGE_VELOCITY,
  STAGE_ACCELERATION,
  PUMP_HEIGHT,
  INJECTION_VOLUME,
  INJECTION_RATE,
  BASE_INJECTION,
  INITIAL_VOLUME,
  INITIAL_DELAY,
  INITIAL_INTENSITY,
  MAX_IMAGE,
  VP_RESYNC,
  CONTINUOUS_INJECTION,
  STEPPED_CONT_INJECTION,
  STARTING_POSITION,
  MAX_END,
  MIN_END,
  INJECTION_DELAY,
  DISPLAY_CABLE,
} Parameter_t;

typedef enum ExposureType {
  EXPOSURE_NORM,
  EXPOSURE_PS,
  EXPOSURE_PUMP,
  EXPOSURE_PS_PUMP,
} ExposureType_t;

struct PrintScripts {
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

  // For VP8bit workaround
  bool VP8 = false;
  int nPat = 0;
  QStringList VP8_ExpList;
  QStringList VP8_DarkList;


  //Used to determine which pattern should trigger a frame change in video pattern mode workaround
  QStringList FrameList;
};

struct PrintSettings {
  int PrinterType = CLIP30UM;
  int ProjectionMode = POTF;
  Stage_t StageType = STAGE_SMC;
  int MotionMode = STEPPED;
  bool PumpingMode = 0;

  int BitMode = 1;
  double PostExposureDelay = 0;
  double JerkTime = 0.04;
  double LayerThickness = 0;
  double StageVelocity = 0;
  double StageAcceleration = 0;
  double StartingPosition = 0;
  double MaxEndOfRun = 0;
  double MinEndOfRun = 0;
  double ExposureTime = 0;
  double DarkTime = 0;
  double InitialExposure = 0;
  int InitialDelay = 0;
  int InitialIntensity = 0;
  int UVIntensity = 0;
  int MaxImageUpload = 20;
  int ResyncVP = 24;
  QString Resin = "No resin selected";

  double PumpingParameter = 0;
};

/**
 * @brief The PrintControls struct
 * Used for variables that change during the print
 */
struct PrintControls {
  bool InitialExposureFlag = true;
  uint nSlice = 0;
  uint layerCount = 0;
  int remainingImages = 0;
  double StagePosition = 0;

  ExposureType_t ExposureType = EXPOSURE_NORM;
  QTime PrintStartTime;
  double TotalPrintTime = 0;
  double RemainingPrintTime = 0;

  //Video Pattern specific
  int BitLayer = 1;
  int FrameCount = 0;
  int ReSyncFlag = 0;
  int ReSyncCount = 0;

  //Motion Specific
  bool inMotion = 0;
  double PrintEnd = 0;

  // Connection flags
  int stageConnect1 = 0;
  int stageConnect2 = 0;
  int lightConnect = 0;
};

struct InjectionSettings {
  double InfusionRate = 0;
  double InfusionVolume = 0;
  double InitialVolume = 0;
  double BaseInjectionRate = 0;
  double InjectionDelayParam = 0;

  bool ContinuousInjection = false;
  bool SteppedContinuousInjection = false;
  int InjectionDelayFlag = 0;
};

#endif // PRINTELEMENTS_H
