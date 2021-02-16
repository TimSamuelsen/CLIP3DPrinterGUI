/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/

/**
 * This module provides high level APIs for manipulating Firmware image.
 * This should be used instead of FlashImage module
 *
 */

#include "Pattern.h"
#include "Splash.h"
#include "batchfile.h"
#include "Firmware.h"
#include "FlashImage.h"

#define FW_MAX_BLOCKS		512

/** Information on batch file block */
typedef struct 
{
	uint16 Index;
	uint16 ID;
	BAT_BatchFile_t *BatchFile;
} FW_BatchInfo_t;

/** Information on splash block */
typedef struct
{
    uint16 Index;
	uint16 ID;
	Image_t *SplashImage;
} FW_SplashInfo_t;

/** Firmware data structure */
struct FW_Firmware_t
{
    FI_FlashImage_t *FlashImage;
	FW_BatchInfo_t BatchList[FW_MAX_BLOCKS];
	FW_SplashInfo_t SplashList[FW_MAX_BLOCKS];
    int NumSplash;
    int NumBatch;
};

/** Information on configuration data */
typedef struct
{
	FW_ConfigField_t Field;      /* Config field number */
	FI_BlockType_t BlockType;    /* Flash block where the data is present */
	uint16 ByteOffset;           /* Byte offset within the data */
	uint08 BitOffset;            /* Bit offset within the data */
	uint16 NumBits;              /* Number of bits in the data */
} FW_ConfigInfo_t;

static const FW_ConfigInfo_t FW_ConfigInfo[] =
{
	{	FW_CFG_STARTUP_STATE,		FI_BLOCK_APP_CONFIG,	1,	0,	8		},
    {	FW_CFG_SPREAD_SPECTRUM,		FI_BLOCK_APP_CONFIG,	3,	0,	8		},
	{	FW_CFG_MASTER_DMD_A_SWAP,   FI_BLOCK_APP_CONFIG,	8,	0,	1		},
	{	FW_CFG_MASTER_DMD_B_SWAP,   FI_BLOCK_APP_CONFIG,	8,	1,	1		},
	{	FW_CFG_MASTER_DMD_AB_SWAP,  FI_BLOCK_APP_CONFIG,	8,	2,	1		},
	{	FW_CFG_SLAVE_DMD_A_SWAP,    FI_BLOCK_APP_CONFIG,	9,	0,	1		},
	{	FW_CFG_SLAVE_DMD_B_SWAP,    FI_BLOCK_APP_CONFIG,	9,	1,	1		},
	{	FW_CFG_SLAVE_DMD_AB_SWAP,   FI_BLOCK_APP_CONFIG,	9,	2,	1		},
	{	FW_CFG_I2C_ADDRESS,         FI_BLOCK_APP_CONFIG,	22,	0,	8		},	
	{	FW_CFG_HDMI_ENABLE,         FI_BLOCK_APP_CONFIG,	28,	0,	1		},
    {	FW_CFG_DEF_BATCH,           FI_BLOCK_APP_CONFIG,	72,	0,	8*2		},
    {	FW_CFG_SEQ_CACHE,           FI_BLOCK_APP_CONFIG,	74,	0,	8*2		},
    {	FW_CFG_FW_TAG,              FI_BLOCK_APP_CONFIG,	76,	0,	8*32	},
    {	FW_CFG_FLASH_READ_DELAY,    FI_BLOCK_APP,			36,	0,	8		},
	{	FW_CFG_FLASH_WRITE_DELAY,   FI_BLOCK_APP,			37,	0,	8		},
	{	FW_CFG_FLASH_WRITE_WIDTH,   FI_BLOCK_APP,			38,	0,	8		},
	{	FW_CFG_FLASH_CS_HOLD,       FI_BLOCK_APP,			39,	0,	8		}
};

static int FW_RemoveSplashsInBatch(FW_Firmware_t *Firmware, int BatchIndex);
static int FW_UpdateBlockIndex(FW_Firmware_t *Firmware, int RemoveIndex);
FW_ConfigInfo_t const *FW_GetConfig(FW_Firmware_t *Firmware, 
										FW_ConfigField_t Field, uint08 **Data);
