/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/

/**
 * @brief This module handles Flash programing related functionality
 *
 * @note:
 *
 */
#include <inttypes.h>
#include <Windows.h>
#include "API.h"
#include "error.h"
#include "flashloader.h"

/******************************** MACROS **************************************/
#define FL_NUM_CHIPS			16 //16*16MB=256MB Support
#define FL_DATA_CHUNK_SIZE		API_MAX_PARAM_SIZE

/***************************** LOCAL TYPES ************************************/
typedef struct
{
    char     ManName[32];   /* Company name. */
    uint16   ManID;         /* Manufacturer ID stored in part. */
    uint64   ManIDLong;     /* Long Manufacturer ID stored in part. */
    char     DevName[32];   /* Part number from data sheet. */
    uint16   DevID;         /* Device ID stored in part. */
    uint64   DevIDLong;     /* Long Device ID stored in part. */
    uint16   SizeMBs;       /* 4, 8, or 16 MBit. */
    uint08   Type;          /* A, B or C algorithm (0, 1, 2) */
    uint32   NumSectors;    /* Number of valid sectors */
    uint32   SectorAddress[512];  /* List of sector addresses. */
} FL_FlashDeviceInfo_t;

/** Flash chip information */
typedef struct
{
    uint32 Start; /**< Start address off the flash chip */
    uint32 Size;  /**< Capacity of the flash chip in bytes */
} FL_FlashChipInfo_t;

/** Order of information present in the flash parameter file */
enum
{
	FL_PARAM_MAN_NAME,
	FL_PARAM_MAN_ID,
	FL_PARAM_MAN_ID_LONG,
	FL_PARAM_DEV_NAME,
	FL_PARAM_DEV_ID,
	FL_PARAM_DEV_ID_LONG,
	FL_PARAM_SIZE_MB,
	FL_PARAM_TYPE,
};

/****************************** VARIABLES *************************************/

/** Data structure to store flash device information for all the chips */
static FL_FlashDeviceInfo_t FlashInfo[FL_NUM_CHIPS];

/** Scratchpad memory for reading one line of data */
static uint08 Buffer[1024*24];

/** Number of chips in the EVM */
static int FL_NumChips;

/** Total flash size */
static uint32 FL_AvailableFlashSize;

/** Bootloader version is Legacy */
static BOOL LegacyBootloader;

/** Callback function */
static FL_Callback_t *FL_Callback;

/** Parameter for the callback function */
static void *FL_CallbackParam;

/** Information of all the flash chips in the system */
static const uint32 FlashCSOrder[FL_NUM_CHIPS] =
{
    0xF9000000UL,
    0xFA000000UL,
    0xF8000000UL,
	0x03000000UL,
	0x04000000UL,
	0x05000000UL,
	0x06000000UL,
	0x07000000UL,
	0x08000000UL,
	0x09000000UL,
	0x0A000000UL,
	0x0B000000UL,
	0x0C000000UL,
	0x0D000000UL,
	0x0E000000UL,
	0x0F000000UL
};
#define FL_FLASH_SIZE_128MB	128
#define FL_FLASH_SIZE_256MB	256

#define FL_CS1_ARRAY_INDEX	0
/** Large Flash chip information */
typedef struct
{
	uint16 FlashSizeMB;  /**< Capacity of the flash chip in Mega Byte*/
    uint64 LongDevID; /**< Long Device ID as mentioned in Datasheet */
	char *PartNumber;
} FL_LargeFlashChipInfo_t;

/*Do not include Less than 128MB Flash chip info here */
static const FL_LargeFlashChipInfo_t FL_LargeFlashChipInfo[] =
{
	{FL_FLASH_SIZE_128MB,	0x22012228227EULL,	"S29GL01GS"},
	{FL_FLASH_SIZE_256MB,	0x22012248227EULL,	"S70GL02GS"}
};


/********************** LOCAL FUNCTION PROTOTYPES *****************************/
static char *FL_GetToken(char **Str);
static int FL_ParseFlashInfo(char *Line, int *VersionSet);
static int FL_SetFlashAddr(uint32 Address);
static int FL_SetDownloadSize(uint32 Size);
static int FL_GetFlashAddr(uint32 Offset, uint32 *Address, uint32 *RemaingSize,
																uint08 *Type);
static int FL_GetSectorInfo(uint32 Offset, uint32 *SectStart, uint32 *SectSize, 
																uint08 *Type);
/************************* FUNCTION DEFINITIONS********************************/

/**
 * Fill the flash device info structure from the flash parameter file 
 *
 * @param FlashParamFile Flash device parameter text file
 *
 * @return SUCCESS/FAIL/ERR_NOT_FOUND
 */
