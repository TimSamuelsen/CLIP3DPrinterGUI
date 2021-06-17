/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/

/**
 * This module provides the flash binary reading and manipulation functions
 */

#include <stdio.h> 
#include <stdlib.h>
#include "error.h"
#include "common.h"
#include "flashimage.h"

#define BOOT_SECTOR_SIZE_MIN    0x8000
#define FI_INVALID_INDEX		0xFFFF

/***************************** Data Types *************************************/

/** Flash chip information */
typedef struct
{
    uint32 Start; /**< Start address off the flash chip */
    uint32 Size;  /**< Capacity of the flash chip in bytes */
} FI_FlashChipInfo_t;

/** Information of all the flash chips in the system */
static const FI_FlashChipInfo_t FlashChipInfo[] =
{
    { 0xF9000000UL, 16UL * 1024 * 1024 },//0-16MB
    { 0xFA000000UL, 16UL * 1024 * 1024 },//16-32MB
    { 0xF8000000UL, 16UL * 1024 * 1024 },//32-48MB
	{ 0x03000000UL, 16UL * 1024 * 1024 },//48-64MB
    { 0x04000000UL, 16UL * 1024 * 1024 },//64-80MB
    { 0x05000000UL, 16UL * 1024 * 1024 },//80-96MB
    { 0x06000000UL, 16UL * 1024 * 1024 },//96-112MB
    { 0x07000000UL, 16UL * 1024 * 1024 },//112-128MB
    { 0x08000000UL, 16UL * 1024 * 1024 },//128-144MB
    { 0x09000000UL, 16UL * 1024 * 1024 },//144-160MB
    { 0x0A000000UL, 16UL * 1024 * 1024 },//160-176MB
    { 0x0B000000UL, 16UL * 1024 * 1024 },//176-192MB
    { 0x0C000000UL, 16UL * 1024 * 1024 },//192-208MB
    { 0x0D000000UL, 16UL * 1024 * 1024 },//208-224MB
    { 0x0E000000UL, 16UL * 1024 * 1024 },//224-240MB
    { 0x0F000000UL, 16UL * 1024 * 1024 }//240-256MB
};

/** Flash block related information (stored in flash) */
typedef struct
{
    uint32 Type;          /**< Flash Block type */
    uint08 ID;            /**< Flash Block index */
    uint08 Reserved1;
    uint16 Reserved2;
    uint32 FlashAddress;  /**< Physical flash address for the block start */
    uint32 ByteCount;     /**< Number of bytes in the block */
} FI_FlashBlock_t;


/** Flash Table Header */
typedef struct
{
  uint32 Signature;        /**< Unique signature for the table 0x1234567 */
  uint32 Boot_Address;     /**< Address of Application entry to bootloader */
  uint16 Version;          /**< Version (0x13) */
  uint16 FlashBlockCount;  /**< Number of Flash block entries present in the 
							    flash image */
  uint32 Free_Area_Start;  /**< Address of first free location in flash */
} FI_FlashTable_t;

/** Flash block information (stored in RAM) */
typedef struct
{
    char File[512];     /**< File name where the content of the data is */
    uint08 *Data;       /**< Pointer to the content of th data - if in memory */
    uint32 DataOffset;  /**< Offset to start of the data (mem/file) */
    uint32 DataSize;    /**< Number of bytes in the data */
    uint32 FlashOffset; /**< Offset in flash for the block */
    uint32 Type;        /**< Type of the flash block */
    uint16 Index;       /**< Index of the flash data */
    uint16 OrgIndex;    /**< Index of the flash data while loading */
} FI_FlashBlockInfo_t;

/** Flash image related information */
struct FI_FlashImage_t
{
    uint32 FlashTableOffset; /**< Flash table start offset */
    uint32 TotalSize;        /**< Total size of the flash image */
    uint32 FlashBlockCount;  /**< Number of flash blocks in the image */
    FI_FlashBlockInfo_t FlashBlockInfo[FI_MAX_FLASH_BLOCKS];
                             /**< Information on each block */
};

/*************************** Local Variables **********************************/
/** Temp buffer for internal use */
static uint8 Buffer[1024*64];
/** Scratch pad for reading flash table header */
static FI_FlashTable_t FlashTable;
/** Scratch pad for reading full flash table (Block information) */
static FI_FlashBlock_t FlashBlock[FI_MAX_FLASH_BLOCKS];

