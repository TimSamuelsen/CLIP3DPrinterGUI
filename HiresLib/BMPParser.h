/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/
#ifndef BMPPARSER_H_
#define BMPPARSER_H_

#include "Error.h"
#include "Common.h"
#include "Pattern.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (BMP_DataFunc_t)(void *Param, uint8 *Data, uint32 Size);
typedef int (BMP_PixelFunc_t)(void *Param, int X, int Y, 
												uint8 *PixValue, uint32 Count);

Image_t *BMP_AllocImage(int Width, int Height, uint08 BitDepth);

uint32 BMP_BMPFileSize(Image_t *Image);

int BMP_ParseImage(BMP_DataFunc_t *GetData, void *DataParam,
								BMP_PixelFunc_t *DrawPixels, void *DrawParam,
								uint8 OutBitDepth);

int BMP_LoadFromFile(const char *FileName, Image_t *Image);

int BMP_SaveToFile(Image_t *Image, const char *FileName);

int BMP_GetFileInfo(const char *FileName, Image_t *BMPHeader);

void BMP_FreeImage(Image_t *Image);

int BMP_StoreImage(Image_t *Image, BMP_DataFunc_t *PutData, 
								void *DataParam, BMP_PixelFunc_t *GetPixels, 
								void *PixelParam);

#ifdef __cplusplus
}
#endif

#endif /* BMPPARSER_H_ */