int FL_UpdateFlashInfo(char const *FlashParamFile)
{
    int ChipNum=0;
	FILE *FlpFile = NULL;
	int VersionSet = 0;
	int Error;
	int Found = 0;
    uint16 i;
	
	uint16 ManID;
	uint64 DevID;

	FL_AvailableFlashSize = 0;
	FL_NumChips = 0;

	TRY_BEGIN()
	{
		for(ChipNum = 0; ChipNum < FL_NUM_CHIPS; ChipNum++)
		{
			if(ChipNum==3)
			{
				//For backward compatablity.
				break;
			}
			STRUCT_CLEAR(FlashInfo[ChipNum]);

			if(FL_SetFlashAddr(FlashCSOrder[ChipNum]) < 0)
			{
				continue;
			}

			if(LCR_GetFlashManID(&ManID) < 0)
			{
				continue;
			}

			if(LCR_GetFlashDevID(&DevID) < 0)
			{
				continue;
			}

			FlashInfo[ChipNum].ManID = ManID;
			FlashInfo[ChipNum].DevIDLong = DevID;
			FlashInfo[ChipNum].DevID = DevID;
			FL_NumChips ++;
		}
		for(i=0; i < ARRAY_SIZE(FL_LargeFlashChipInfo); i++)
		{
			if(FlashInfo[FL_CS1_ARRAY_INDEX].DevIDLong == FL_LargeFlashChipInfo[i].LongDevID)
			{
                switch(FL_LargeFlashChipInfo[i].FlashSizeMB)
				{
					case FL_FLASH_SIZE_128MB:
						FL_NumChips=8; // 16MB*8=128MB
						break;
						
					case FL_FLASH_SIZE_256MB:
						FL_NumChips=16; // 16MB*16=256MB
						break;
						
					default: break;
				}
				
				for(; ChipNum<FL_NumChips; ChipNum++)
				{
					FlashInfo[ChipNum].ManID = FlashInfo[FL_CS1_ARRAY_INDEX].ManID;
					FlashInfo[ChipNum].DevID = FlashInfo[FL_CS1_ARRAY_INDEX].DevID;
				}
				break;
			}
		}

		if(FL_NumChips == 0)
		{
			TRY_THROW_MSG(FAIL, "Unable to read flash device IDs");
		}

		FlpFile = fopen(FlashParamFile, "r");
		if(FlpFile == NULL)
			TRY_THROW_MSG(FAIL, "Unable to open file : %s", FlashParamFile);

		while(Found < FL_NumChips)
		{
			int Count;

			if(fgets((char *)Buffer, sizeof(Buffer), FlpFile) == NULL)
				TRY_THROW_MSG(ERR_NOT_FOUND, "Unknown Flash IDs : %x %x", 
							FlashInfo[Found].ManID, FlashInfo[Found].DevID);

			TRY_TRY(Count = FL_ParseFlashInfo((char *)Buffer, &VersionSet));

			Found += Count;
		}
	}
	TRY_END(Error);

	if(FlpFile != NULL)
		fclose(FlpFile);

	return Error;
}

/**
 * Erase the flash sectors in the given offset range
 *
 * @param Offset Byte offset from the beginning of flash
 * @param Size Number of byte to erase
 *
 * @return  SUCCESS/FAIL
 */
int FL_EraseFlashRange(uint32 Offset, uint32 Size)
{
	uint32 Begin = Offset;
	uint32 End = Offset + Size;
	uint32 Address = 0;
	uint32 SectSize;
	uint08 Type;

	while(Offset < End)
	{

		TRY(FL_GetSectorInfo(Offset, &Address, &SectSize, &Type));

		TRY(FL_SetFlashAddr(Address));

		if(FL_Callback)
		{
            uint08 Percent = (uint08)(((Offset - Begin)/(float)Size)*100);
			if(FL_Callback(FL_CallbackParam, FL_OPER_ERASE, Address, Percent))
			{
				THROW_MSG(ERR_ABORT, "Flash Erase aborted at %X", Address);
			}
		}

		if(LCR_SetFlashType(Type) < 0)
			THROW_MSG(FAIL, "Unable to set flash type");

		if(LCR_FlashSectorErase() < 0)
			THROW_MSG(FAIL, "Flash sector erase failed at : 0x%X", Address);

		LCR_WaitForFlashReady();

		Offset += SectSize;
	}

	if(FL_Callback)
	{
        if(FL_Callback(FL_CallbackParam, FL_OPER_ERASE, Address, 100))
		{
			THROW_MSG(ERR_ABORT, "Flash Erase aborted at %X", Address);
		}
	}

	return SUCCESS;
}

/**
 * Program the flash signature.
 *
 * @param Offset Offset to start from
 *
 * @return SUCCESS/FAIL
 */
