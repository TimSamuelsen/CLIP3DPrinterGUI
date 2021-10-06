/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/

/**
 * @brief This module handles batchfile related functions
 *
 * @note:
 */

#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "error.h"

#include "batchfile.h"
#include "API.h"

#define CMD_DISP_MODE           0x69
#define CMD_PTN_LUT             0x78
#define CMD_PTN_INIT_MASTER     0x2A
#define CMD_PTN_LOAD_MASTER     0x2B
#define CMD_PTN_INIT_SLAVE      0x2C
#define CMD_PTN_LOAD_SLAVE      0x2D
#define CMD_SPLASH_LOAD         0x7F
#define BAT_NAME_LEN			16
#define BAT_MAX_PTN_IMG			512

/******************************** MACROS **************************************/
/***************************** LOCAL TYPES ************************************/
/** Linked list for storing commands in batch file */
typedef struct BAT_List_t BAT_List_t;

struct BAT_List_t
{
    BAT_List_t *Next;       /**< Pointer to the next node */
	uint08 I2CCmd;			/**< I2C Command number */
	uint16 USBCmd;			/**< USB Command number */
	char const *CmdName;	/**< Command Name */
	uint16 PayloadLen;		/**< Number of bytes in payload */
    uint08 Payload[1];      /**< Command payload */
};

/** Batch file information */
struct BAT_BatchFile_t
{
    char Name[BAT_NAME_LEN]; /**< Name of the batch file */
    uint08 *Data;            /**< Data byte of the batch file */
    uint32 DataSize;         /**< Number of bytes in the batch file data */
    uint32 CmdCount;         /**< Number of commands in the batch file */
	uint16 PtnImgList[BAT_MAX_PTN_IMG]; /**< List of pattern images used */
    BAT_List_t *CmdList;     /**< List of commands */
    BAT_List_t **CmdListEnd; /**< Pointer to the last command */
};

/********************** LOCAL FUNCTION PROTOTYPES *****************************/
static uint32 BAT_ComputeCRC32(uint32 crc, const uint8 *buf, size_t len);
static char *BAT_GetToken(char **Str);
/****************************** VARIABLES *************************************/
/************************* FUNCTION DEFINITIONS********************************/
/**
 * Allocates memory for a new batch file
 *
 * @return Pointer to the allocated memory
 */
BAT_BatchFile_t *BAT_Alloc(void)
{
    BAT_BatchFile_t *BatchFile;

    BatchFile = malloc(sizeof(BAT_BatchFile_t));
    memset(BatchFile->Name, 0, sizeof(BatchFile->Name));
    BatchFile->CmdList = NULL;
    BatchFile->CmdListEnd = &BatchFile->CmdList;
    BatchFile->DataSize = 0;
    BatchFile->Data = NULL;
    BatchFile->CmdCount = 0;

    return BatchFile;
}


/**
 * Clears all the commands in the batch file
 *
 * @param BatchFile Batch file structure allocated using BAT_Alloc()
 *
 * @return SUCCESS/ERR_NULL_PTR
 */
int BAT_Clear(BAT_BatchFile_t* BatchFile)
{
    BAT_List_t *This;

    if(BatchFile == NULL)
        THROW(ERR_NULL_PTR);

    This = BatchFile->CmdList;

    while(This)
    {
        BAT_List_t *Next = This->Next;
        free(This);
        This = Next;
    }

    free(BatchFile->Data);
    BatchFile->CmdList = NULL;
	BatchFile->CmdCount = 0;
    BatchFile->CmdListEnd = &BatchFile->CmdList;
	BatchFile->Data = NULL;


    return SUCCESS;
}

/**
 * Clears all the commands in the batch file
 *
 * @param BatchFile Batch file structure allocated using BAT_Alloc()
 *
 * @return SUCCESS/ERR_NULL_PTR
 */