/*************************** Local Functions **********************************/
/**
 * This function adds FF to the output file till the given offset.
 * @param Out Output file pointer
 * @param Offset Byte offset till what to pad
 *
 * @return FAIL/SUCCESS
 */
static int PadFile(FILE *Out, int Offset)
{
    int Size = Offset - ftell(Out);

    memset(Buffer, 0xFF, sizeof(Buffer));

    while(Size > 0)
    {
        int Chunk = sizeof(Buffer);

        if(Chunk > Size)
            Chunk = Size;

        if(fwrite(Buffer, 1, Chunk, Out) != (unsigned)Chunk)
            return FAIL;

        Size = Size - Chunk;
    }

    return SUCCESS;
}

/**
 * This function copies the input file content to output file till the given 
 * size
 *
 * @param Inp Input file pointer
 * @param Out Output file pointer
 * @param Size Number of byte to copy. If 0 then Copy till end of file
 *
 * @return FAIL if file read/write failed. SUCCESS otherwise
 */
static int CopyFile(FILE *Inp, FILE *Out, int Size)
{
    if(Size > 0)
    {
		/* Copy chuck (size of buffer) at a time  */
        while(Size > 0)
        {
            int Chunk = sizeof(Buffer);
            int ReadLen;

            if(Chunk > Size)
                Chunk = Size;

            ReadLen = fread(Buffer, 1, Chunk, Inp);

            if(ReadLen < Chunk)
            {
                if(ReadLen < 0)
                    ReadLen = 0;

                memset(Buffer + ReadLen, 0xFF, Chunk - ReadLen);
            }

            if(fwrite(Buffer, 1, Chunk, Out) != (unsigned)Chunk)
                return FAIL;

            Size = Size - Chunk;
        }
    }
    else
    {
        while(1)
        {
            unsigned Chunk = sizeof(Buffer);

            Chunk = fread(Buffer, 1, Chunk, Inp);
            if(Chunk <= 0)
                break;

            if(fwrite(Buffer, 1, Chunk, Out) != Chunk)
                return FAIL;
        }
    }
    return SUCCESS;
}

/**
 * This function opens the input file and copy the content to output file
 *
 * @param InpFileName Name of the input file
 * @param Offset Byte offset in input file from where to read the bytes from
 * @param Out Output file pointer
 * @param Size Number of bytes to copy
 *
 * @return FAIL if any of the file operation failed, SUCCESS otherwise
 */
static int OpenCopyFile(char const *InpFileName, unsigned Offset, FILE *Out,
                        unsigned Size)
{
    FILE *Inp = NULL;
    int Error;

    TRY_BEGIN()
    {
        if(InpFileName == NULL || Out == NULL)
            TRY_THROW(ERR_NULL_PTR);

        Inp = fopen(InpFileName, "rb");
        if(Inp == NULL)
            TRY_THROW_MSG(FAIL, "Unable to open %s", InpFileName);

        if(fseek(Inp, Offset, SEEK_SET < 0))
            TRY_THROW_MSG(FAIL, "Error while reading %s", InpFileName);

        TRY_TRY(CopyFile(Inp, Out, Size));
    }
    TRY_END(Error);

    if(Inp)
        fclose(Inp);

    return Error;
}

/**
 * Writes the content of one flash block to the given file
 * @param BlockInfo The block information of the block to be written
 * @param OutFile Output file pointer
 *
 * @return FAIL if any of the file operation has failed, SUCCESS otherwise
 */
static int FI_WriteBlockToFile(FI_FlashBlockInfo_t *BlockInfo, FILE *OutFile)
{
    if(PadFile(OutFile, BlockInfo->FlashOffset))
        THROW(FAIL);

    if(BlockInfo->Data != NULL)
    {
        if(fwrite(BlockInfo->Data, 1,
                    BlockInfo->DataSize, OutFile) != BlockInfo->DataSize)
            THROW_MSG(FAIL, "File Write Error");
    }
    else
    {
        if(OpenCopyFile(BlockInfo->File, BlockInfo->DataOffset, OutFile,
                                                   BlockInfo->DataSize))
            THROW(FAIL);
    }

    return SUCCESS;
}