static int MemSearch(uint08 const *Data, int DataLen, uint08 const *Search, 
															int SearchLen);

/**
 * Initializes the firmware data structure using the given file content
 *
 * @param FileName Flash image file name
 */
FW_Firmware_t *FW_LoadFromFile(char const *FileName)
{
	int Error;
	FW_Firmware_t *Firmware = NULL;
	FI_FlashInfo_t FlashInfo;
    uint16 Index;

	TRY_BEGIN()
	{
		Firmware = malloc(sizeof(*Firmware));
		if(Firmware == NULL)
			TRY_THROW(ERR_OUT_OF_RESOURCE);

		Firmware->FlashImage = NULL;
		Firmware->NumSplash = 0;
		Firmware->NumBatch = 0;

		Firmware->FlashImage = FI_LoadFromFile(FileName);
		if(Firmware->FlashImage == NULL)
			TRY_THROW(FAIL);

		TRY_TRY(FI_GetFlashInfo(Firmware->FlashImage, &FlashInfo));

		/* Count all the blocks in the firmware image */
		for(Index = 0; Index < FlashInfo.NumBlocks; Index++)
		{
			FI_BlockInfo_t BlockInfo;
			int Size;
			uint08 *Data;

			TRY_TRY(FI_GetBlockInfo(Firmware->FlashImage, Index, &BlockInfo));

			if(BlockInfo.Type == FI_BLOCK_SPLASH)
			{
				SPL_Info_t Info;
				Image_t *SplashImage;

				if(Firmware->NumSplash >= FW_MAX_BLOCKS)
					TRY_THROW(ERR_OUT_OF_RESOURCE);

				TRY_TRY(Size = FI_GetBlockData(Firmware->FlashImage, Index, &Data));

				TRY_TRY(SPL_GetSplashImageInfo(Data, &Info));

                SplashImage = PTN_Alloc(Info.Width, Info.Height, 24, PTN_RGB24);
				if(SplashImage == NULL)
					TRY_THROW_MSG(ERR_OUT_OF_RESOURCE, 
							"Unable to allocate memory for the Splash image");

				if(SPL_ConvSplashToImage(Data, SplashImage))
				{
					PTN_Free(SplashImage);
					TRY_THROW(FAIL);
				}

				Firmware->SplashList[Firmware->NumSplash].Index = Index;
				Firmware->SplashList[Firmware->NumSplash].ID = BlockInfo.Index;
				Firmware->SplashList[Firmware->NumSplash].SplashImage = SplashImage;
				Firmware->NumSplash++;
			}
			else if(BlockInfo.Type == FI_BLOCK_BAT)
			{
				BAT_BatchFile_t *BatchFile;

				if(Firmware->NumBatch >= FW_MAX_BLOCKS)
					TRY_THROW(ERR_OUT_OF_RESOURCE);

				TRY_TRY(Size = FI_GetBlockData(Firmware->FlashImage, Index, &Data));

				BatchFile = BAT_Alloc();
				if(BatchFile == NULL)
					TRY_THROW(ERR_OUT_OF_RESOURCE);

				if(BAT_LoadFromData(BatchFile, Data, Size) < 0)
				{
					BAT_Free(BatchFile);
					TRY_THROW(FAIL);
				}
				Firmware->BatchList[Firmware->NumBatch].Index = Index;
				Firmware->BatchList[Firmware->NumBatch].ID = BlockInfo.Index;
				Firmware->BatchList[Firmware->NumBatch].BatchFile = BatchFile;
				Firmware->NumBatch++;
			}
		}
	}
	TRY_END(Error);

	if(Error)
	{
		FW_Free(Firmware);
		Firmware = NULL;
	}

	return Firmware;
}

/**
 * Deallocate all the memory related to the firmware image data structure
 *
 * @param Firmware Firmware data structure initialized using 
 *        FW_LoadFromFile()
 */
