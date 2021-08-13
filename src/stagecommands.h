#ifndef STAGECOMMANDS_H
#define STAGECOMMANDS_H

#include <QObject>
#include "PrintElements.h"
#include "SMC100C.h"

class StageCommands: public QObject
{
    Q_OBJECT
public:
    static StageCommands& Instance() {
        static StageCommands myInstance;
        return myInstance;
    }
    StageCommands(StageCommands const&) = delete;               //Copy construct
    StageCommands(StageCommands&&) = delete;                    //Move contstruct
    StageCommands& operator=(StageCommands const&) = delete;    //Copy assign
    StageCommands& operator=(StageCommands &&) = delete;        //Move assign

    SMC100C SMC;
    serialib StageSerial;

    int StageInit(const char* COMPort,Stage_t StageType);
    int StageClose(Stage_t StageType);
    int StageHome(Stage_t StageType);
    int StageStop(Stage_t StageType);
    int SetStageVelocity(float VelocityToSet, Stage_t StageType);
    int SetStageAcceleration(float AccelerationToSet, Stage_t StageType);
    int SetStagePositiveLimit(float PositiveLimit, Stage_t StageType);
    int SetStageNegativeLimit(float NegativeLimit, Stage_t StageType);
    int StageAbsoluteMove(float AbsoluteMovePosition, Stage_t StageType);
    int StageRelativeMove(float RelativeMoveDistance, Stage_t StageType);
    QString StageGetPosition(Stage_t);
    void initStagePosition(PrintSettings si_PrintSettings);
    void initStageStart(PrintSettings si_PrintSettings);

signals:
    void StagePrintSignal(QString StringToPrint);
    void StageError(QString ErrorString);

private:
    void GetValues();
    void verifyStageParams(PrintSettings s_PrintSettings);

private slots:
    void initStageSlot();

    void fineMovement();
protected:
    StageCommands() {

    }

    ~StageCommands() {

    }
};

#endif // STAGECOMMANDS_H