/**
 * Convert flash address to binary image offset
 *
 * @param Address Physical address in flash
 * @return Offset in bytes to the location in binary image
 */
static uint32 FI_FlashAddressToOffset(uint32 Address)
{
    unsigned i;
    uint32 Offset = 0;

    for(i = 0; i < ARRAY_SIZE(FlashChipInfo); i++)
    {
        if(FlashChipInfo[i].Start <= Address &&
                Address < FlashChipInfo[i].Start + FlashChipInfo[i].Size)
        {
            return Address - FlashChipInfo[i].Start + Offset;
        }
        Offset += FlashChipInfo[i].Size;
    }

    return 0;
}

/**
 * Convert offset in binary image to physical address in splash
 *
 * @param Offset Byte offset in binary image
 * @return Physical address in flash
 */
static uint32 FI_FlashOffsetToAddress(uint32 Offset)
{
    unsigned i;
    uint32 Size = 0;

    for(i = 0; i < ARRAY_SIZE(FlashChipInfo); i++)
    {
        if(Offset < Size + FlashChipInfo[i].Size)
        {
            return FlashChipInfo[i].Start + (Offset - Size);
        }

        Size += FlashChipInfo[i].Size;
    }

    return 0;
}


/**
 * Find the flash address location to fit the given data block
 *
 * @param Offset Offset from the beginning in bytes
 * @param Size Number of bytes in the data
 *
 * @return Flash offset where this data can be fit
 */
static uint32 FI_FindSlotInFlash(uint32 Offset, uint32 Size)
{
    unsigned i;
    uint32 Total = 0;
	uint32 NextBound;
	uint32 ALIGN_Offset=ALIGN_BYTES_NEXT(Offset, 16);

    for(i = 0; i < ARRAY_SIZE(FlashChipInfo); i++)
	{
			NextBound = Total + FlashChipInfo[i].Size;

		if(ALIGN_Offset < NextBound)
        {
			if(ALIGN_Offset+Size < NextBound)
			{
				return ALIGN_Offset;
			}
			// This cannot fit within this flash chip. start from next flash chip start adress
			else
			{
                if(i + 1 >= ARRAY_SIZE(FlashChipInfo))
				{
					// end of flash memory.. we cannot fit
					return 0;
				}
				return NextBound;
			}	
#if 0
			//16Mb !< 15mb+5mb
			if(NextBound < ALIGN_Offset + Size)
			{
				if(i + 1 >= ARRAY_SIZE(FlashChipInfo_LargeFlash))
				{

					return 0;
				}

				// 0 = F9000000+1000000=FA000000, [i+1] FA000000, False
				// 1 = FA000000+1000000=FB000000, [i+1] F8000000, True
				// 2 = F8000000+1000000=F9000000  [i+1] ?		, True

                if(FlashChipInfo[i].Start + FlashChipInfo[i].Size !=
                        FlashChipInfo[i+1].Start)
                    return NextBound;
            }
            return ALIGN_BYTES_NEXT(Offset, 16);
#endif
        }
        Total = NextBound;
    }

    return 0;
}

/*************************** Global Functions *********************************/

/**
 * This function gets the flash image information
 *
 * @param FlashImage Flash image data structure loaded using
 *                   \FI_LoadFlashImage()
 * @param Info Flash image related information
 *
 * @return ERR_NULL_PTR,ERR_NOT_INITIALIZED, FAIL, SUCCESS
 */
int FI_GetFlashInfo(FI_FlashImage_t *FlashImage, FI_FlashInfo_t *Info)
{
    if(FlashImage == NULL || Info == NULL)
        THROW(ERR_NULL_PTR);

    if(FlashImage->FlashTableOffset == 0)
        THROW(ERR_NOT_INITIALIZED);

    Info->FlashTableOffset = FlashImage->FlashTableOffset;
    Info->TotalSize = FlashImage->TotalSize;
    Info->NumBlocks = FlashImage->FlashBlockCount;

    return SUCCESS;
}