BOOL FL_ProgramSignature(uint32 Offset, BOOL Program)
{
    uint32 DummySize;
    uint08 DummyType;
    uint32 StartChipAddress = 0;
    int Error;
    TRY_BEGIN()
    {
        TRY_TRY(FL_GetFlashAddr(Offset, &StartChipAddress, &DummySize, &DummyType));
        uint8 signature[4] = {0x67, 0x45, 0x23, 0x01};
        if (!Program)
        {
            signature[0] = 0;
            signature[1] = 0;
            signature[2] = 0;
            signature[3] = 0;
        }

        TRY_TRY(FL_SetFlashAddr(StartChipAddress));
        TRY_TRY(FL_SetDownloadSize(4));

        if(LCR_DownloadData(signature, 4) < 4)
            TRY_THROW_MSG(FAIL, "Signature programming failed");
    }
    TRY_END(Error);
    return Error;
}


/**
 * Program the flash with the image file content. It also erases the area
 * before programming.
 *
 * @param FileName Name of the flash image file
 * @param Offset Offset to start from
 * @param Size Numbe of bytes to program. If 0, then till end of file
 * @param CompWithCache Whether to compare existing content with cache file
 * @param FileNameCached Name of the cached file for comparison
 * @param IsFlashContChanged flag set to TRUE or FALSE based on flash content changed
 * @param ProgramSignature Flag to indicate flash signature need to be programmed or not at the end of download
 *
 * @return SUCCESS/FAIL
 */
