/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/
#ifndef FIRMWARE_H_
#define FIRMWARE_H_

/**
 * This module provides the flash binary reading and manipulation functions
 */

#include "common.h"
#include "Pattern.h"
#include "splash.h"
#include "BatchFile.h"
#include "FlashImage.h"
#include "splash.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	FW_CFG_STARTUP_STATE,
	FW_CFG_I2C_ADDRESS,
	FW_CFG_HDMI_ENABLE,
	FW_CFG_DEF_BATCH,
	FW_CFG_FW_TAG,
	FW_CFG_MASTER_DMD_A_SWAP,
	FW_CFG_MASTER_DMD_B_SWAP,
	FW_CFG_MASTER_DMD_AB_SWAP,
	FW_CFG_SLAVE_DMD_A_SWAP,
	FW_CFG_SLAVE_DMD_B_SWAP,
	FW_CFG_SLAVE_DMD_AB_SWAP,
	FW_CFG_FLASH_READ_DELAY,
	FW_CFG_FLASH_WRITE_DELAY,
	FW_CFG_FLASH_WRITE_WIDTH,
	FW_CFG_FLASH_CS_HOLD,
    FW_CFG_SEQ_CACHE,
    FW_CFG_SPREAD_SPECTRUM,
}FW_ConfigField_t;

typedef struct FW_Firmware_t FW_Firmware_t;

FW_Firmware_t *FW_LoadFromFile(char const *FileName);
void FW_Free(FW_Firmware_t *Firmware);
int FW_SaveToFile(FW_Firmware_t *Firmware, char const *FileName);

int FW_GetNumSplash(FW_Firmware_t const *Firmware);
int FW_GetNumBatchFiles(FW_Firmware_t const *Firmware);
int FW_GetFlashTableOffset(FW_Firmware_t const *Firmware);
int FW_SetFlashTableOffset(FW_Firmware_t *Firmware, int FlashTableOffset);

Image_t *FW_GetSplashImage(FW_Firmware_t *Firmware, int Index);
int FW_RemoveSplash(FW_Firmware_t *Firmware, int Index, BOOL UpdateBatch);
int FW_AddSplash(FW_Firmware_t *Firmware, Image_t *SplashImage, SPL_Compression_t compression);

BAT_BatchFile_t *FW_GetBatchFile(FW_Firmware_t *Firmware, int Index);
int FW_AddBatchFile(FW_Firmware_t *Firmware, BAT_BatchFile_t *BatchFile);
int FW_RemoveBatchFile(FW_Firmware_t *Firmware, int Index, BOOL RemveSplash);

int FW_GetConfigBits(FW_Firmware_t *Firmware, FW_ConfigField_t Field,
														uint32 *Value);
int FW_GetConfigBytes(FW_Firmware_t *Firmware, FW_ConfigField_t Field,
														uint08 *Value);
int FW_SetConfigBits(FW_Firmware_t *Firmware, FW_ConfigField_t Field, 
														uint32 Value);
int FW_SetConfigBytes(FW_Firmware_t *Firmware, FW_ConfigField_t Field, 
														uint08 *Value);

int FW_GetBLVersion(FW_Firmware_t *Firmware, int *Major, int *Minor, int *Patch);

int FW_GetBlockInfo(FW_Firmware_t *Firmware, int Index, FI_BlockInfo_t *Info);

#ifdef __cplusplus
}
#endif


#endif /* FIRMWARE_H_ */