void FW_Free(FW_Firmware_t *Firmware)
{
	if(Firmware)
	{
        int i;
        for(i = 0; i < Firmware->NumSplash; i++)
        {
        PTN_Free(Firmware->SplashList[i].SplashImage);
        }

        for(i = 0; i < Firmware->NumBatch; i++)
        {
            if(Firmware->BatchList[i].BatchFile != NULL)
            BAT_Free(Firmware->BatchList[i].BatchFile);
        }
        FI_FreeFlashImage(Firmware->FlashImage);
        free(Firmware);
        Firmware = NULL;
	}
}


/**
 * Gets the number of splash images in the firmware
 *
 * @param Firmware Firmware data structure loaded using FW_LoadFromFile()
 *
 * @return Number of splash images in the firmware image
 */
int FW_GetNumSplash(FW_Firmware_t const *Firmware)
{
	return Firmware->NumSplash;
}


/**
 * Gets the number of batch files in the firmware
 *
 * @param Firmware Firmware data structure loaded using FW_LoadFromFile()
 *
 * @return Number of batch files in the firmware image
 */
int FW_GetNumBatchFiles(FW_Firmware_t const *Firmware)
{
	return Firmware->NumBatch;
}

/**
 * Gets the flash table offset of the firmware image
 *
 * @param Firmware Firmware data structure loaded using FW_LoadFromFile()
 *
 * @return Flash table offset, -ve in case of failure
 */
int FW_GetFlashTableOffset(FW_Firmware_t const *Firmware)
{
	FI_FlashInfo_t FlashInfo;

	if(FI_GetFlashInfo(Firmware->FlashImage, &FlashInfo))
		return -1;

	return FlashInfo.FlashTableOffset;
}

/**
 * Sets the flash table offset of the firmware image
 *
 * @param Firmware Firmware data structure loaded using FW_LoadFromFile()
 *
 * @return 0 on Success, -ve on failure
 */
int FW_SetFlashTableOffset(FW_Firmware_t *Firmware, int FlashTableOffset)
{
	FI_FlashInfo_t FlashInfo;
	if(FI_GetFlashInfo(Firmware->FlashImage, &FlashInfo))
		return -1;
	FlashInfo.FlashTableOffset = FlashTableOffset;

	return FI_SetFlashInfo(Firmware->FlashImage, &FlashInfo);
}


/**
 * Saves the given firmware image to a file
 *
 * @param Firmware Firmware data structure loaded using FW_LoadFromFile()
 * @param FileName File name to save the firmware image
 * @return 0 on Success and -ve on failure
 */

int FW_SaveToFile(FW_Firmware_t *Firmware, char const *FileName)
{
	int i;
	/* Update all the batchfiles */
	for(i = 0; i < Firmware->NumBatch; i++)
	{
		uint08 *Data;
		int Size;
		TRY(Size = BAT_BuildData(Firmware->BatchList[i].BatchFile, &Data));
		TRY(FI_SetBlockData(Firmware->FlashImage, 
								Firmware->BatchList[i].Index, Data, Size));
	}

	return FI_SaveToFile(Firmware->FlashImage, FileName);
}

/**
 * Gets the n-th splash image from the firmware binary
 *
 * @param Firmware Firmware data structure loaded using FW_LoadFromFile()
 * @param index Splash image index in the firmware binary
 *
 * @return 0 on success, -1 on failure
 */
Image_t *FW_GetSplashImage(FW_Firmware_t *Firmware, int Index)
{
    if(Index >= Firmware->NumSplash)
		THROW_MSG(NULL, "Invalid splash image index");

    return Firmware->SplashList[Index].SplashImage;
}

/**
 * Delets the splash image from the firmware binary
 *
 * @param Firmware Firmware data structure loaded using FW_LoadFromFile()
 * @param index Splash image index to be removed
 *
 * @return 0 on success, -1 on failure
 */