int FL_ProgramFlash(char const *FileName, uint32 Offset, uint32 Size, BOOL CompWithCache, char const *FileNameCached,
                    BOOL *IsFlashContChanged, BOOL ProgramSignature)
{
    int Error;
    BOOL flashUpdated = FALSE;
    FILE *FlashFile = NULL;
    FILE *FlashFileCached = NULL;
	uint32 Begin = Offset;
	uint32 RemingSize;
	uint32 ChipAddress;
    uint32 OriginalOffset = Offset;
    uint32 StartChipAddress = 0;

	TRY_BEGIN()
	{
		if((FlashFile = fopen(FileName, "rb")) == NULL)
			TRY_THROW_MSG(FAIL, "Unable to open firmware file : %s", FileName);

		if(Size == 0)
		{
			long End;
			if(fseek(FlashFile, 0, SEEK_END) < 0)
				TRY_THROW_MSG(FAIL, "File read failed : %s", FileName);
			if((End = ftell(FlashFile)) < 0)
				TRY_THROW_MSG(FAIL, "File read failed : %s", FileName);

			Size = End - Offset;
		}

		if(Offset + Size > FL_AvailableFlashSize)
			TRY_THROW_MSG(FAIL, "Program size exceeds flash : %d > %d", 
										Offset + Size, FL_AvailableFlashSize);

		if(fseek(FlashFile, Offset, SEEK_SET) < 0)
			TRY_THROW_MSG(FAIL, "File size is too small : %s", FileName);

        //Check cached file if it is available
        if(CompWithCache == TRUE)
        {
            if((FlashFileCached = fopen(FileNameCached, "rb")) == NULL)
                TRY_THROW_MSG(FAIL, "Unable to open firmware file : %s", FileNameCached);

            uint32 tempOffset = Offset;
            RemingSize = Size;
            uint32 firstIterationOffset = 0;

            if(OriginalOffset != 0)
            {
                uint32 DummySize;
                uint08 DummyType;
                TRY_TRY(FL_GetFlashAddr(OriginalOffset, &StartChipAddress, &DummySize, &DummyType));
            }

            //Erase the mismatch areas in the flash and program the same
            while(RemingSize)
            {
                uint32 SectStart;
                uint32 SectSize;
                uint08 Type;
                uint08 *FlashFileBuf;
                uint08 *CachedFlashFileBuf;


                TRY(FL_GetSectorInfo(tempOffset,&SectStart,&SectSize,&Type));
                if((tempOffset+SectSize)>= (Offset+Size))
                    SectSize = (Offset+Size) - tempOffset;

                if(fseek(FlashFile, tempOffset, SEEK_SET) < 0)
                    TRY_THROW_MSG(FAIL, "File size is too small : %s", FileName);

                //Don't care the size of the cache file
                fseek(FlashFileCached, tempOffset, SEEK_SET);

                //Request 1KB buffer for readout
                FlashFileBuf = malloc(1024);
                CachedFlashFileBuf = malloc(1024);
                if((FlashFileBuf == NULL) || (CachedFlashFileBuf == NULL))
                    TRY_THROW_MSG(FAIL, "Unable to allocate memory");

                //Check for differences in the sectSize area in the
                //two flash binary files from the tempOffSet location
                uint32 bytesCompared = SectSize;
                BOOL programSector = FALSE;
                while(bytesCompared)
                {
                    if((OriginalOffset != 0) && (OriginalOffset == tempOffset))
                    {
                        programSector = TRUE;
                        break;
                    }
                    int tempVarA = fread(FlashFileBuf, 1, 1024, FlashFile);
                    int tempVarB = fread(CachedFlashFileBuf, 1, 1024, FlashFileCached);

                    //Reached end of file
                    if(tempVarA < 0)
                        break;

                    if(tempVarA <= tempVarB)
                    {
                        if(memcmp(FlashFileBuf,CachedFlashFileBuf,tempVarA)!=0)
                        {
                            programSector = TRUE;
                            break;
                        }
                        else
                        {
                            bytesCompared -= tempVarA;
                        }
                    }
                    else
                    {
                        //Anyways tempVarA exceeds the cache file we must erase and program the flash
                        programSector = TRUE;
                        break;
                    }
                }

                if(programSector)
                {
                    flashUpdated = TRUE;

                    //Erase mismatch sector @ tempOffset
                    TRY_TRY(FL_EraseFlashRange(tempOffset, SectSize));

                    uint32 WriteSize;
                    uint32 ChipSize;
                    uint32 ChipAddress;
                    uint08 Type;

                    //Set the sector address and download size it should be <= sector size
                    TRY_TRY(FL_GetFlashAddr(tempOffset, &ChipAddress, &ChipSize, &Type));

                    if((OriginalOffset != 0) && (OriginalOffset == tempOffset))
                    {
                        tempOffset += 4;
                        ChipAddress += 4;
                        SectSize -= 4;
                        firstIterationOffset = 4;
                    }
                    else
                    {
                        firstIterationOffset = 0;
                    }

                    if(fseek(FlashFile, tempOffset, SEEK_SET) < 0)
                        TRY_THROW_MSG(FAIL, "File size is too small : %s", FileName);

                    TRY_TRY(FL_SetFlashAddr(ChipAddress));
                    TRY_TRY(FL_SetDownloadSize(SectSize));

                    if(LCR_SetFlashType(Type) < 0)
                        THROW_MSG(FAIL, "Unable to set flash type");

                    WriteSize = SectSize;
                    Begin = tempOffset;
                    while(WriteSize > 0)
                    {
                        int Chunk = WriteSize;
                        if(Chunk > FL_DATA_CHUNK_SIZE)
                            Chunk = FL_DATA_CHUNK_SIZE;

                        if(fread(Buffer, 1, Chunk, FlashFile) < (unsigned)Chunk)
                            TRY_THROW_MSG(FAIL, "Unable to read file : %s", FileName);

                        if(FL_Callback)
                        {
                            uint08 Percent = (uint08)((tempOffset - Begin)/(float)SectSize * 100);
                            uint32 Address;
                            uint32 RemSize;
                            uint08 ChipType;
                            FL_GetFlashAddr(tempOffset, &Address, &RemSize, &ChipType);

                            if(FL_Callback(FL_CallbackParam, FL_OPER_PROGRAM,
                                           Address, Percent))
                            {
                                THROW_MSG(ERR_ABORT, "Flash Programming aborted at %X",
                                          Address);
                            }
                        }

                        if(LCR_DownloadData(Buffer, Chunk) < Chunk)
                            TRY_THROW_MSG(FAIL, "Flash programing failed at : 0x%X",
                                          ChipAddress + tempOffset);

                        WriteSize -= Chunk;
                        tempOffset += Chunk;
                    }


                    if(FL_Callback)
                    {
                        uint32 Address;
                        uint32 RemSize;
                        uint08 ChipType;
                        FL_GetFlashAddr(tempOffset, &Address, &RemSize, &ChipType);
                        Address = ChipAddress + tempOffset;
                        if(FL_Callback(FL_CallbackParam, FL_OPER_PROGRAM, Address, 100))
                        {
                            THROW_MSG(ERR_ABORT, "Flash Programming aborted at %X", Address);
                        }
                    }
                    SectSize += firstIterationOffset;

                }
                else
                {
                    tempOffset += SectSize;
                }

                free(FlashFileBuf);
                free(CachedFlashFileBuf);
                FlashFileBuf = NULL;
                CachedFlashFileBuf = NULL;

                RemingSize -= SectSize;
            }

            if ((OriginalOffset != 0) && (ProgramSignature == TRUE))
            {
                FL_ProgramSignature(OriginalOffset, TRUE);
            }
        }
        else
        {
            flashUpdated = TRUE;

            TRY_TRY(FL_EraseFlashRange(Offset, Size));

            RemingSize = Size;
            BOOL firstIteration = FALSE;
            uint32 firstIterationOffset = 0;
            while(RemingSize)
            {
                uint32 ChipSize;
                uint32 WriteSize;
                uint32 LocalChecksum = 0;
                uint32 ReceivedChecksum = 0;
                uint08 Type;

                LCR_WaitForFlashReady();

                TRY_TRY(FL_GetFlashAddr(Offset, &ChipAddress, &ChipSize, &Type));
                if(ChipSize > RemingSize)
                    ChipSize = RemingSize;

                if((OriginalOffset != 0) && (firstIteration == FALSE))
                {
                    firstIteration = TRUE;
                    firstIterationOffset = 4;
                    StartChipAddress = ChipAddress;
                    if(fseek(FlashFile, Offset + 4, SEEK_SET) < 0)
                        TRY_THROW_MSG(FAIL, "File size is too small : %s", FileName);
                    ChipAddress = ChipAddress + 4;
                    ChipSize = ChipSize - 4;
                    TRY_TRY(FL_SetFlashAddr(ChipAddress));
                    TRY_TRY(FL_SetDownloadSize(ChipSize));
                }
                else
                {
                    firstIterationOffset = 0;
                    TRY_TRY(FL_SetFlashAddr(ChipAddress));
                    TRY_TRY(FL_SetDownloadSize(ChipSize));
                }

                if(LCR_SetFlashType(Type) < 0)
                    THROW_MSG(FAIL, "Unable to set flash type");

                WriteSize = ChipSize;
                while(WriteSize > 0)
                {
                    int i;
                    int Chunk = WriteSize;
                    if(Chunk > FL_DATA_CHUNK_SIZE)
                        Chunk = FL_DATA_CHUNK_SIZE;

                    if(fread(Buffer, 1, Chunk, FlashFile) < (unsigned)Chunk)
                        TRY_THROW_MSG(FAIL, "Unable to read file : %s", FileName);

                    if(FL_Callback)
                    {
                        uint08 Percent = (uint08)((Offset - Begin)/(float)Size * 100);
                        uint32 Address;
                        uint32 RemSize;
                        uint08 ChipType;
                        FL_GetFlashAddr(Offset, &Address, &RemSize, &ChipType);
                        if(FL_Callback(FL_CallbackParam, FL_OPER_PROGRAM,
                                                            Address, Percent))
                        {
                            THROW_MSG(ERR_ABORT, "Flash Programming aborted at %X",
                                                                            Address);
                        }
                    }

                    if(LCR_DownloadData(Buffer, Chunk) < Chunk)
                        TRY_THROW_MSG(FAIL, "Flash programing failed at : 0x%X",
                                                            ChipAddress + Offset);

                    /* Compute checksum for written number of bytes */
                    for(i = 0; i < Chunk; i++)
                    {
                        LocalChecksum += Buffer[i];
                    }

                    WriteSize -= Chunk;
                    Offset += Chunk;
                }

                LCR_WaitForFlashReady();

                TRY_TRY(FL_SetFlashAddr(ChipAddress));
                TRY_TRY(FL_SetDownloadSize(ChipSize));

                if(LCR_CalculateFlashChecksum() < 0)
                    TRY_THROW_MSG(FAIL, "Checksum read failed at 0x%X", ChipAddress);

                LCR_WaitForFlashReady();

                if(LCR_GetFlashChecksum(&ReceivedChecksum) < 0)
                    TRY_THROW_MSG(FAIL, "Checksum read failed at 0x%X", ChipAddress);

                if(ReceivedChecksum != LocalChecksum)
                    TRY_THROW_MSG(FAIL, "Flash checksum mismatch : 0x%X != 0x%X",
                                            LocalChecksum, ReceivedChecksum);

                ChipAddress -= firstIterationOffset;
                ChipSize += firstIterationOffset;
                Offset += firstIterationOffset;
                RemingSize -= ChipSize;
            }

            if(FL_Callback)
            {
                uint32 Address = ChipAddress + Offset;
                if(FL_Callback(FL_CallbackParam, FL_OPER_PROGRAM, Address, 100))
                {
                    THROW_MSG(ERR_ABORT, "Flash Programming aborted at %X", Address);
                }
            }

            if ((OriginalOffset != 0) && (ProgramSignature == TRUE))
            {
                FL_ProgramSignature(OriginalOffset, TRUE);
            }

        }
    }
	TRY_END(Error);

	if(FlashFile != NULL)
	{
		fclose(FlashFile);
	}

    if(FlashFileCached != NULL)
    {
        fclose(FlashFileCached);
    }

    (flashUpdated == TRUE)?(*IsFlashContChanged = TRUE):(*IsFlashContChanged = FALSE);

    return Error;
}