BAT_BatchFile_t *BAT_Copy(BAT_BatchFile_t* BatchFile)
{
	BAT_BatchFile_t *New;
    BAT_List_t *Cmd;

    if(BatchFile == NULL)
        THROW_MSG(NULL, "Null pointer input");

	New = BAT_Alloc();
	if(New == NULL)
        THROW_MSG(NULL, "Unable to allocate memory for batch file");

	for(Cmd = BatchFile->CmdList; Cmd != NULL; Cmd = Cmd->Next)
	{
		API_CommandInfo_t CmdInfo;
		CmdInfo.I2CCmd = Cmd->I2CCmd;
		CmdInfo.USBCmd = Cmd->USBCmd;
		CmdInfo.CmdName = Cmd->CmdName;
		CmdInfo.Read = 0;
		CmdInfo.Payload = Cmd->Payload;
		CmdInfo.PayloadLen = Cmd->PayloadLen;
		if(BAT_AddCommand(New, -1, &CmdInfo))
		{
			BAT_Free(New);
			THROW_MSG(NULL, "Adding batch command failed");
		}
    }

	memcpy(New->Name, BatchFile->Name, BAT_NAME_LEN);

    return New;
}

/**
 * Free the batch file memory allocated by the BAT_Alloc()
 *
 * @param BatchFile Batch file structure allocated using BAT_Alloc()
 *
 */
void BAT_Free(BAT_BatchFile_t* BatchFile)
{
    BAT_Clear(BatchFile);
    free(BatchFile);
    BatchFile = NULL;

}


/**
 * Returns the list of all the splash indexes used by the batch file
 *
 * @param BatchFile Batch file structure allocated using BAT_Alloc()
 * @param SplashIndex List of splash indexes
 *
 * @return Number of splash indexes used
 *
 */
int BAT_GetSplashList(BAT_BatchFile_t *BatchFile, uint16 *SplashIndex)
{
	BAT_List_t *Cmd;
	int Count = 0;

	if(BatchFile == NULL)
		return 0;

    for(Cmd = BatchFile->CmdList; Cmd != NULL; Cmd = Cmd->Next)
    {
		/* For all Pattern LUT commands */
		if(Cmd->I2CCmd == CMD_PTN_LUT)
		{
			uint16 Param = MAKE_WORD16(Cmd->Payload[11], Cmd->Payload[10]);
			SplashIndex[Count] = (Param & 0x7FF);
            Count++;
		}
		else if(Cmd->I2CCmd == CMD_SPLASH_LOAD)
		{
			SplashIndex[Count] = Cmd->Payload[0];
            Count++;
        }
	}

    return Count;
}

/**
 * Checks if a splash is referenced in the batch file
 *
 * @param BatchFile Batch file structure allocated using BAT_Alloc()
 * @param SplashIndex Splash index to be checked
 *
 * @return TRUE - If referenced, FALSE - If not referenced
 *
 */
int BAT_CheckSplashRef(BAT_BatchFile_t *BatchFile, uint16 SplashIndex)
{
	BAT_List_t *Cmd;

	if(BatchFile == NULL)
		return 0;

    for(Cmd = BatchFile->CmdList; Cmd != NULL; Cmd = Cmd->Next)
    {
		/* For all Pattern LUT commands */
		if(Cmd->I2CCmd == CMD_PTN_LUT)
		{
			uint16 Param = MAKE_WORD16(Cmd->Payload[11], Cmd->Payload[10]);
			if(SplashIndex == (Param & 0x7FF))
				return TRUE;
		}
#if 0
		else if(Cmd->I2CCmd == CMD_SPLASH_LOAD)
		{
			if(SplashIndex == Cmd->Payload[0])
				return TRUE;
        }
#endif
	}

    return FALSE;
}

/**
 * Adjust all the splash references for removal of given splash index
 *
 * @param BatchFile Batch file structure allocated using BAT_Alloc()
 * @param SplashIndex Splash index to be removed
 *
 * @return 0 on success, -1 on failure
 *
 */
