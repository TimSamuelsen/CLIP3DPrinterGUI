/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/

#ifndef BATCHFILE_H_
#define BATCHFILE_H_

/**
 * @brief This module handles batchfile related functions
 *
 * @note:
 *
 */

#include "error.h"
#include "common.h"
#include "API.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Abstract data type for batch file */
typedef struct BAT_BatchFile_t BAT_BatchFile_t;

/** Command information  data structure */
typedef struct
{
	uint08 I2CCmd;				/**< I2C Command number */
	uint16 USBCmd;
	uint08 *Payload;		/**< Payload buffer */
	char const *CmdName;	/**< Command Name */
	uint16 PayloadLen;		/**< Number of bytes in payload */
	void *Handle;			/**< Hanlde for internal use */
} BAT_CommandInfo_t;

BAT_BatchFile_t *BAT_Alloc(void);
void BAT_Free(BAT_BatchFile_t* BatchFile);
int BAT_Clear(BAT_BatchFile_t* BatchFile);
BAT_BatchFile_t *BAT_Copy(BAT_BatchFile_t* BatchFile);

int BAT_SetName(BAT_BatchFile_t *BatchFile, char const *Name);

int BAT_GetName(BAT_BatchFile_t *BatchFile, char *Name);

int BAT_AddCommand(BAT_BatchFile_t *BatchFile, int Index,
                                                    API_CommandInfo_t *CmdInfo);
int BAT_RemoveCommand(BAT_BatchFile_t *BatchFile, int Index);

int BAT_BuildData(BAT_BatchFile_t *BatchFile, uint08 **Data);
int BAT_GetCommandInfo(BAT_BatchFile_t *BatchFile, int Index, 
										BAT_CommandInfo_t *Info);
int BAT_NextCommandInfo(BAT_CommandInfo_t *Info);
int BAT_GetCommandCount(BAT_BatchFile_t *BatchFile);

int BAT_LoadFromFile(BAT_BatchFile_t *BatchFile, char const *FileName);
int BAT_LoadFromData(BAT_BatchFile_t *BatchFile, uint08 *Data, int Size);
int BAT_SaveToFile(BAT_BatchFile_t *BatchFile, char const *FileName);

int BAT_CheckSplashRef(BAT_BatchFile_t *BatchFile, uint16 SplashIndex);
int BAT_AdjustSplashIndex(BAT_BatchFile_t *BatchFile, uint16 SplashIndex);
int BAT_OTF2SplashPtn(BAT_BatchFile_t *BatchFile, int SplashOffset);

int BAT_IsOTFPtnPresent(BAT_BatchFile_t *BatchFile);

int BAT_GetSplashList(BAT_BatchFile_t *BatchFile, uint16 *SplashIndex);

#ifdef __cplusplus
}
#endif

#endif /* BATCHFILE_H_ */
