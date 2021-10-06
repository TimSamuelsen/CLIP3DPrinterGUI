/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/

#ifndef PATTERN_H_
#define PATTERN_H_
/**
*
* @brief This module handles Pattern and image related functions
*
* @note:
*
*/

#include "error.h"
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
    PTN_COLOR_BLUE,
    PTN_COLOR_GREEN,
    PTN_COLOR_RED,
    PTN_NUM_COLORS
} PTN_Color_t;

typedef enum
{
    PTN_RGB24,
    PTN_GRAYSCALE16,
    PTN_NUM_FORMATS
} PTN_Format_t;

/** Image data structure */
typedef struct
{
	unsigned char *Buffer; /**< Pointer to the image buffer */
    int Width;             /**< Width of the image in pixels */
    int Height;            /**< Height of the image in pixels */
    int LineWidth;         /**< Number of bytes in a line */
    int FullHeight;        /**< Total height of the original image */
	uint08 BitDepth;       /**< Image pixel bitdepth */
    PTN_Format_t Format;   /**< Image Format */
}Image_t;


Image_t *PTN_Alloc(int Width, int Height, uint08 BitDepth,  PTN_Format_t Format);
void PTN_Free(Image_t *Image);
int PTN_Fill(Image_t *Image, uint08 Fill);

int PTN_Copy(Image_t *DstImg, Image_t const *SrcImg);
int PTN_Crop(Image_t *Image, int StartX, int StartY, int Width, int Height);
int PTN_Merge(Image_t *DstImg, Image_t const *SrcImg, int BitPos, int BitDepth);
int PTN_Quantize(Image_t *DstImg, Image_t const *SrcImg, int BitDepth);
int PTN_Extract(Image_t *DstImg, Image_t const *SrcImg, int BitPos, int BitDepth);
int PTN_SwapColors(Image_t *Image, PTN_Color_t Red, PTN_Color_t Green, 
													PTN_Color_t Blue);


#ifdef __cplusplus
}
#endif

#endif /* PATTERN_H_ */