int BAT_AdjustSplashIndex(BAT_BatchFile_t *BatchFile, uint16 SplashIndex)
{
	BAT_List_t *Cmd;

	if(BatchFile == NULL)
		return 0;

    for(Cmd = BatchFile->CmdList; Cmd != NULL; Cmd = Cmd->Next)
    {
		/* For all Pattern LUT commands */
		if(Cmd->I2CCmd == CMD_PTN_LUT)
		{
			uint16 Param = MAKE_WORD16(Cmd->Payload[11], Cmd->Payload[10]);
			if(SplashIndex < (Param & 0x7FF))
			{
				Param--;
				Cmd->Payload[10] = Param;
				Cmd->Payload[11] = Param >> 8;
			}
		}
		else if(Cmd->I2CCmd == CMD_SPLASH_LOAD)
		{
			if(SplashIndex < Cmd->Payload[0])
			{
				Cmd->Payload[0]--;
			}
        }
	}

    return FALSE;
}

/**
 * Converts all the OTF patterns to splash pattern
 *
 * @param BatchFile Batch file structure allocated using BAT_Alloc()
 * @param Splash offset
 *
 * @return 0 on success, -1 on failure
 *
 */
int BAT_OTF2SplashPtn(BAT_BatchFile_t *BatchFile, int SplashOffset)
{
	BAT_List_t *Cmd;
	BOOL OTFPattern = FALSE;

	if(BatchFile == NULL)
		return 0;

    for(Cmd = BatchFile->CmdList; Cmd != NULL; Cmd = Cmd->Next)
    {
		/* For all Pattern LUT commands */
		if(Cmd->I2CCmd == CMD_PTN_LUT)
		{
			uint16 Param = MAKE_WORD16(Cmd->Payload[11], Cmd->Payload[10]);
			if(OTFPattern == TRUE)
			{
				Param += SplashOffset;
				Cmd->Payload[10] = Param;
				Cmd->Payload[11] = Param >> 8;
			}
		}
		/* if "On the fly pattern" mode, convert to "splash pattern" mode */
		else if(Cmd->I2CCmd == CMD_DISP_MODE)
		{
			if(Cmd->Payload[0] == 3)
			{
				Cmd->Payload[0] = 1;
				OTFPattern = TRUE;
			}
			else
			{
				OTFPattern = FALSE;
			}
		}
	}

    return SUCCESS;
}

/**
 * Checks presents of OTF pattern update
 *
 * @param BatchFile Batch file structure allocated using BAT_Alloc()
 *
 * @return TRUE = OTF present, FALSE = OTF not present
 *
 */
int BAT_IsOTFPtnPresent(BAT_BatchFile_t *BatchFile)
{
	BAT_List_t *Cmd;
	BOOL OTFPattern = FALSE;

	if(BatchFile == NULL)
		return 0;

    for(Cmd = BatchFile->CmdList; Cmd != NULL; Cmd = Cmd->Next)
    {
		/* If LUT commands followed by OTF display mode */
		if(Cmd->I2CCmd == CMD_PTN_LUT)
		{
			if(OTFPattern == TRUE)
			{
				return TRUE;
			}
		}
		/* if "On the fly pattern" mode */
		else if(Cmd->I2CCmd == CMD_DISP_MODE)
		{
			if(Cmd->Payload[0] == 3)
			{
				OTFPattern = TRUE;
			}
		}
	}

    return FALSE;
}


/**
 * Sets the name for the batch file
 *
 * @param BatchFile Batch file structure allocated using BAT_Alloc()
 * @param Name 0 terminated string. Any character more than 16 will be ignored
 *
 * @return SUCCESS/ERR_NULL_PTR
 */
int BAT_SetName(BAT_BatchFile_t *BatchFile, char const *Name)
{
    int i;

    if(BatchFile == NULL || Name == NULL)
        THROW(ERR_NULL_PTR);

    memset(BatchFile->Name, 0, BAT_NAME_LEN);
    for(i = 0; Name[i] != 0 && i < BAT_NAME_LEN; i++)
    {
        BatchFile->Name[i] = Name[i];
    }

    return SUCCESS;
}

/**
 * Gets the name for the batch file
 *
 * @param BatchFile Batch file structure allocated using BAT_Alloc()
 * @param [out] Name 0 terminated string
 *
 * @return SUCCESS/ERR_NULL_PTR
 */
