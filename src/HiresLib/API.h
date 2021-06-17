/*
 * API.h
 *
 * This module provides C callable APIs for each of the command supported by LightCrafter4500 platform and detailed in the programmer's guide.
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#ifndef API_H
#define API_H

#include "Common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Total size of all the parameters/payload in bytes */
#define API_MAX_PARAM_SIZE      500

typedef struct
{
    unsigned short firstPixel;
    unsigned short firstLine;
    unsigned short pixelsPerLine;
    unsigned short linesPerFrame;
}rectangle;

typedef struct
{
   unsigned char blstatus;
   unsigned int blVerMajor;
   unsigned int blVerMinor;
   unsigned int blVerPatch;
   unsigned char blASICID;
   unsigned char rsvd[7];
}BootLoaderStaus;

typedef struct
{
	uint08 I2CCmd;			/**< I2C Command number */
	uint08 Read;			/**< Read Command / Write Command */
	uint16 USBCmd;          /**< USB Command number */
	char const *CmdName;	/**< Command Name */
	uint08 *Payload;		/**< Payload buffer */
	uint16 PayloadLen;		/**< Number of bytes in payload */
} API_CommandInfo_t;

typedef enum
{
    PTN_MODE_DISABLE, /**< Disable pattern mode */
    PTN_MODE_SPLASH,  /**< Patterns are loaded from flash */
    PTN_MODE_VIDEO,   /**< Patterns are streamed through video port */
    PTN_MODE_OTF,     /**< Patterns are loaded on the fly through USB/I2C */
    PTN_NUM_MODES
} API_DisplayMode_t;

typedef enum {
    VIDEO_CON_DISABLE,
    VIDEO_CON_HDMI,
    VIDEO_CON_DP,
} API_VideoConnector_t;

typedef void API_DataCallback_t(void *Param, API_CommandInfo_t *CmdInfo);

int LCR_SoftwareReset(void);

int LCR_SetOutputBufferIndex(int index);

/* System status commands */
int LCR_GetStatus(unsigned char *pHWStatus, unsigned char *pSysStatus,
                                                    unsigned char *pMainStatus);
int LCR_GetVersion(unsigned int *pApp_ver, unsigned int *pAPI_ver,
                    unsigned int *pSWConfig_ver, unsigned int *pSeqConfig_ver);
int LCR_GetFrmwVersion(unsigned int *pFrmwType, char *pFrmwTag);

/* Flash Programing commands */
int LCR_GetBLStatus(BootLoaderStaus *pBL_Status);
int LCR_SetFlashType(unsigned char Type);
int LCR_EnterProgrammingMode(void);
int LCR_ExitProgrammingMode(void);
int LCR_GetProgrammingMode(BOOL *ProgMode);
int LCR_GetFlashPresent(BOOL *pFlashAtCS0, BOOL *pFlashAtCS1, BOOL *pFlashAtCS2);
int LCR_GetFlashManID(unsigned short *manID);
int LCR_GetFlashDevID(unsigned long long *devID);
int LCR_SetFlashAddr(unsigned int Addr);
int LCR_SetFlashAddr4Byte(unsigned int Addr);
int LCR_FlashSectorErase(void);
int LCR_SetDownloadSize(unsigned int dataLen);
int LCR_SetDownloadSize4Byte(unsigned int dataLen);
int LCR_DownloadData(unsigned char *pByteArray, unsigned int dataLen);
void LCR_WaitForFlashReady(void);
int LCR_CalculateFlashChecksum(void);
int LCR_GetFlashChecksum(unsigned int*checksum);
int LCR_EnableMasterSlave(BOOL MasterEnable, BOOL SlaveEnable);

int LCR_SetPowerMode(unsigned char);
int LCR_GetPowerMode(BOOL *Standby);

int LCR_SetCurtainColor(unsigned int red,unsigned int green, unsigned int blue);
int LCR_GetCurtainColor(unsigned int *pRed, unsigned int *pGreen, unsigned int *pBlue);

int LCR_SetDataChannelSwap(unsigned int port, unsigned int swap);
int LCR_GetDataChannelSwap(unsigned int Port, unsigned int *pSwap);

int LCR_SetPortClock(unsigned int clock);
int LCR_GetPortClock(unsigned int *pClock);

int LCR_SetInputSource(unsigned int source, unsigned int portWidth);
int LCR_GetInputSource(unsigned int *pSource, unsigned int *portWidth);

int LCR_SetPixelFormat(unsigned int format);
int LCR_GetPixelFormat(unsigned int *pFormat);