/**
 * Check the flash with the image file content for matching
 *
 * @param FileName Name of the flash image file whose content will be chcked for matching
 * @param Offset Offset to start from
 * @param Size Numbe of bytes to compare. If 0, then till end of file
 *
 * @return SUCCESS/FAIL
 */
int FL_CheckFlash(char const *FileName, uint32 Offset, uint32 Size)
{
    FILE *FlashFile = NULL;
    int Error;
    uint32 RemingSize;
    uint32 ChipAddress;
    uint32 Begin = Offset;

    TRY_BEGIN()
    {
        if((FlashFile = fopen(FileName, "rb")) == NULL)
            TRY_THROW_MSG(FAIL, "Unable to open firmware file : %s", FileName);

        if(Size == 0)
        {
            long End;
            if(fseek(FlashFile, 0, SEEK_END) < 0)
                TRY_THROW_MSG(FAIL, "File read failed : %s", FileName);
            if((End = ftell(FlashFile)) < 0)
                TRY_THROW_MSG(FAIL, "File read failed : %s", FileName);

            Size = End - Offset;
        }

        if(Offset + Size > FL_AvailableFlashSize)
            TRY_THROW_MSG(FAIL, "Program size exceeds flash : %d > %d",
                                        Offset + Size, FL_AvailableFlashSize);

        if(fseek(FlashFile, Offset, SEEK_SET) < 0)
            TRY_THROW_MSG(FAIL, "File size is too small : %s", FileName);

        RemingSize = Size;
        while(RemingSize)
        {
            uint32 ChipSize;
            uint32 WriteSize;
            uint32 LocalChecksum = 0;
            uint32 ReceivedChecksum = 0;
            uint08 Type;

            LCR_WaitForFlashReady();

            TRY_TRY(FL_GetFlashAddr(Offset, &ChipAddress, &ChipSize, &Type));
            if(ChipSize > RemingSize)
                ChipSize = RemingSize;

            /*
            TRY_TRY(FL_SetFlashAddr(ChipAddress));
            TRY_TRY(FL_SetDownloadSize(ChipSize));

            if(LCR_SetFlashType(Type) < 0)
                THROW_MSG(FAIL, "Unable to set flash type");
            */

            WriteSize = ChipSize;
            while(WriteSize > 0)
            {
                int i;
                int Chunk = WriteSize;
                if(Chunk > FL_DATA_CHUNK_SIZE)
                    Chunk = FL_DATA_CHUNK_SIZE;

                if(fread(Buffer, 1, Chunk, FlashFile) < (unsigned)Chunk)
                    TRY_THROW_MSG(FAIL, "Unable to read file : %s", FileName);

                if(FL_Callback)
                {
                    uint08 Percent = (uint08)((Offset - Begin)/(float)Size * 100);
                    uint32 Address;
                    uint32 RemSize;
                    uint08 ChipType;
                    FL_GetFlashAddr(Offset, &Address, &RemSize, &ChipType);
                    if(FL_Callback(FL_CallbackParam, FL_OPER_VERIFY,
                                                        Address, Percent))
                    {
                        THROW_MSG(ERR_ABORT, "Flash Programming aborted at %X",
                                                                        Address);
                    }
                }

                /* Compute checksum for written number of bytes */
                for(i = 0; i < Chunk; i++)
                {
                    LocalChecksum += Buffer[i];
                }

                WriteSize -= Chunk;
                Offset += Chunk;
            }

            LCR_WaitForFlashReady();

            TRY_TRY(FL_SetFlashAddr(ChipAddress));
            TRY_TRY(FL_SetDownloadSize(ChipSize));

            if(LCR_CalculateFlashChecksum() < 0)
                TRY_THROW_MSG(FAIL, "Checksum read failed at 0x%X", ChipAddress);

            LCR_WaitForFlashReady();

            if(LCR_GetFlashChecksum(&ReceivedChecksum) < 0)
                TRY_THROW_MSG(FAIL, "Checksum read failed at 0x%X", ChipAddress);

            if(ReceivedChecksum != LocalChecksum)
                TRY_THROW_MSG(FAIL, "Flash checksum mismatch : 0x%X != 0x%X",
                                        LocalChecksum, ReceivedChecksum);

            RemingSize -= ChipSize;
        }

        if(FL_Callback)
        {
            uint32 Address = ChipAddress + Offset;
            if(FL_Callback(FL_CallbackParam, FL_OPER_VERIFY, Address, 100))
            {
                THROW_MSG(ERR_ABORT, "Flash Programming aborted at %X", Address);
            }
        }

    }
    TRY_END(Error);

    if(FlashFile != NULL)
    {
        fclose(FlashFile);
    }

    return Error;
}