int BAT_GetName(BAT_BatchFile_t *BatchFile, char *Name)
{
    int i;

    if(BatchFile == NULL || Name == NULL)
        THROW(ERR_NULL_PTR);

    for(i = 0; i < BAT_NAME_LEN; i++)
    {
        Name[i] = BatchFile->Name[i];
    }
    Name[BAT_NAME_LEN] = 0;

    return SUCCESS;
}

/**
 * Add one command to the batch file command list
 * @param BatchFile Batch file data structure allocated using BAT_Alloc()
 * @param CmdInfo Command to be added
 *
 * @return SUCCESS/ERR_NULL_PTR/ERR_OUT_OF_RESOURCE/ERR_NOT_FOUND
 */
int BAT_AddCommand(BAT_BatchFile_t *BatchFile, int Index,
                                                    API_CommandInfo_t *CmdInfo)
{
    BAT_List_t *CmdNode;
    BAT_List_t **Insert;

    if(BatchFile == NULL)
        THROW(ERR_NULL_PTR);

    CmdNode = malloc(sizeof(BAT_List_t) + CmdInfo->PayloadLen - 1);
    if(CmdNode == NULL)
        THROW_MSG(ERR_OUT_OF_RESOURCE, "Not enough memory to add batch command");

	/* if not a write command */
    if(CmdInfo->Read == 1)
	{
		/* Dont add read commands to the batch file */
        return SUCCESS;
    }

    CmdNode->Next = NULL;
    CmdNode->I2CCmd = CmdInfo->I2CCmd;
    CmdNode->USBCmd = CmdInfo->USBCmd;
    CmdNode->PayloadLen = CmdInfo->PayloadLen;
	CmdNode->CmdName = CmdInfo->CmdName;
	memcpy(CmdNode->Payload, CmdInfo->Payload, CmdInfo->PayloadLen);

	/* If index negative, insert it at the end */
    if(Index < 0)
    {
        Insert = BatchFile->CmdListEnd;
    }
    else
    {
        Insert = &BatchFile->CmdList;
        while(Index > 0)
        {
			if(*Insert == NULL)
				THROW_MSG(ERR_NOT_FOUND, "Invalid batch command index");
            Insert = &(*Insert)->Next;
            Index--;
        }
    }

    CmdNode->Next = *Insert;
    *Insert = CmdNode;

    if(CmdNode->Next == NULL)
        BatchFile->CmdListEnd = &CmdNode->Next;

    BatchFile->CmdCount++;

    return SUCCESS;
}

/**
 * Removes the given command from the batch file structure
 *
 * @param BatchFile Batch file data structure allocated using BAT_Alloc()
 * @param Command index to be removed
 *
 * @return SUCCESS/ERR_NULL_PTR/ERR_NOT_FOUND
 *
 */
int BAT_RemoveCommand(BAT_BatchFile_t *BatchFile, int Index)
{
    BAT_List_t **Remove;
    BAT_List_t *Tmp;

    if(BatchFile == NULL)
        THROW(ERR_NULL_PTR);

    Remove = &BatchFile->CmdList;
    while(Index > 0)
    {
        if(*Remove == NULL)
            THROW_MSG(ERR_NOT_FOUND, "Invalid batch command index");
        Remove = &(*Remove)->Next;
        Index--;
    }

    Tmp = *Remove;
    *Remove = (*Remove)->Next;

    if(*Remove == NULL)
        BatchFile->CmdListEnd = Remove;

    free(Tmp);

    BatchFile->CmdCount--;

    return SUCCESS;
}

/**
 * Generates and returns the batch file data (flash format).
 *
 * @param BatchFile Batch file data structure allocated using BAT_Alloc()
 * @param Data [O] Batch file data
 *
 * @return Number of bytes in batch file data
 *
 */