int FW_RemoveSplash(FW_Firmware_t *Firmware, int Index, BOOL UpdateBatch)
{
	int i;
    if(Index >= Firmware->NumSplash)
		THROW_MSG(ERR_INVALID_PARAM, "Invalid Splash Index to remove");

    TRY(FI_RemoveBlock(Firmware->FlashImage, Firmware->SplashList[Index].Index));

	if(UpdateBatch)
	{
		for(i = 0; i < Firmware->NumBatch; i++)
		{
			TRY(BAT_AdjustSplashIndex(Firmware->BatchList[i].BatchFile, 
										Firmware->SplashList[Index].ID));
		}
	}

    TRY(FW_UpdateBlockIndex(Firmware, Firmware->SplashList[Index].Index));

    return SUCCESS;
}


/**
 * Adds the given pattern image to the firmware data structure
 *
 * @param Firmware Firmware data structure loaded using FW_LoadFromFile()
 * @param splashImage Splash image
 *
 * @return
 */
int FW_AddSplash(FW_Firmware_t *Firmware, Image_t *SplashImage, SPL_Compression_t compression)
{
    int Size;
    uint08 *Data = NULL;
	FI_BlockInfo_t BlockInfo;
	int Error;
    int Index;

	TRY_BEGIN()
	{
		Data = SPL_AllocSplash(SplashImage->Width, SplashImage->Height);
		if(Data == NULL)
			TRY_THROW(ERR_OUT_OF_RESOURCE);

        TRY_TRY(Size = SPL_ConvImageToSplash(SplashImage, compression, Data));

        TRY_TRY(Index = FI_AddBlock(Firmware->FlashImage, FI_BLOCK_SPLASH, Data, Size));

		TRY_TRY(FI_GetBlockInfo(Firmware->FlashImage, Index, &BlockInfo));

		Firmware->SplashList[Firmware->NumSplash].SplashImage = SplashImage;
        Firmware->SplashList[Firmware->NumSplash].Index = Index;
        Firmware->SplashList[Firmware->NumSplash].ID = BlockInfo.Index;

		Firmware->NumSplash ++;
	}
	TRY_END(Error);

	SPL_Free(Data);

    return Error;
}

/**
 * Gets the nth batch file from the firmware binary
 *
 * @param Firmware Firmware data structure loaded using FW_LoadFromFile()
 * @param index Index of the batch file to be returned
 *
 * @return Pointer to the batch file. NULL on failure
 */
BAT_BatchFile_t *FW_GetBatchFile(FW_Firmware_t *Firmware, int Index)
{
    if(Index >= Firmware->NumBatch)
		THROW_MSG(NULL, "Invalid Batch file index");

    return Firmware->BatchList[Index].BatchFile;
}

/**
 * Add the given batch file to the firmware data structure
 *
 * @param Firmware Firmware data structure loaded using FW_LoadFromFile()
 * @param batchFile batch file to be added
 *
 * @return 0 on success, -1 on failure
 */
int FW_AddBatchFile(FW_Firmware_t *Firmware, BAT_BatchFile_t *BatchFile)
{
    int Size;
    uint08 *Data;
    int Index;
    FI_BlockInfo_t BlockInfo;

	TRY(Size = BAT_BuildData(BatchFile, &Data));

    TRY(Index = FI_AddBlock(Firmware->FlashImage, FI_BLOCK_BAT, Data, Size));

    TRY(FI_GetBlockInfo(Firmware->FlashImage, Index, &BlockInfo));

	Firmware->BatchList[Firmware->NumBatch].BatchFile = BatchFile;
    Firmware->BatchList[Firmware->NumBatch].Index = Index;
    Firmware->BatchList[Firmware->NumBatch].ID = BlockInfo.Index;

	Firmware->NumBatch++;

    return SUCCESS;
}

/**
 * Remove the given batch file from the firmware data structure
 *
 * @param Firmware Firmware data structure loaded using FW_LoadFromFile()
 * @param index
 *
 * @return 0 on success, -1 on failure
 */