/**
 * Setup the callback function for flash operation. The callback will
 * be called periodically duirng flash erase / program operation
 *
 * @param Callback
 * @param Param
 *
 * @return 
 */
int FL_SetCallback(FL_Callback_t *Callback, void *Param)
{
	FL_Callback = Callback;
	FL_CallbackParam = Param;

	return SUCCESS;
}

/**
 * Enables or disables old bootloader commands only. To support legacy
 * bootloders
 * @param Enable TRUE - Use only old commands, FALSE - Use old and new commands
 *
 * @return SUCCESS;
 */
int FL_UseLegacyCommands(BOOL Enable)
{
    LegacyBootloader = Enable;
    return SUCCESS;
}

/****************************** LOCAL FUNCTIONS *******************************/
/**
 * Sets the flash address to the EVM based on the bootloader version
 * @param Address Address to be set
 *
 * @return SUCCESS/FAIL
 */
static int FL_SetFlashAddr(uint32 Address)
{
    int Result;
    if(LegacyBootloader)
    {
        Result = LCR_SetFlashAddr(Address);
    }
    else
    {
        Result = LCR_SetFlashAddr4Byte(Address);
    }

	if(Result < 0)
		THROW_MSG(FAIL, "Unable to set flash address for programing : 0x%X", 
																	Address);
    return SUCCESS;
}