/**
 * Updates the Flash start address and flash table offset location
 *
 * @param FlashImage Flash image data structure loaded using
 *                   \FI_LoadFlashImage()
 * @param Info Flash image information to be updated
 *
 * @return ERR_NULL_PTR, SUCCESS
 */
int FI_SetFlashInfo(FI_FlashImage_t *FlashImage, FI_FlashInfo_t const *Info)
{
    if(FlashImage == NULL || Info == NULL)
        THROW(ERR_NULL_PTR);

    FlashImage->FlashTableOffset = Info->FlashTableOffset;

    if(Info->NumBlocks < FlashImage->FlashBlockCount)
        FlashImage->FlashBlockCount = Info->NumBlocks;

    return SUCCESS;
}

/**
 * Gets one flash block related information
 *
 * @param FlashImage Flash image data structure loaded using
 *                   \FI_LoadFlashImage()
 * @param Index Index of the flash block  (for which the information is needed)
 * @Info Flash block information
 *
 * @return ERR_NOT_FOUND, ERR_NULL_PTR, SUCCESS
 *
 */
int FI_GetBlockInfo(FI_FlashImage_t *FlashImage, uint16 Index, 
													FI_BlockInfo_t *Info)
{

    if(FlashImage == NULL || Info == NULL)
        THROW(ERR_NULL_PTR);

	if(Index >= FlashImage->FlashBlockCount)
		THROW_MSG(ERR_NOT_FOUND, "Index Exceeded : %d[%d]", Index,
				FlashImage->FlashBlockCount);

    Info->Type = FlashImage->FlashBlockInfo[Index].Type;
    Info->FlashOffset = FlashImage->FlashBlockInfo[Index].FlashOffset;
    Info->FlashAddress = FI_FlashOffsetToAddress(Info->FlashOffset);
    Info->DataSize = FlashImage->FlashBlockInfo[Index].DataSize;
    Info->Index = FlashImage->FlashBlockInfo[Index].Index;

    return SUCCESS;
}

/**
 * Loads the flash image information to the internal data structure for
 * manipulation
 *
 * @param FileName Flash image file name
 *
 * @return Flash image data structure
 */