int BAT_BuildData(BAT_BatchFile_t *BatchFile, uint08 **Data)
{
    BAT_List_t *Cmd;
    uint32 CRC;
    uint32 Index;

    if(BatchFile == NULL)
        THROW_MSG(ERR_NULL_PTR, "Invalid parameter");


    /* Go through all the commands and calculate size */
    Index = 4 + BAT_NAME_LEN;

	for(Cmd = BatchFile->CmdList; Cmd != NULL; Cmd = Cmd->Next)
    {
        /* Don't store image load commands in batch file */
        if(Cmd->I2CCmd != CMD_PTN_LOAD_MASTER &&
                Cmd->I2CCmd != CMD_PTN_INIT_MASTER &&
                Cmd->I2CCmd != CMD_PTN_LOAD_SLAVE &&
                Cmd->I2CCmd != CMD_PTN_INIT_SLAVE)
        {
            Index += Cmd->PayloadLen + 1;
        }
    }

    if(BatchFile->Data || BatchFile->DataSize < Index)
	{
        free(BatchFile->Data);
		BatchFile->Data = NULL;
	}

    BatchFile->Data = malloc(Index);
    if(BatchFile->Data == NULL)
        THROW_MSG(ERR_OUT_OF_RESOURCE, "Unable to allocate memory for batchfile");

    Index = 4 + BAT_NAME_LEN;

	for(Cmd = BatchFile->CmdList; Cmd != NULL; Cmd = Cmd->Next)
    {
        /* Don't store image load commands in batch file */
        if(Cmd->I2CCmd != CMD_PTN_LOAD_MASTER &&
                Cmd->I2CCmd != CMD_PTN_INIT_MASTER &&
                Cmd->I2CCmd != CMD_PTN_LOAD_SLAVE &&
                Cmd->I2CCmd != CMD_PTN_INIT_SLAVE)
        {
			/* Add write flag to the command */
            BatchFile->Data[Index] = Cmd->I2CCmd | 0x80;
            memcpy(&BatchFile->Data[Index + 1], Cmd->Payload, Cmd->PayloadLen);
			
			// BAT_UpdateSplashData(&BatchFile->Data[Index], SplashInfo);

            Index += Cmd->PayloadLen + 1;
        }
    }

    memcpy(&BatchFile->Data[4], BatchFile->Name, BAT_NAME_LEN);

    CRC = BAT_ComputeCRC32(0, BatchFile->Data + 4, Index - 4);

    BatchFile->Data[0] = BYTE0(CRC);
    BatchFile->Data[1] = BYTE1(CRC);
    BatchFile->Data[2] = BYTE2(CRC);
    BatchFile->Data[3] = BYTE3(CRC);

	*Data = BatchFile->Data;
	BatchFile->DataSize = Index;

	return Index;
}

/**
 * Returns information about given command.
 *
 * @param BatchFile Batch file data structure allocated using BAT_Alloc()
 * @param Index Index of the command to be retrieved
 * @param Info [O] Command information
 *
 * @return SUCCESS/ERR_NOT_FOUND/FAIL
 *
 */
int BAT_GetCommandInfo(BAT_BatchFile_t *BatchFile, int Index,
                                        BAT_CommandInfo_t *Info)
{
    BAT_List_t *Cmd;
	
    if(Info == NULL || BatchFile == NULL)
        THROW(ERR_NULL_PTR);

    Info->I2CCmd = 0;
	Info->USBCmd = 0;
	Info->CmdName = NULL;
    Info->Payload = NULL;
    Info->PayloadLen = 0;
	Info->Handle = NULL;

	for(Cmd = BatchFile->CmdList; Index > 0 && Cmd != NULL; Cmd = Cmd->Next)
    {
        Index--;
    }

	if(Cmd == NULL)
		THROW(ERR_NOT_FOUND);

    Info->I2CCmd = Cmd->I2CCmd;
    Info->USBCmd = Cmd->USBCmd;
    Info->CmdName = Cmd->CmdName;
    Info->Payload = Cmd->Payload;
    Info->PayloadLen = Cmd->PayloadLen;
	Info->Handle = (void *)Cmd->Next;

    return SUCCESS;
}

/**
 * Returns information about next command.
 *
 * @param Info [IO] Command information. Originally initialized using
 *        BAT_GetCommandInfo()
 *
 * @return SUCCESS/ERR_NOT_FOUND/FAIL
 *
 */
