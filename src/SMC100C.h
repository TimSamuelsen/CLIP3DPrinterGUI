#ifndef SMC100C_h
#define SMC100C_h

#include <stdint.h>
#include "serialib.h"

class SMC100C
{
    public:
            typedef void( *FinishedListener )();
            //ASCII commands from SMC100C User Manual p. 22-70
            enum class CommandType
            {
                None,
                Acceleration,
                BacklashComp,
                HysterisisComp,
                DriverVoltage,
                KdLowPassFilterCutOff,
                FollowingErrorLim,
                FrictionComp,
                HomeSearchType,
                StageIdentifier,
                LeaveJoggingState,
                KeypadEnable,
                JerkTime,
                DerivativeGain,
                IntegralGain,
                ProportionalGain,
                VelocityFeedForward,
                Enable,
                HomeSearchVelocity,
                HomeSearch,
                HomeSearchTimeout,
                MoveAbs,
                MoveRel,
                MoveEstimate,
                Configure,
                Analog,
                TTLInputVal,
                Reset,
                RS485Adress,
                TTLOutputVal,
                ControlLoopState,
                NegativeSoftwareLim,
                PositiveSoftwareLim,
                StopMotion,
                EncoderIncrementVal,
                CommandErrorString,
                LastCommandErr,
                PositionAsSet,
                PositionReal,
                ErrorStatus,
                Velocity,
                BaseVelocity,
                ControllerRevisionInfo,
                AllConfigParam,
                ESPStageConfig,
            };
            enum class CommandParameterType
            {
                None,
                Int,
                Float,
            };
            enum class CommandGetSetType
            {
                None,
                Get,
                Set,
                GetSet,
                GetAlways,
            };
            enum class ModeType
            {
                Inactive,
                Idle,
                WaitingForCommandReply,
            };
            struct CommandStruct
            {
                CommandType Command;
                const char* CommandChar;
                CommandParameterType SendType;
                CommandGetSetType GetSetType;
            };
            struct CommandEntry
            {
                const CommandStruct* Command;
                CommandGetSetType GetOrSet;
                float Parameter;

            };
            enum class StatusType
            {
                Unknown,
                Error,
                Config,
                NoReference,
                Homing,
                Moving,
                Ready,
                Disabled,
                Jogging,
            };
            struct StatusCharSet
            {
                const char* Code;
                StatusType Type;
            };
            bool SMC100CInit(const char*);
            void SMC100CClose();
            bool Home(void);
            bool QueryHardware();
            void SetVelocity(float VelocityToSet);
            void RelativeMove(float CommandParameter);
            void AbsoluteMove(float AbsoluteDistanceToMove);
            char* GetPosition();
            char* GetVelocity();
            char* GetAcceleration();
            char* GetPositiveLimit();
            char* GetNegativeLimit();
            void GetError();
            char* GetMotionTime();
            void StopMotion();
            void SetPositiveLimit(float Limit);
            void SetNegativeLimit(float Limit);
            void SetAcceleration(float AccelerationToSet);
            serialib serial;

    private:
            static const char GetCharacter;
            //void Home(void);
            static const CommandStruct CommandLibrary[];
            static const StatusCharSet StatusLibrary[];
            const char* ConvertToErrorString(char ErrorCode);
            bool SendCurrentCommand();
            CommandEntry CommandToPrint;
            const CommandStruct* CurrentCommand;
		    CommandGetSetType CurrentCommandGetOrSet;
		    float CurrentCommandParameter;
            void SetCommand(CommandType Type, float Parameter, CommandGetSetType GetOrSet);
            StatusType ConvertStatus(char* StatusChar);
            char* SerialRead();
};

#endif