FI_FlashImage_t *FI_LoadFromFile(char const *FileName)
{
    FI_FlashImage_t *FlashImage = NULL;
    FILE *InpFile = NULL;
    uint32 TableSize;
    uint32 i;
    uint32 FlashTableAddr = BOOT_SECTOR_SIZE_MIN;
    uint32 FileSize;
	int Error;
	FI_FlashBlockInfo_t *BlockInfo;

    TRY_BEGIN()
    {
        FlashImage = calloc(1, sizeof(FI_FlashImage_t));
        if(FlashImage == NULL)
            TRY_THROW_MSG(ERR_OUT_OF_RESOURCE, "Not enough memory to load the file");

        InpFile = fopen(FileName, "rb");

        if(InpFile == NULL)
            TRY_THROW_MSG(FAIL, "Unable to open : %s", FileName);

        if(fseek(InpFile, 0, SEEK_END) < 0)
            TRY_THROW_MSG(FAIL, "Unable to read image file : %s", FileName);

        FileSize = ftell(InpFile);

        rewind(InpFile);

        while(TRUE)
        {
            /* Read flash table */
            if(FlashTableAddr >= FileSize ||
                    fseek(InpFile, FlashTableAddr, SEEK_SET) < 0)
                TRY_THROW_MSG(FAIL, "Invalid image file (Flash Table Missing) : %s",
                                                                     FileName);

            if(fread(&FlashTable, sizeof(FI_FlashTable_t), 1, InpFile) <= 0)
                TRY_THROW_MSG(FAIL, "Error while reading : %s", FileName);

            if(FlashTable.Signature == 0x1234567)
                break;

            FlashTableAddr *= 2;
        }

        if(FlashTable.FlashBlockCount > ARRAY_SIZE(FlashBlock))
            TRY_THROW_MSG(FAIL, "Number of flash blocks exceeded limit %d [%d]",
													FlashTable.FlashBlockCount,
													ARRAY_SIZE(FlashBlock));

        TableSize = sizeof(FI_FlashBlock_t) * (FlashTable.FlashBlockCount);

        if(fread(FlashBlock, TableSize, 1, InpFile) <= 0)
            TRY_THROW_MSG(FAIL, "Unable to read flash table : %s", FileName);

        if(strlen(FileName) >= sizeof(FlashImage->FlashBlockInfo[0].File))
            TRY_THROW_MSG(FAIL, "File Name Too long");

        FlashImage->FlashTableOffset = FlashTableAddr;
        FlashImage->TotalSize = FlashTableAddr + TableSize +
												sizeof(FI_FlashTable_t);
        FlashImage->FlashBlockCount = FlashTable.FlashBlockCount + 1;

		BlockInfo = &FlashImage->FlashBlockInfo[0];

        strcpy(BlockInfo->File, FileName);
        BlockInfo->DataOffset = 0;
        BlockInfo->FlashOffset = 0;
        BlockInfo->Data = NULL;
        BlockInfo->DataSize = FlashTableAddr;
        BlockInfo->Type = FI_BLOCK_BOOT_LOADER;

        for(i = 0; i < FlashTable.FlashBlockCount; i++)
        {
            uint32 EndOffset;
            FI_FlashBlockInfo_t *BlockInfo = &FlashImage->FlashBlockInfo[i + 1];
            strcpy(BlockInfo->File, FileName);
            BlockInfo->Data = NULL;

            BlockInfo->FlashOffset = FI_FlashAddressToOffset(FlashBlock[i].FlashAddress);

            BlockInfo->DataOffset = BlockInfo->FlashOffset;

            BlockInfo->DataSize = FlashBlock[i].ByteCount;

            BlockInfo->Type = FlashBlock[i].Type;

            BlockInfo->Index = FlashBlock[i].ID;
            BlockInfo->OrgIndex = FlashBlock[i].ID;

            EndOffset = BlockInfo->FlashOffset + FlashBlock[i].ByteCount;

            if(EndOffset > FileSize)
                TRY_THROW_MSG(FAIL, "Invalid Flash Image File");

            if(EndOffset > FlashImage->TotalSize)
            {
                FlashImage->TotalSize = EndOffset;
            }
            /* TODO : Add overlap check */
        }
    }
    TRY_END(Error);

	if(Error)
	{
		free(FlashImage);
		FlashImage = NULL;
	}

    if(InpFile)
        fclose(InpFile);


    return FlashImage;
}

/**
 * Free the flash image data structure previously loaded using
 * \FI_LoadFlashImage()
 *
 * @param FlashImage Flash image data structure
 *
 */
void FI_FreeFlashImage(FI_FlashImage_t *FlashImage)
{
    uint32 i;
    if(FlashImage == NULL)
        return;
    for(i = 0; i < FlashImage->FlashBlockCount; i++)
    {
        free(FlashImage->FlashBlockInfo[i].Data);
        FlashImage->FlashBlockInfo[i].Data = NULL;
    }
    free(FlashImage);
    FlashImage = NULL;
}


/**
 * Stores the given flash image to a file
 *
 * @param FlashImage Flash image data structure loaded using \FI_LoadFlashImage()
 * @param FileName Out put file name
 *
 * @return FAIL,ERR_NULL_PTR,SUCCESS
 */