int BAT_NextCommandInfo(BAT_CommandInfo_t *Info)
{
    BAT_List_t *Cmd;

    if(Info == NULL)
        THROW(ERR_NULL_PTR);

    Cmd = Info->Handle;

	Info->I2CCmd = 0;
	Info->USBCmd = 0;
	Info->CmdName = NULL;
	Info->Payload = NULL;
	Info->PayloadLen = 0;
	Info->Handle = NULL;

    if(Cmd == NULL)
    {
        return ERR_NOT_FOUND;
    }

    Info->I2CCmd = Cmd->I2CCmd;
	Info->USBCmd = Cmd->USBCmd;
    Info->CmdName = Cmd->CmdName;
    Info->Payload = Cmd->Payload;
    Info->PayloadLen = Cmd->PayloadLen;
    Info->Handle = (void *)Cmd->Next;

    return SUCCESS;
}


/**
 * Returns the number of commands in the batch file
 *
 * @param BatchFile Batch file data structure allocated using BAT_Alloc()
 *
 * @return Number of commands in the batch file
 *
 */
int BAT_GetCommandCount(BAT_BatchFile_t *BatchFile)
{
    if(BatchFile == NULL)
        THROW(ERR_NULL_PTR);

    return BatchFile->CmdCount;
}

/**
 * Load the batch file structure from binary data
 *
 * @param BatchFile Batch file data structure allocated using BAT_Alloc()
 * @param Data Pointer to the batch file data
 *
 * @return SUCCESS/ERR_NULL_PTR/ERR_FORMAT_ERROR
 *
 */
int BAT_LoadFromData(BAT_BatchFile_t *BatchFile, uint08 *Data, int Size)
{
	uint32 CRCStored;
	uint32 CRC;
	int Index;
	char Name[BAT_NAME_LEN + 1];

	if(Size < BAT_NAME_LEN + 4)
		THROW_MSG(ERR_FORMAT_ERROR, "Invalid batch file data");

	CRCStored = MAKE_WORD32(Data[3], Data[2], Data[1], Data[0]);

    CRC = BAT_ComputeCRC32(0, Data + 4, Size - 4);
	if(CRC != CRCStored)
		THROW_MSG(ERR_FORMAT_ERROR, "Batch file data CRC mismatch");

	memcpy(Name, &Data[4], BAT_NAME_LEN);
	Name[BAT_NAME_LEN] = 0;

	Index = BAT_NAME_LEN + 4;
	while(Index < Size)
	{
		int Length;
		API_CommandInfo_t CmdInfo;
		if(API_GetCommandLength(&Data[Index], &Length))
			THROW_MSG(ERR_FORMAT_ERROR, 
					"Batch file data invalid command : %02X", Data[Index]);

        CmdInfo.I2CCmd = Data[Index] & 0x7F;
        CmdInfo.Read = !(Data[Index] >> 7);
		if(API_GetCommandName(CmdInfo.I2CCmd, &CmdInfo.CmdName))
		{
			THROW_MSG(ERR_NOT_FOUND, 
				"Invalid command found in batch file : %02X", CmdInfo.I2CCmd);
		}
		if(API_GetUSBCommand(CmdInfo.CmdName, &CmdInfo.USBCmd))
		{
			THROW_MSG(ERR_NOT_FOUND, 
				"Invalid command found in batch file : %02X", CmdInfo.I2CCmd);
		}
		CmdInfo.Payload = &Data[Index+1];
		CmdInfo.PayloadLen = Length;

		TRY(BAT_AddCommand(BatchFile, -1, &CmdInfo));

		Index += Length + 1;
	}

	TRY(BAT_SetName(BatchFile, Name));

	return SUCCESS;
}

/**
 * Load the batch file structure from a disk file. This file typically generated
 * using BAT_SaveToFile()
 *
 * @param BatchFile Batch file data structure allocated using BAT_Alloc()
 * @param FileName Disk file name
 *
 * @return SUCCESS/ERR_NULL_PTR/ERR_FORMAT_ERROR
 *
 */