int LCR_SetTPGSelect(unsigned int pattern);
int LCR_GetTPGSelect(unsigned int *pPattern);

int LCR_SetTPGColor(unsigned short redFG, unsigned short greenFG,
                    unsigned short blueFG, unsigned short redBG,
                    unsigned short greenBG, unsigned short blueBG);
int LCR_GetTPGColor(unsigned short *pRedFG, unsigned short *pGreenFG,
                    unsigned short *pBlueFG, unsigned short *pRedBG,
                    unsigned short *pGreenBG, unsigned short *pBlueBG);

int LCR_LoadSplash(unsigned int index);
int LCR_GetSplashIndex(unsigned int *pIndex);

int LCR_SetLongAxisImageFlip(BOOL);
BOOL LCR_GetLongAxisImageFlip();
int LCR_SetShortAxisImageFlip(BOOL);
BOOL LCR_GetShortAxisImageFlip();

int LCR_SetLedEnables(BOOL SeqCtrl, BOOL Red, BOOL Green, BOOL Blue);
int LCR_GetLedEnables(BOOL *pSeqCtrl, BOOL *pRed, BOOL *pGreen, BOOL *pBlue);

int LCR_SetLEDPWMInvert(BOOL invert);
int LCR_GetLEDPWMInvert(BOOL *inverted);

int LCR_GetLedCurrents(unsigned char *pRed, unsigned char *pGreen,
                                            unsigned char *pBlue);
int LCR_SetLedCurrents(unsigned char RedCurrent, unsigned char GreenCurrent,
                                            unsigned char BlueCurrent);

int LCR_SetLedFrequency(unsigned int Frequency);
int LCR_GetLedFrequency(unsigned int *pFrequency);

int LCR_SetGPIOConfig(unsigned int pinNum, BOOL dirOutput,
                                BOOL outTypeOpenDrain, BOOL pinState);
int LCR_GetGPIOConfig(unsigned int pinNum, BOOL *pDirOutput,
                                BOOL *pOutTypeOpenDrain, BOOL *pState);

int LCR_SetGeneralPurposeClockOutFreq(unsigned int clkId, BOOL enable, unsigned int clkDivider);
int LCR_GetGeneralPurposeClockOutFreq(unsigned int clkId, BOOL *pEnabled, unsigned int *pClkDivider);


int LCR_SetPWMEnable(unsigned int channel, BOOL Enable);
int LCR_GetPWMEnable(unsigned int channel, BOOL *pEnable);
int LCR_SetPWMConfig(unsigned int channel, unsigned int pulsePeriod,
                                            unsigned int dutyCycle);
int LCR_GetPWMConfig(unsigned int channel, unsigned int *pPulsePeriod,
                                            unsigned int *pDutyCycle);

int LCR_getBatchFileName(unsigned char id, char *batchFileName, unsigned char* length);
int LCR_executeBatchFile(unsigned char id);

int LCR_SetMode(API_DisplayMode_t SLmode);
int LCR_GetMode(API_DisplayMode_t *pMode);

int LCR_SoftwareDMDPark(int DmdPark);
int LCR_GetSoftwareDMDPark(int *pDmdPark);

int LCR_SetDisplay(rectangle croppedArea, rectangle displayArea);
int LCR_GetDisplay(rectangle *pCroppedArea, rectangle *pDisplayArea);

int LCR_SetTrigOutConfig(unsigned int trigOutNum, BOOL invert, short rising, short falling);
int LCR_GetTrigOutConfig(unsigned int trigOutNum, BOOL *pInvert,short *pRising, short *pFalling);

int LCR_SetTrigIn1Config(BOOL invert, unsigned int trigDelay);
int LCR_GetTrigIn1Config(BOOL *pInvert, unsigned int *pTrigDelay);
int LCR_SetTrigIn1Delay(unsigned int Delay);
int LCR_GetTrigIn1Delay(unsigned int *pDelay);

int LCR_SetTrigIn2Config(BOOL invert);
int LCR_GetTrigIn2Config(BOOL *pInvert);

int LCR_SetRedLEDStrobeDelay(BOOL invert, short rising, short falling);
int LCR_SetGreenLEDStrobeDelay(BOOL invert, short rising, short falling);
int LCR_SetBlueLEDStrobeDelay(BOOL invert, short rising, short falling);
int LCR_GetRedLEDStrobeDelay(BOOL *pInvert, short *, short *);
int LCR_GetGreenLEDStrobeDelay(BOOL *pInvert, short *, short *);
int LCR_GetBlueLEDStrobeDelay(BOOL *pInvert, short *, short *);