int FI_SaveToFile(FI_FlashImage_t *FlashImage, char const *FileName)
{
    int Error;
    FILE *OutFile = NULL;
    uint32 i;
    int TableSize;

    TRY_BEGIN()
    {
        if(FlashImage == NULL || FileName == NULL)
            TRY_THROW(ERR_NULL_PTR);

        OutFile = fopen(FileName, "wb");

        if(OutFile == NULL)
            TRY_THROW_MSG(FAIL, "Unable to open %s", FileName);

        if(FI_WriteBlockToFile(&FlashImage->FlashBlockInfo[0], OutFile))
            TRY_THROW_MSG(FAIL, "File Write Failed %s", FileName);

        if(PadFile(OutFile, FlashImage->FlashTableOffset) < 0)
            TRY_THROW_MSG(FAIL, "File Write Failed %s", FileName);

        for(i = 1; i < FlashImage->FlashBlockCount; i++)
        {
            FlashBlock[i-1].Type = FlashImage->FlashBlockInfo[i].Type;
            FlashBlock[i-1].ID = FlashImage->FlashBlockInfo[i].Index;
            FlashBlock[i-1].Reserved1 = 0xFF;
            FlashBlock[i-1].Reserved2 = 0xFFFF;
            FlashBlock[i-1].FlashAddress = FI_FlashOffsetToAddress(FlashImage->
                                                FlashBlockInfo[i].FlashOffset);
            FlashBlock[i-1].ByteCount = FlashImage->FlashBlockInfo[i].DataSize;
        }

        FlashTable.Signature = 0x1234567;
        FlashTable.Boot_Address = FI_FlashOffsetToAddress(0);
        FlashTable.Version = 0x13;
        FlashTable.FlashBlockCount = FlashImage->FlashBlockCount-1;
        FlashTable.Free_Area_Start = ALIGN_BYTES_NEXT(FlashImage->TotalSize, 16);

        if(fwrite(&FlashTable, sizeof(FlashTable), 1, OutFile) <= 0)
            TRY_THROW_MSG(FAIL, "File Write Failed %s", FileName);

        TableSize = sizeof(FI_FlashBlock_t) * (FlashImage->FlashBlockCount);

        if(fwrite(FlashBlock, TableSize, 1, OutFile) <= 0)
            TRY_THROW_MSG(FAIL, "File Write Failed %s", FileName);

        for(i = 1; i < FlashImage->FlashBlockCount; i++)
        {
            if(FI_WriteBlockToFile(&FlashImage->FlashBlockInfo[i], OutFile))
                TRY_THROW_MSG(FAIL, "File Write Failed %s", FileName);
        }

		/* Align the end to 4 byte boundary */
		if(PadFile(OutFile, ALIGN_BYTES_NEXT(FlashImage->TotalSize, 4)) < 0)
			TRY_THROW_MSG(FAIL, "File Write Failed %s", FileName);
    }
    TRY_END(Error);

    if(OutFile)
        fclose(OutFile);

    return Error;
}

/**
 * Adds the given block data (From memory array) to the flash image data
 * structure
 *
 * @FlashImage Flash image data structure to be updated (Previously loaded using
 *             \FI_LoadFlashImage()
 * @Type Type of the flash image
 * @Data Pointer to the data to be added
 * @Size Data size in number of bytes
 *
 * @return ERR_OUT_OF_RESOURCE, SUCCESS
 */
int FI_AddBlock(FI_FlashImage_t *FlashImage, FI_BlockType_t Type,
													uint08 *Data, uint32 Size)
{
    FI_FlashBlockInfo_t *BlockInfo;
    uint32 TypeIndex = 0;
    uint32 i;
	uint32 FlashOffset;

    if(FlashImage->FlashBlockCount >= FI_MAX_FLASH_BLOCKS)
        THROW(ERR_OUT_OF_RESOURCE);

	FlashOffset = FI_FindSlotInFlash(FlashImage->TotalSize, Size);
	if(FlashOffset == 0)
		THROW_MSG(FAIL, "Unable to fit the data into flash");

    BlockInfo = &FlashImage->FlashBlockInfo[FlashImage->FlashBlockCount];

    if(BlockInfo->Data != NULL)
        free(BlockInfo->Data);


    BlockInfo->Data = malloc(Size);
    if(BlockInfo->Data == NULL)
        THROW_MSG(ERR_OUT_OF_RESOURCE,
                    "Memory allocation failed for the flash block");

    memcpy(BlockInfo->Data, Data, Size);

    for(i = 0; i < FlashImage->FlashBlockCount; i++)
    {
        if(FlashImage->FlashBlockInfo[i].Type == Type &&
                FlashImage->FlashBlockInfo[i].Index >= TypeIndex)
        {
            TypeIndex = FlashImage->FlashBlockInfo[i].Index + 1;
        }
    }

    BlockInfo->Type = Type;
    BlockInfo->Index = TypeIndex;
    BlockInfo->OrgIndex = FI_INVALID_INDEX;
    BlockInfo->DataOffset = 0;
    BlockInfo->DataSize = Size;
    BlockInfo->FlashOffset = FlashOffset;
    FlashImage->TotalSize = BlockInfo->FlashOffset + Size;

    FlashImage->FlashBlockCount ++;

    return FlashImage->FlashBlockCount - 1;
}