int BAT_LoadFromFile(BAT_BatchFile_t *BatchFile, char const *FileName)
{
    int Error;
    FILE *File = NULL;
    static char Line[5500 + 32];
    static uint08 Payload[1030];
    uint08 CmdNum;

    TRY_BEGIN()
    {
        if(BatchFile == NULL || FileName == NULL)
            TRY_THROW(ERR_NULL_PTR);

        File = fopen(FileName, "r");
        if(File == NULL)
            TRY_THROW_MSG(FAIL, "Unable to read file : %s", FileName);

        while(fgets(Line, sizeof(Line), File) != NULL)
        {
            char *Token;
            char const *CmdStr;
            char *Ptr = Line;
            uint16 Index;
			API_CommandInfo_t CmdInfo;

            Token = BAT_GetToken(&Ptr);
            if(Token == NULL)
                continue;

            if(API_GetI2CCommand(Token, &CmdNum) == -1)
                TRY_THROW_MSG(ERR_FORMAT_ERROR,
                                    "Invalid command name : %s", Token);
            if(API_GetCommandName(CmdNum, &CmdStr))
                TRY_THROW_MSG(ERR_FORMAT_ERROR,
                                    "Invalid command name : %s", Token);
            Index = 0;
            while((Token = BAT_GetToken(&Ptr)))
            {
                int Value;
				char *End;
                Value = strtol(Token, &End, 0);
                if(*End != 0 || Value > 0xFF)
                    TRY_THROW_MSG(ERR_FORMAT_ERROR,
                                        "Invalid command data : %s", Token);

				if(Index >= ARRAY_SIZE(Payload))
					TRY_THROW_MSG(ERR_FORMAT_ERROR,
									"Too many bytes in payload : %s", CmdStr);
                Payload[Index++] = Value;
            }
			CmdInfo.I2CCmd = CmdNum;
			CmdInfo.CmdName = CmdStr;
			if(API_GetUSBCommand(CmdInfo.CmdName, &CmdInfo.USBCmd))
			{
                TRY_THROW_MSG(ERR_FORMAT_ERROR, "Invalid command name : %s", 
															CmdInfo.CmdName);
			}
			CmdInfo.Read = 0;
			CmdInfo.Payload = Payload;
			CmdInfo.PayloadLen = Index;
            /* all commands in the existing batch files are assumed 
			 * to be write commands */
            BAT_AddCommand(BatchFile, -1, &CmdInfo);
        }
    }
    TRY_END(Error);

    if(File)
        fclose(File);

    return Error;
}

/**
 * Save the batch file content to a disk file
 *
 * @param BatchFile Batch file data structure allocated using BAT_Alloc()
 * @param FileName Disk file name
 *
 * @return SUCCESS/ERR_NULL_PTR/ERR_NOT_SUPPORTED
 */
int BAT_SaveToFile(BAT_BatchFile_t *BatchFile, char const *FileName)
{
    int Error;
    FILE *File = NULL;
    BAT_List_t *Cmd;
    int i;

    TRY_BEGIN()
    {
        if(BatchFile == NULL || FileName == NULL)
            TRY_THROW(ERR_NULL_PTR);

        File = fopen(FileName, "w");
        if(File == NULL)
            TRY_THROW_MSG(FAIL, "Unable to open file : %s", FileName);

        for(Cmd = BatchFile->CmdList; Cmd != NULL; Cmd = Cmd->Next)
        {
            fprintf(File, "%s : ", Cmd->CmdName);
            for(i = 0; i < Cmd->PayloadLen; i++)
            {
                fprintf(File, "0x%02X ", Cmd->Payload[i]);
            }
            fputc('\n',File);
        }
    }
    TRY_END(Error);

    if(File)
        fclose(File);

    return Error;
}

/**
 * Get one token from a line of batch file data
 *
 * @param Str Pointer to the line data to be parsed
 *
 * @return Current token
 */
static char *BAT_GetToken(char **Str)
{
    char *Ptr;
    char *Start;

    Ptr = *Str;
    while(!isalnum((int)*Ptr) && *Ptr != '_')
    {
        if(*Ptr == 0)
            return NULL;
        Ptr++;
    }

    Start = Ptr;

    while(isalnum((int)*Ptr) || *Ptr == '_')
    {
        Ptr++;
    }

    if(*Ptr == 0)
    {
        /* String ended */
        *Str = Ptr;
    }
    else
    {
        /* Still more token to parse */
        *Ptr = 0;
        *Str = Ptr + 1;
    }

    return Start;
}


