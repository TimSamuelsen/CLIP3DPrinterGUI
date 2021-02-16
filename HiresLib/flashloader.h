/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/
#ifndef FLASHLOADER_H
#define FLASHLOADER_H

/**
 * This module provides the flash programing APIs
 */

#include "common.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    FL_OPER_VERIFY,
    FL_OPER_ERASE,
	FL_OPER_PROGRAM,
    FL_NUM_OPER
} FL_Operation_t;

typedef int FL_Callback_t(void *Param, FL_Operation_t Oper, 
									uint32 Address, uint08 Percent);

int FL_UpdateFlashInfo(char const *FlashParamFile);
int FL_EraseFlashRange(uint32 Offset, uint32 Size);
int FL_ProgramFlash(char const *FileName, uint32 Offset, uint32 Size, BOOL CompWithCache, char const *FileNameCached, BOOL *IsFlashContChanged, BOOL ProgramSignature);
BOOL FL_ProgramSignature(uint32 Offset, BOOL Program);
int FL_CheckFlash(char const *FileName, uint32 Offset, uint32 Size);
int FL_SetCallback(FL_Callback_t *Callback, void *Param);
int FL_UseLegacyCommands(BOOL Enable);

#ifdef __cplusplus
}
#endif

#endif // FLASHLOADER_H