/**
 * Removes the given flash block from the flash image data structure
 *
 * @param FlashImage Flash image data structure loaded using \FI_LoadFlashImage()
 * @param Index Index of the flash block
 * @return ERR_NOT_FOUND/SUCCESS
 */
int FI_RemoveBlock(FI_FlashImage_t *FlashImage, uint32 Index)
{
    uint32 Addr;
    uint32 i;
    uint16 OrgID;
    uint32 OrgType;

    if(Index >= FlashImage->FlashBlockCount)
        THROW(ERR_NOT_FOUND);

    OrgID = FlashImage->FlashBlockInfo[Index].Index;
    OrgType = FlashImage->FlashBlockInfo[Index].Type;

    free(FlashImage->FlashBlockInfo[Index].Data);
    FlashImage->FlashBlockInfo[Index].Data = NULL;

    Addr = FlashImage->FlashBlockInfo[Index].FlashOffset;
    for(i = Index; i < FlashImage->FlashBlockCount - 1; i++)
    {
        FlashImage->FlashBlockInfo[i] = FlashImage->FlashBlockInfo[i+1];
        /* Dont change the application location */
        if(FlashImage->FlashBlockInfo[i].Type != FI_BLOCK_APP)
        {
            FlashImage->FlashBlockInfo[i].FlashOffset = 
					FI_FindSlotInFlash(Addr, 
										FlashImage->FlashBlockInfo[i].DataSize);
        }
		Addr = FlashImage->FlashBlockInfo[i].FlashOffset + 
						FlashImage->FlashBlockInfo[i].DataSize;
    }
    FlashImage->FlashBlockCount --;
    FlashImage->TotalSize = Addr;

    for(i = 0; i < FlashImage->FlashBlockCount; i++)
    {
        if(FlashImage->FlashBlockInfo[i].Type == OrgType &&
                FlashImage->FlashBlockInfo[i].Index > OrgID)
        {
            FlashImage->FlashBlockInfo[i].Index--;
        }
    }

    return SUCCESS;
}

/**
 * Dumps the given flash block to the given memory array
 * @param FlashImage Flash image data structure loaded using \FI_LoadFlashImage()
 * @param Index Index of the flash block
 * @param DataPtr Output memory data pointer
 *
 * @return On success number of bytes in data
 *			ERR_NULL_PTR/ERR_NOT_FOUND/ERR_OUT_OF_RESOURCE
 */
int FI_GetBlockData(FI_FlashImage_t *FlashImage, uint32 Index, uint08 **DataPtr)
{
	int Error;
    FI_FlashBlockInfo_t *BlockInfo = NULL;
    FILE *ImgFile = NULL;

    TRY_BEGIN()
    {
        if(FlashImage == NULL)
            TRY_THROW(ERR_NULL_PTR);

		if(Index >= FlashImage->FlashBlockCount)
			TRY_THROW_MSG(ERR_NOT_FOUND, "Index Exceeded : %d[%d]",
					Index, FlashImage->FlashBlockCount);


        BlockInfo =  &FlashImage->FlashBlockInfo[Index];

        if(BlockInfo->Data == NULL)
        {
            BlockInfo->Data = malloc(BlockInfo->DataSize);
            if(BlockInfo->Data == NULL)
                TRY_THROW_MSG(ERR_OUT_OF_RESOURCE, 
							"Unable to allocate memory for flash block");

            ImgFile = fopen(BlockInfo->File, "rb");
            if(ImgFile == NULL)
                TRY_THROW_MSG(FAIL,"Unable to open file : %s", 
															BlockInfo->File);
            if(BlockInfo->DataOffset)
            {
                if(fseek(ImgFile, BlockInfo->DataOffset, SEEK_SET) < 0)
                    TRY_THROW_MSG(FAIL, "Unable to read file : %s", 
															BlockInfo->File);
            }
            if(fread(BlockInfo->Data, BlockInfo->DataSize, 1, ImgFile) <= 0)
                TRY_THROW_MSG(FAIL, "File read failed : %s", 
															BlockInfo->File);
        }
    }
    TRY_END(Error);

    if(ImgFile)
    {
        fclose(ImgFile);
    }

    if(Error && BlockInfo != NULL)
    {
        free(BlockInfo->Data);
        BlockInfo->Data = NULL;
    }

	*DataPtr = BlockInfo->Data;

    return BlockInfo->DataSize;
}