int FW_RemoveBatchFile(FW_Firmware_t *Firmware, int Index, BOOL RemoveSplash)
{
    if(Index >= Firmware->NumBatch)
		THROW(ERR_INVALID_PARAM);

	if(RemoveSplash)
	{
		TRY(FW_RemoveSplashsInBatch(Firmware, Index));
	}

    TRY(FI_RemoveBlock(Firmware->FlashImage, Firmware->BatchList[Index].Index));

    TRY(FW_UpdateBlockIndex(Firmware, Firmware->BatchList[Index].Index));

    return 0;
}

/**
 * Get the value of one of the firmware config field  (1-31 bit number)
 *
 * @param Firmware Firmware data structure loaded using FW_LoadFromFile()
 * @param Field The enum/number of the field
 * @param [out] Value Value of the field
 *
 * @return 0 on success, -1 on failure
 *
 */
int FW_GetConfigBits(FW_Firmware_t *Firmware, FW_ConfigField_t Field, 
														uint32 *Value)
{
	uint08 *Data;
	FW_ConfigInfo_t const *CfgInfo = FW_GetConfig(Firmware, Field, &Data);
	if(CfgInfo == NULL)
		THROW(FAIL);

	if(CfgInfo->NumBits > 32)
		THROW(FAIL);

	*Value = 0;
	memcpy(Value, Data + CfgInfo->ByteOffset, DIV_CEIL(CfgInfo->NumBits,8));
	*Value >>= CfgInfo->BitOffset;
	*Value &= GEN_BIT_MASK(0, CfgInfo->NumBits);

	return SUCCESS;
}

/**
 * Get the value of one of the firmware config field (byte array)
 *
 * @param Firmware Firmware data structure loaded using FW_LoadFromFile()
 * @param Field The enum/number of the field
 * @param [out] Value Value of the field
 *
 * @return 0 on success, -1 on failure
 *
 */
int FW_GetConfigBytes(FW_Firmware_t *Firmware, FW_ConfigField_t Field, 
														uint08 *Value)
{
	uint08 *Data;
	FW_ConfigInfo_t const *CfgInfo = FW_GetConfig(Firmware, Field, &Data);

	if(CfgInfo == NULL)
		THROW(FAIL);

	if((CfgInfo->NumBits % 8) != 0)
		THROW(FAIL);

	memcpy(Value, Data + CfgInfo->ByteOffset, DIV_CEIL(CfgInfo->NumBits,8));

	return SUCCESS;
}

/**
 * Sets the value of one of the firmware config field (1 to 32 bits)
 *
 * @param Firmware Firmware data structure loaded using FW_LoadFromFile()
 * @param Field The enum/number of the field
 * @param Value Value of the field
 *
 * @return 0 on success, -1 on failure
 *
 */
int FW_SetConfigBits(FW_Firmware_t *Firmware, FW_ConfigField_t Field, 
														uint32 Value)
{
	uint08 *Data;
	uint32 OldValue;
	uint32 Mask;

	FW_ConfigInfo_t const *CfgInfo = FW_GetConfig(Firmware, Field, &Data);
	if(CfgInfo == NULL)
		THROW(FAIL);

	if(CfgInfo->NumBits > 32)
		THROW(FAIL);

	OldValue = 0;
	memcpy(&OldValue, Data + CfgInfo->ByteOffset, DIV_CEIL(CfgInfo->NumBits,8));
	Value <<= CfgInfo->BitOffset;
	Mask = GEN_BIT_MASK(CfgInfo->BitOffset, CfgInfo->NumBits);
	Value = MERGE_BITS(OldValue, Value, Mask);
	memcpy(Data + CfgInfo->ByteOffset, &Value, DIV_CEIL(CfgInfo->NumBits,8));

	return SUCCESS;
}

/**
 * Sets the value of one of the firmware config field (byte array)
 *
 * @param Firmware Firmware data structure loaded using FW_LoadFromFile()
 * @param Field The enum/number of the field
 * @param Value Value of the field
 *
 * @return 0 on success, -1 on failure
 *
 */