/**
 * Set the download size for programing flash depending on the bootloader
 * version
 *
 * @param Size Number of bytes to be downloaded
 *
 * @return SUCCESS/FAIL
 */
static int FL_SetDownloadSize(uint32 Size)
{
	int Result;

	if(LegacyBootloader)
	{
		Result = LCR_SetDownloadSize(Size);
	}
	else
	{
		Result = LCR_SetDownloadSize4Byte(Size);
	}

	if(Result < 0)
		THROW_MSG(FAIL, "Unable to set download size : %d", Size);

	return Result;
}

/**
 * Parse the list of integer from the string to array
 *
 * @param Str String to be parsed
 * @param [out] Array Array to be populated
 * @param MaxCount Max number of elements in the array
 *
 * @return Number of elements populated in the array
 */
static int FL_ParseIntArray(char *Str, uint32 *Array, uint32 MaxCount)
{
    uint32 Count;
	uint32 PrevValue = 0;

	for(Count = 0; Count < MaxCount; Count++)
	{
		uint32 Value;
		char *Token = FL_GetToken(&Str);
		char *End;

		if(Token == NULL)
			break;

		Value = strtol(Token, &End, 0);
		if(*End != 0)
			THROW_MSG(ERR_FORMAT_ERROR, 
					"Invalid number in flash parameter file : %s", Token);

		if(Value < PrevValue)
			break;

		Array[Count] = Value;
		PrevValue = Value;
	}
	return Count;
}

/**
 * Parses a line of flash info and updates the data structure
 *
 * @param Line Line string to be parsed
 * @param VersionSet Version information read ?
 *
 * @return ERR_FORMAT_ERROR/FAIL/Number of devices updated
 */
static int FL_ParseFlashInfo(char *Line, int *VersionSet)
{
	int i;
	int Found = 0;
	int Chip;
	char *Ptr = Line;
	char *Info[10];
	char *End;
	int MatchIndex = -1;
	uint16 ManID;
	uint16 DevID;

	for(i = 0; i < 10; i++) 
	{
		Info[i] = FL_GetToken(&Ptr);
		if(Info[i] == 0)
			break;
	}

	if(i == 0)
		return 0;

	if(*VersionSet == 0)
	{
		if(i >= 2)
		{
			if(strcmp(Info[0], "Version") == 0)
			{
				int Version = strtol(Info[1], &End, 0);
                if(*End != 0 || Version < 3)
				{
					THROW_MSG(ERR_FORMAT_ERROR,
						"Invalid Flash Parameter file version : %s", Info[1]);
				}
				*VersionSet = 1;
				return 0;
			}
		}
	}

	if(i < 10)
	{
		THROW_MSG(ERR_FORMAT_ERROR, "Invalid Flash Parameter file ");
	}

	ManID = strtol(Info[FL_PARAM_MAN_ID], &End, 0);
	if(*End != 0)
		THROW_MSG(ERR_FORMAT_ERROR, "Invalid Flash Parameter file");
	DevID = strtol(Info[FL_PARAM_DEV_ID], &End, 0);
	if(*End != 0)
		THROW_MSG(ERR_FORMAT_ERROR, "Invalid Flash Parameter file");

	for(Chip = 0; Chip < FL_NumChips; Chip++)
	{
		if(FlashInfo[Chip].ManID != ManID || 
				FlashInfo[Chip].DevID != DevID)
		{
			continue;
		}

		if(MatchIndex >= 0)
		{
			FlashInfo[Chip] = FlashInfo[MatchIndex];
			Found++;
			FL_AvailableFlashSize += FlashInfo[Chip].SizeMBs * (MEGA_BYTE/8);
			continue;
		}

		strcpy(FlashInfo[Chip].ManName, Info[FL_PARAM_MAN_NAME]);
		FlashInfo[Chip].ManIDLong = strtol(Info[FL_PARAM_MAN_ID_LONG], &End, 0);
		strcpy(FlashInfo[Chip].DevName, Info[FL_PARAM_DEV_NAME]);
		FlashInfo[Chip].DevIDLong = strtol(Info[FL_PARAM_DEV_ID_LONG], &End, 0);
		FlashInfo[Chip].SizeMBs = strtol(Info[FL_PARAM_SIZE_MB], &End, 0);
		FlashInfo[Chip].Type = strtol(Info[FL_PARAM_TYPE], &End, 0);
		FlashInfo[Chip].NumSectors = FL_ParseIntArray(Ptr, 
								FlashInfo[Chip].SectorAddress,
								sizeof(FlashInfo[Chip].SectorAddress));
		if(FlashInfo[Chip].NumSectors <= 0)
			THROW_MSG(ERR_NOT_FOUND, "Invalid entry in flash parameter file");

		Found++;
		MatchIndex = Chip;

		FL_AvailableFlashSize += FlashInfo[Chip].SizeMBs * (MEGA_BYTE/8);
	}

	return Found;
}