int LCR_PatternDisplay(int Action);

int LCR_SetInvertData(BOOL invert);
int LCR_GetInvertData(BOOL *pInvert);

int LCR_SetPatternConfig(unsigned int numLutEntries, unsigned int repeat);
int LCR_GetPatternConfig(unsigned int *pNumLutEntries, BOOL *pRepeat, unsigned int *pNumPatsForTrigOut2, unsigned int *pNumSplash);

int LCR_ClearPatLut(void);
int LCR_AddToPatLut(int patNum, int ExpUs, BOOL ClearPat, int BitDepth, int LEDSelect, BOOL WaitForTrigger, int DarkTime, BOOL TrigOut2, int SplashIndex, int BitIndex);
int LCR_SendPatLut(void);
int LCR_SendPatReorderUpdate(uint8* payload, int size);

int LCR_InitPatternMemLoad(BOOL master, unsigned short imageNum, unsigned int size);
int LCR_pattenMemLoad(BOOL master, unsigned char *pByteArray, int size);

int LCR_WriteI2CPassThrough(unsigned int port, unsigned int devadd, unsigned char* wdata, unsigned int nwbytes);
int LCR_ReadI2CPassThrough(unsigned int port, unsigned int devadd, unsigned char* wdata, unsigned int nwbytes, unsigned int nrbytes, unsigned char* rdata);
int LCR_I2CConfigure(unsigned int port, unsigned int addm, unsigned int clk);

int LCR_SetPortConfig(unsigned int dataPort,unsigned int pixelClock,unsigned int dataEnable,unsigned int syncSelect);
int LCR_GetPortConfig(unsigned int *pDataPort,unsigned int *pPixelClock,unsigned int *pDataEnable,unsigned int *pSyncSelect);

int LCR_enableDebug();
int LCR_ExecuteRawCommand(uint16 USBCmd, uint08 *Data, int Length);

int LCR_SetIT6535PowerMode(API_VideoConnector_t powerMode);
int LCR_GetIT6535PowerMode(API_VideoConnector_t *pPowerMode);

int LCR_SetDMDBlocks(int startBlock, int numBlocks);
int LCR_GetDMDBlocks(int *startBlock, int *numBlocks);
int LCR_SetDMDSaverMode(short mode);
int LCR_GetDMDSaverMode();

int LCR_ReadErrorCode(unsigned int *pCode);
int LCR_ReadErrorString(char *errStr);

int LCR_SetParallelPortConfig(uint16 TAPPL, uint16 TAPLF, uint16 AAFP, uint16 AAFL, uint16 AAPPL, uint16 AAPLF, uint16 BFFL, uint16 PixelClockFreq);
int LCR_GetParallelPortConfig(uint16 *TAPPL, uint16 *TAPLF, uint16 *AAFP, uint16 *AAFL, uint16 *AAPPL, uint16 *AAPLF, uint16 *BFFL, uint32 *PixelClockFreq);

int LCR_SetMinLEDPulseWidth(uint32 pulseWidthUS);
int LCR_GetMinLEDPulseWidth(uint32 *pulseWidthUS);

int LCR_GetMinPatExposure(int minExposureUS[16]);

int LCR_ReadSram(uint32 address, uint32 *data, uint08 type);
int LCR_WriteSram(uint32 address, uint32 data, uint08 type);
int LCR_LoadNGo_Init(uint32 size);
int LCR_LoadNGo_Data(unsigned char *pByteArray, unsigned int dataLen);
int LCR_LoadNGo_SRAM(BOOL run);

void API_SetDataCallback(API_DataCallback_t *Callback, void *Param);
int API_GetI2CCommand(char *command, unsigned char *i2cCommand);
int API_GetUSBCommand(const char *command, uint16 *usbCommand);
int API_GetCommandLength(unsigned char *cmd, int *len);
int API_GetCommandName(unsigned char i2cCommand, const char **command);
int LCR_GetBusyGPIOConfig(unsigned char *value);
int LCR_SetBusyGPIOConfig(unsigned char value);
int LCR_GetStandbyDelaySec(unsigned char *value);
int LCR_SetFlashTest(unsigned int offset, unsigned int size);
int LCR_SetDeGammaIndex(BOOL enable, unsigned int index);
int LCR_GetDeGammaIndex(BOOL *pEnable, unsigned int *pIndex);

#ifdef __cplusplus
}
#endif
#endif // API_H