/**
 * Set new data for the given flash block
 *
 * @param FlashImage Flash image data structure loaded using \FI_LoadFlashImage()
 * @param Index Index of the flash block
 * @param Data Pointer to the data to be set
 * @param Size Number of bytes in the data
 *
 * @return On success number of bytes in data
 *			ERR_NULL_PTR/ERR_NOT_FOUND/ERR_OUT_OF_RESOURCE
 */
int FI_SetBlockData(FI_FlashImage_t *FlashImage, uint32 Index, uint08 *Data,
																   uint32 Size)
{
    FI_FlashBlockInfo_t *BlockInfo = NULL;

	if(FlashImage == NULL)
		THROW(ERR_NULL_PTR);

	if(Index >= FlashImage->FlashBlockCount)
		THROW_MSG(ERR_NOT_FOUND, "Index Exceeded : %d[%d]", 
						Index, FlashImage->FlashBlockCount);

	BlockInfo =  &FlashImage->FlashBlockInfo[Index];

	if(BlockInfo->Data != NULL || BlockInfo->DataSize < Size)
	{
		free(BlockInfo->Data);
		BlockInfo->Data = NULL;
	}

	BlockInfo->Data = malloc(Size);
	if(BlockInfo->Data == NULL)
		THROW_MSG(ERR_OUT_OF_RESOURCE, 
					"Unable to allocate memory for flash block");

	memcpy(BlockInfo->Data, Data, Size);

	BlockInfo->DataSize = Size;

    return SUCCESS;
}

/**
 * Dumps the given flash block to a file
 * @param FlashImage Flash image data structure loaded using \FI_LoadFlashImage()
 * @param Type Type of the flash block
 * @param Index Index of the flash block
 * @param FileName Output file name
 *
 * @return ERR_NOT_FOUND/SUCCESS
 */
int FI_SaveBlockDataToFile(FI_FlashImage_t *FlashImage, uint32 Index, 
															char const *FileName)
{
    int Error;
    FI_FlashBlockInfo_t *BlockInfo = NULL;
    FILE *ImgFile = NULL;
    FILE *DumpFile = NULL;

    TRY_BEGIN()
    {
        if(FlashImage == NULL || FileName == NULL)
            TRY_THROW(ERR_NULL_PTR);

		if(Index >= FlashImage->FlashBlockCount)
			TRY_THROW_MSG(FAIL, "Index Exceeded : %d[%d]", Index,
												FlashImage->FlashBlockCount);


        BlockInfo =  &FlashImage->FlashBlockInfo[Index];

        DumpFile = fopen(FileName, "wb");
        if(DumpFile == NULL)
            TRY_THROW_MSG(FAIL, "Unable to create file : %s", FileName);

        if(BlockInfo->Data)
        {
            if(fwrite(BlockInfo->Data, BlockInfo->DataSize, 1, DumpFile) < 1)
                TRY_THROW_MSG(FAIL, "Unable to write to file : %s",
                                                                    FileName);
        }
        else
        {
            Error = OpenCopyFile(BlockInfo->File, BlockInfo->DataOffset,
                                            DumpFile, BlockInfo->DataSize);
        }
    }
    TRY_END(Error);

    if(DumpFile)
        fclose(ImgFile);

    return Error;
}

/**
 * Mapping of original and new index for the given block type
 *
 * @param FlashImage Flash image data structure loaded using \FI_LoadFlashImage()
 * @param Type Type of the flash block
 * @param [out] Map Index mapping 
 *
 * @return Number of entries in the Map
 */
int FI_GetNewBlockIndex(FI_FlashImage_t *FlashImage, FI_BlockType_t Type,
															int OrgIndex)
{
    uint16 i;

	for(i = 0; i < FlashImage->FlashBlockCount; i++)
	{
		if(FlashImage->FlashBlockInfo[i].Type == Type && 
				OrgIndex == FlashImage->FlashBlockInfo[i].OrgIndex)
		{
			OrgIndex = FlashImage->FlashBlockInfo[i].Index;
			break;
		}
	}

	return OrgIndex;
}