/**
* Computes the 32bit CRC for the given data
*
* crc - Initial CRC
* buf - Pointer to the data
* len - Length in bytes
*
* returns the computed CRC
*/
static uint32 BAT_ComputeCRC32(uint32 crc, const uint8 *buf, size_t len)
{
    static uint32 table[256];
    static int have_table = 0;
    uint32 rem, octet;
    int i, j;
    const uint8 *p, *q;

    /* This check is not thread safe; there is no mutex. */
    if (have_table == 0) {
        /* Calculate CRC table. */
        for (i = 0; i < 256; i++) {
            rem = i;  /* remainder from polynomial division */
            for (j = 0; j < 8; j++) {
                if (rem & 1) {
                    rem >>= 1;
                    rem ^= 0xedb88320;
                } else
                    rem >>= 1;
            }
            table[i] = rem;
        }
        have_table = 1;
    }

    crc = ~crc;
    q = buf + len;
    for (p = buf; p < q; p++) {
        octet = *p;  /* Cast to unsigned octet. */
        crc = (crc >> 8) ^ table[(crc & 0xff) ^ octet];
    }
    return ~crc;
}


#if 0

/**
 * Allocates BAT_SplashInfo_t structure for given number of splash index
 *
 * @param NumIndex - Number of splash index to be stored
 * @return Pointer to the allocated memory, NULL if allocation failed
 */
BAT_SplashInfo_t *BAT_AllocSplashInfo(int NumIndex)
{
	return (BAT_SplashInfo_t *)malloc(4 + NumIndex * 4);
}

/**
 * Free the memory allocated by BAT_AllocSplashInfo()
 *
 */
void BAT_FreeSplashInfo(BAT_SplashInfo_t *SplashInfo)
{
	free(SplashInfo);
    SplashInfo = NULL;
}


/**
 * Updates the splash index info in the command
 *
 * @param Data - Command Data bytes
 * @param SplashInfo - Splash index related inforamtion
 *
 * @return TRUE - If the command data is updated, FALSE otherwise.
 *
 */
static BOOL BAT_UpdateSplashData(uint08 *Data, 
											BAT_SplashInfo_t const *SplashInfo) 
{
	BOOL Updated = FALSE;
	static BOOL OTFPattern = FALSE;

	if(Data == NULL || SplashInfo == NULL)
		return FALSE;

	if(Data[0] == CMD_SPLASH_LOAD)
	{
		int i;
		/* Convert the image index */
		for(i = 0; i < SplashInfo->NumSplashIndex; i++)
		{
			if(Data[1] == SplashInfo->SplashIndexMap[i].Org)
			{
				Data[1] = SplashInfo->SplashIndexMap[i].New;
				break;
			}
		}
	}
	else if(Data[0] == CMD_PTN_LUT)
	{
		uint16 Param = MAKE_WORD16(Data[12], Data[11]);
		if(OTFPattern == TRUE)
		{
			/* Offset the image index in pattern LUT
			 * on the fly pattern mode */
			Param += SplashInfo->OTFSplashOffset;
			Updated = TRUE;
		}
		else
		{
			int i;
			/* Convert the image index */
			for(i = 0; i < SplashInfo->NumSplashIndex; i++)
			{
                if((Param & 0x7FF) == SplashInfo->SplashIndexMap[i].Org)
				{
                    Param = (Param & 0xF800) |
                            SplashInfo->SplashIndexMap[i].New;
					Updated = TRUE;
					break;
				}
			}
		}

		Data[12] = Param >> 8;
		Data[11] = Param;
	}
	/* if "On the fly pattern" mode, convert to "splash pattern" mode */
	else if(Data[0] == CMD_DISP_MODE)
	{
		if(Data[1] == 3)
		{
			Data[1] = 1;
			OTFPattern = TRUE;
			Updated = TRUE;
		}
		else
		{
			OTFPattern = FALSE;
		}
	}

	return Updated;
}
#endif