int FW_SetConfigBytes(FW_Firmware_t *Firmware, FW_ConfigField_t Field, 
															uint08 *Value)
{
	uint08 *Data;

	FW_ConfigInfo_t const *CfgInfo = FW_GetConfig(Firmware, Field, &Data);
	if(CfgInfo == NULL)
		THROW(FAIL);

	if((CfgInfo->NumBits % 8) != 0)
		THROW(FAIL);

	memcpy(Data + CfgInfo->ByteOffset, Value, DIV_CEIL(CfgInfo->NumBits,8));

	return SUCCESS;
}

/**
 * Gets one flash block related information
 *
 * @param Firmware Firmware image data structure loaded using
 *                   \FW_LoadFromFile()
 * @param Index Index of the flash block  (for which the information is needed)
 * @Info Flash block information
 *
 * @return ERR_NOT_FOUND, ERR_NULL_PTR, SUCCESS
 *
 */
int FW_GetBlockInfo(FW_Firmware_t *Firmware, int Index, FI_BlockInfo_t *Info)
{
    return FI_GetBlockInfo(Firmware->FlashImage, Index, Info);
}

/**
 * Read the bootloader version stored in the firmware
 *
 * @param Firmware Firmware data structure loaded using FW_LoadFromFile()
 * @param Major Major version number
 * @param Minor Minor version number
 * @param Patch Patch number
 *
 * @return SUCCESS/ERR_NULL_PTR/ERR_NOT_FOUND
 *
 */
int FW_GetBLVersion(FW_Firmware_t *Firmware, int *Major, int *Minor, int *Patch)
{
	uint08 *Data;
	static uint08 VersionTag[] = "__BLVERSION__";
	int Length;
	int Index;

	if(Firmware == NULL)
		THROW(ERR_NULL_PTR);

	TRY(Length = FI_GetBlockData(Firmware->FlashImage, 0, &Data));

	Index = MemSearch(Data, Length, VersionTag, sizeof(VersionTag));
	if(Index < 0)
		THROW_MSG(ERR_NOT_FOUND, "Unable to find bootlader version");

	Index += sizeof(VersionTag);

	*Major = Data[Index];
	*Minor = Data[Index + 1];
	*Patch = MAKE_WORD16(Data[Index + 3], Data[Index + 2]);

    return SUCCESS;
}

/**
 * Remove all the splash images referred by the batch file
 *
 * @param Firmware Firmware data structure loaded using FW_LoadFromFile()
 * @param batchFile
 *
 * @return 0 on success, -1 on failure
 */
static int FW_RemoveSplashsInBatch(FW_Firmware_t *Firmware, int BatchIndex)
{
	int Count;
	int i;

    if(Firmware == NULL)
		THROW(ERR_NULL_PTR);

	Count = Firmware->NumSplash;

	while(Count > 0)
	{
		BOOL Remove = TRUE;
		Count --;
		for(i = 0; i < Firmware->NumBatch; i++)
		{
			if(BAT_CheckSplashRef(Firmware->BatchList[i].BatchFile, 
                                          Firmware->SplashList[Count].ID))
			{
                /* if other batchfiles referring to it */
                if(i != BatchIndex)
                {
                    Remove = FALSE;
                    break;
                }
			}
            else
            {
                /* if this batch file not referring to it */
                if(i == BatchIndex)
                {
                    Remove = FALSE;
                    break;
                }
            }
		}

        if(Remove == TRUE)
		{
            TRY(FW_RemoveSplash(Firmware, Count, TRUE));
		}
	}

    return SUCCESS;
}

/**
 * Adjustes all the block indxes for removing one index
 *
 * @param Firmware Firmware ata structure
 * @param RemoveIndex Index to be removed
 * @return
 */
