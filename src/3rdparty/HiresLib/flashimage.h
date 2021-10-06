#ifndef FLASHIMAGE_H_
#define FLASHIMAGE_H_

/**
 * This module provides the flash binary reading and manipulation functions
 */

#include "common.h"


#ifdef __cplusplus
extern "C" {
#endif

#define FI_MAX_FLASH_BLOCKS        1024

/** Converts Flash block type to printable string */
#define FI_BLOCK_TYPE2STR(Type, Str) do { \
        Str[0] = Type >> 24; \
        Str[1] = Type >> 16; \
        Str[2] = Type >>  8; \
        Str[3] = Type; \
        Str[4] = 0; \
        } while(0)

/** Flash Data Block Types */
typedef enum
{
    FI_BLOCK_ANY            = 0,           /**< Any Block */
    FI_BLOCK_APP            = 0x41505000,  /**< Application Binary */
    FI_BLOCK_OSD            = 0x4F534400,  /**< OSD data */
    FI_BLOCK_SPLASH         = 0x53530000,  /**< Splash data */
    FI_BLOCK_ASIC_CONFIG    = 0x41430000,  /**< ASIC Configuration data */
    FI_BLOCK_APP_CONFIG     = 0x4F430000,  /**< Application Config data */
    FI_BLOCK_OTHER          = 0x4F540000,  /**< Application Other Binary data */
    FI_BLOCK_SEQ            = 0x53510000,  /**< Sequence data */
	FI_BLOCK_BAT			= 0x42415443,  /**< Command batch file */
    FI_BLOCK_BOOT_LOADER    = 0x424C0000   /**< Bootloader */
} FI_BlockType_t;

/** Flash block information */
typedef struct
{
    FI_BlockType_t Type; /**< Type of the flash block */
    uint32 Index;        /**< Index of the flash block */
    uint32 FlashOffset;  /**< Start offset of the block in flash image */
    uint32 FlashAddress; /**< Physical address of the block in flash image */
    uint32 DataSize;     /**< Number of bytes in the block data */
} FI_BlockInfo_t;

/** Flash image information */
typedef struct
{
    uint32 FlashStartAddr;   /**< Physical start address of the flash image */
    uint32 FlashTableOffset; /**< Offset of the flash table start */
    uint32 TotalSize;        /**< Total size of the flash image */
    uint32 NumBlocks;        /**< Number of flash blocks present in the image */
} FI_FlashInfo_t;


/** Abstract data type for storing and manipulating flash image */
typedef struct FI_FlashImage_t FI_FlashImage_t;

FI_FlashImage_t *FI_LoadFromFile(char const *FileName);

void FI_FreeFlashImage(FI_FlashImage_t *Image);

int FI_SaveToFile(FI_FlashImage_t *FlashImage, char const *FileName);

int FI_GetFlashInfo(FI_FlashImage_t *FlashImage, FI_FlashInfo_t *Info);

int FI_SetFlashInfo(FI_FlashImage_t *FlashImage, FI_FlashInfo_t const *Info);

int FI_GetBlockInfo(FI_FlashImage_t *FlashImage, uint16 Index, 
														FI_BlockInfo_t *Info);

int FI_SetBlockData(FI_FlashImage_t *FlashImage, uint32 Index, uint08 *DataPtr,
																   uint32 Size);

int FI_GetBlockData(FI_FlashImage_t *FlashImage, uint32 Index, uint08 **DataPtr);

int FI_AddBlock(FI_FlashImage_t *FlashImage, FI_BlockType_t Type,
													uint08 *Data, uint32 Size);

int FI_RemoveBlock(FI_FlashImage_t *FlashImage, uint32 Index);

int FI_SaveBlockDataToFile(FI_FlashImage_t *FlashImage, uint32 Index, 
														char const *FileName);

int FI_GetNewBlockIndex(FI_FlashImage_t *FlashImage, FI_BlockType_t Type,
															int OrgIndex);
#ifdef __cplusplus
}
#endif


#endif /* FLASHIMAGE_H_ */