/**
 * Get one token from a camea seperated list of items
 *
 * @param Str Pointer to the line data to be parsed
 *
 * @return Current token
 */
static char *FL_GetToken(char **Str)
{
    char *Ptr;
    char *Start;
	char *End;

    Ptr = *Str;
	/* Skip the initial white spaces */
    while(*Ptr == ' ' || *Ptr == '\t' || *Ptr == '\n' || *Ptr == '\r')
    {
		if(Ptr[0] == '/' && Ptr[1] == '/')
		{
			*Ptr = 0;
			break;
		}
        Ptr++;
    }

    Start = Ptr;

	/* Run till the end of the token */
    while(!(*Ptr == ',' || *Ptr == ' ' || *Ptr == '\t' || 
				*Ptr == 0 || *Ptr == '\n' || *Ptr == '\r'))
    {
		if(Ptr[0] == '/' && Ptr[1] == '/')
		{
			*Ptr = 0;
			break;
		}
        Ptr++;
    }

	/* Mark the end point */
	End = Ptr;

	/* Run till ',' */
	while(*Ptr != 0)
	{
		if(*Ptr == ',')
		{
			Ptr++;
			break;
		}
		Ptr++;
	}

	*Str = Ptr;
	*End = 0;

	/* If nothing in the token - return NULL */
	if(*Start == 0)
		return NULL;

    return Start;
}

/**
 * Get the flash physical address for the given offset.
 *
 * @param Offset Byte offset in flash
 * @param [out] Address Physical address on the flash
 * @param [out] RemaingSize Number of bytes available from the offset
 * @param [out] Type Flash device type (programing algorithm)
 *
 * @return SUCCESS/ERR_NOT_FOUND
 */
static int FL_GetFlashAddr(uint32 Offset, uint32 *Address, uint32 *RemaingSize,
																uint08 *Type)
{
    int Chip;
    uint32 Size = 0;
	uint32 ChipSize;

    for(Chip = 0; Chip < FL_NumChips; Chip++)
    {
		ChipSize = FlashInfo[Chip].SizeMBs * (MEGA_BYTE/8);
        if(Offset < Size + ChipSize)
        {
			break;
        }
        Size += ChipSize;
    }

	if(Chip >= FL_NumChips)
		THROW_MSG(ERR_NOT_FOUND, "Flash offset out of range : 0x%X", Offset);

	*Address = FlashCSOrder[Chip] + Offset - Size;
	*RemaingSize = Size + ChipSize - Offset;
	*Type = FlashInfo[Chip].Type;

	return SUCCESS;
}


/**
 * Returns the sector information for the given flash offset
 *
 * @param Offset Flash Offset in bytes
 * @param Start Start address of the sector
 * @param Size Size of the sector
 * @param Type Type of the chip
 *
 * @return  SUCCESS/ERR_NOT_FOUND
 */
static int FL_GetSectorInfo(uint32 Offset, uint32 *SectStart, uint32 *SectSize, 
																uint08 *Type)
{
    int Chip;
    uint32 Sect;
    uint32 Size = 0;
	uint32 ChipSize;

    for(Chip = 0; Chip < FL_NumChips; Chip++)
    {
		ChipSize = FlashInfo[Chip].SizeMBs * (MEGA_BYTE/8);
        if(Offset < Size + ChipSize)
        {
			break;
        }
        Size += ChipSize;
    }

	if(Chip >= FL_NumChips)
		THROW_MSG(ERR_NOT_FOUND, "Flash offset out of range : 0x%X", Offset);

	Offset -= Size;

	for(Sect = 1; Sect < FlashInfo[Chip].NumSectors; Sect++)
	{
        if(Offset < FlashInfo[Chip].SectorAddress[Sect])
        {
			break;
        }
	}

	*SectStart = FlashCSOrder[Chip] + FlashInfo[Chip].SectorAddress[Sect - 1];
	*Type = FlashInfo[Chip].Type;

	if(Sect >= FlashInfo[Chip].NumSectors)
	{
		*SectSize = ChipSize - FlashInfo[Chip].SectorAddress[Sect - 1];
	}
	else
	{
		*SectSize = FlashInfo[Chip].SectorAddress[Sect] - 
						FlashInfo[Chip].SectorAddress[Sect - 1];
	}

	return SUCCESS;
}