static int FW_UpdateBlockIndex(FW_Firmware_t *Firmware, int RemoveIndex)
{

    BOOL Move = FALSE;
    int i;

    for(i = 0; i < Firmware->NumSplash; i++)
    {
        if(Firmware->SplashList[i].Index == RemoveIndex)
        {
            Move = TRUE;
        }
        else if(Firmware->SplashList[i].Index > RemoveIndex)
        {
            Firmware->SplashList[i].Index--;
            if(Move)
            {
                Firmware->SplashList[i-1] = Firmware->SplashList[i];
            }
        }
    }
    Firmware->NumSplash -= Move;


    Move = FALSE;
    for(i = 0; i < Firmware->NumBatch; i++)
    {
        if(Firmware->BatchList[i].Index == RemoveIndex)
        {
            Move = TRUE;
        }
        else if(Firmware->BatchList[i].Index > RemoveIndex)
        {
            Firmware->BatchList[i].Index--;
            if(Move)
            {
                Firmware->BatchList[i-1] = Firmware->BatchList[i];
            }
        }
    }

    Firmware->NumBatch -= Move;

    return SUCCESS;
}

/**
 * Gets the information about a firmware config field
 *
 * @param Firmware Firmware data structure loaded using FW_LoadFromFile()
 * @param [out] Data Pointer to the beginning of the config data
 *
 * @return Pointer to the config info data structure
 */
FW_ConfigInfo_t const *FW_GetConfig(FW_Firmware_t *Firmware, 
										FW_ConfigField_t Field, uint08 **Data)
{
    uint16 CfgIndex;
    uint16 BlkIndex;
	FW_ConfigInfo_t const *CfgInfo;
	int Size;
	int NumBytes;
	FI_FlashInfo_t FlashInfo;

	if(Firmware == NULL)
		THROW(NULL);

	CfgInfo = NULL;

	for(CfgIndex = 0; CfgIndex < ARRAY_SIZE(FW_ConfigInfo); CfgIndex++)
	{
		if(FW_ConfigInfo[CfgIndex].Field == Field)
		{
			CfgInfo = &FW_ConfigInfo[CfgIndex];
			break;
		}
	}
	if(CfgInfo == NULL)
		THROW_MSG(NULL, "Unknown config filed requested :%d", Field);

	if(FI_GetFlashInfo(Firmware->FlashImage, &FlashInfo))
		THROW(NULL);

	/* Count all the blocks in the firmware image */
	for(BlkIndex = 0; BlkIndex < FlashInfo.NumBlocks; BlkIndex++)
	{
		FI_BlockInfo_t BlockInfo;

		if(FI_GetBlockInfo(Firmware->FlashImage, BlkIndex, &BlockInfo))
			THROW(NULL);

		if(BlockInfo.Type == FW_ConfigInfo[CfgIndex].BlockType)
		{
			break;
		}
	}

	if(BlkIndex == FlashInfo.NumBlocks)
		THROW(NULL);

	if((Size = FI_GetBlockData(Firmware->FlashImage, BlkIndex, Data)) < 0)
		THROW(NULL);

	NumBytes = DIV_CEIL(FW_ConfigInfo[CfgIndex].NumBits,8);

	if(Size < FW_ConfigInfo[CfgIndex].ByteOffset + NumBytes)
		THROW(NULL);

	return CfgInfo;
}

/**
 * Search the memory for a given string
 *
 * @param Data Data memory to search
 * @param DataLen Number of bytes in the Data memory
 * @param Search Search string
 * @param SearchLen Number of bytes in search string
 *
 * @return First index where the string found, -1 if not found
 */
static int MemSearch(uint08 const *Data, int DataLen, uint08 const *Search, 
															int SearchLen)
{
	int DataIndex;
	int SearchIndex;

	DataLen -= SearchLen;

	for(DataIndex = 0; DataIndex <= DataLen; DataIndex++)
	{
		for(SearchIndex = 0; SearchIndex < SearchLen; SearchIndex++)
		{
			if(Data[SearchIndex] != Search[SearchIndex])
				break;
		}
		if(SearchIndex == SearchLen)
			return DataIndex;
		Data ++;
	}

	return -1;
}
