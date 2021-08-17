#ifndef PUMPCOMMANDS_H
#define PUMPCOMMANDS_H

#include <QObject>
#include "serialib.h"

/*!
 * \brief The PumpCommands class handles injection pump operation
 */
class PumpCommands: public QObject
{
    Q_OBJECT

public:
    static PumpCommands& Instance() {
        static PumpCommands myInstance;
        return myInstance;
    }
    PumpCommands(PumpCommands const&) = delete;               //Copy construct
    PumpCommands(PumpCommands&&) = delete;                    //Move contstruct
    PumpCommands& operator=(PumpCommands const&) = delete;    //Copy assign
    PumpCommands& operator=(PumpCommands &&) = delete;        //Move assign

    serialib PumpSerial;

    int StartInfusion();
    int StartWithdraw();
    int Stop();

    int SetTargetTime(double T_Time);
    int SetTargetVolume(double T_Vol);
    int SetSyringeVolume(double S_Vol);
    int SetInfuseRate(double I_Rate);
    int SetWithdrawRate(double W_Rate);

    int ClearTime();
    int ClearVolume();
    int CustomCommand(QString NewCommand);
    int IndefiniteRun();

    QString GetTargetTime();
    QString GetTargetVolume();
    QString GetSyringeVolume();
    QString GetInfuseRate();
    QString GetWithdrawRate();

signals:
    /*!
     * \brief PumpPrintSignal Injection pump terminal printing  signal,
     * connected to TerminalPrint function in Main Window.
     * \param StringToPrint String to be printed
     */
    void PumpPrintSignal(QString StringToPrint);
    /*!
     * \brief PumpError Light engine error signal,
     * connected to showError function in Main Window.
     * \param ErrorString Error string to be shown
     */
    void PumpError(QString ErrorString);

private:
    char* SerialRead();
    void CommandBufferUpdate();

protected:
    PumpCommands(){

    }
    ~PumpCommands(){

    }
};

#endif // PUMPCOMMANDS_H
