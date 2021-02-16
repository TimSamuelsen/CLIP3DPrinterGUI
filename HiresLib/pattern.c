/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/

/**
*
* @brief This module handles Pattern and image related functions
*
* @note:
*
*/

#include <string.h>
#include "common.h"
#include "error.h"

#include "pattern.h"


/******************************** MACROS **************************************/
#define RGB24_BYTES_PER_PIX       3
#define GRAYSCALE16_BYTES_PER_PIX 2
/***************************** LOCAL TYPES ************************************/

/********************** LOCAL FUNCTION PROTOTYPES *****************************/
/****************************** VARIABLES *************************************/
/************************* FUNCTION DEFINITIONS********************************/

/**
 * Allocate memory to store the the pattern image structure. User should free
 * the memory using PTN_Free() API
 * @param Width Max width of the image
 * @param Height Max height of the image
 * @param BitDepth Bit depth of the image
 * @param Format - Format of the pattern image
 *
 * @return Pointer to the allocated image data structure
 */
Image_t *PTN_Alloc(int Width, int Height, uint08 BitDepth, PTN_Format_t Format)
{
    Image_t *Image;
    int LineWidth = 0;
    if(Format == PTN_RGB24)
    {
        LineWidth = ALIGN_BYTES_NEXT(Width * RGB24_BYTES_PER_PIX, 4);
    }
    else if(Format == PTN_GRAYSCALE16)
    {
        LineWidth = ALIGN_BYTES_NEXT(Width * 2, 4);
    }
    else
    {
        return NULL;
    }

    Image = malloc(Height*LineWidth + sizeof(Image_t));

    if(Image == NULL)
        return NULL;

    /* Use memory from end of image for the buffer */
    Image->Buffer = (uint08 *)(Image + 1);

    Image->Width = Width;
    Image->Height = Height;
    Image->FullHeight = Height;
    Image->LineWidth = LineWidth;
    Image->BitDepth = BitDepth;
    Image->Format = Format;

    return Image;
}

/**
 * Fee the image memory allocated by the PTN_Alloc() API
 *
 * @param Image image pointer to be freed
 *
 */
void PTN_Free(Image_t *Image)
{
    free(Image);
    Image = NULL;
}

/**
 * Copy the content of the source image to the destination
 * after croping and quantizing the image to the destination format
 *
 * @param DstImg Destination image 
 * @param SrcImg Source image
 *
 * @return SUCCESS/ERR_NULL_PTR
 */
int PTN_Copy(Image_t *DstImg, Image_t const *SrcImg)
{
    int y;
    int Width;
    int Height;
    uint08 *DstPtr;
    uint08 *SrcPtr;

    if(DstImg == NULL || SrcImg == NULL)
        THROW(ERR_NULL_PTR);

#if 0
    if(DstImg->BitDepth < SrcImg->BitDepth)
    {
        return PTN_Quantize(DstImg, SrcImg, DstImg->BitDepth);
    }
#endif

    Width = MIN(DstImg->Width, SrcImg->Width);
    Height = MIN(DstImg->Height, SrcImg->Height);

    DstPtr = DstImg->Buffer;
    SrcPtr = SrcImg->Buffer;

    for(y = 0; y < Height; y++)
    {
        memcpy(DstPtr, SrcPtr, Width * RGB24_BYTES_PER_PIX);
        DstPtr += DstImg->LineWidth;
        SrcPtr += SrcImg->LineWidth;
    }

    DstImg->BitDepth = SrcImg->BitDepth;
    DstImg->Format = SrcImg->Format;

    return SUCCESS;
}

/**
 * Crop the given image to the new dimension. If all the dimensions are 0 then
 * it resets the previously applied croppings.
 *
 * @param Image Image to be cropped
 * @param StartX Start x position within the given image
 * @param StartY Start y position within the given image
 * @param Width New width of the image
 * @param Height New height of the image
 *
 * @return  SUCCESS/ERR_NULL_PTR/ERR_INVALID_PARAM
 */
int PTN_Crop(Image_t *Image, int StartX, int StartY, int Width, int Height)
{
	if(Image == NULL)
		THROW(ERR_NULL_PTR);

    int BytesPerPixel = 0;

    if(Image->Format == PTN_RGB24)
    {
        BytesPerPixel = RGB24_BYTES_PER_PIX;
    }
    else if(Image->Format == PTN_GRAYSCALE16)
    {
        BytesPerPixel = GRAYSCALE16_BYTES_PER_PIX;
    }

    if(Width == 0 && Height == 0 && StartX == 0 && StartY == 0)
	{
		Image->Buffer = (uint08 *)(Image + 1);
        Image->Width = Image->LineWidth/BytesPerPixel;
		Image->Height = Image->FullHeight;
	}
	else
	{

		if(StartX + Width > Image->Width)
			THROW(ERR_INVALID_PARAM);

		if(StartY + Height > Image->Height)
			THROW(ERR_INVALID_PARAM);

		Image->Buffer = Image->Buffer + 
						Image->LineWidth * StartY +
                        StartX * BytesPerPixel;

		Image->Width = Width;
		Image->Height = Height;
	}

    return SUCCESS;
}

/**
 * Merges the source image bits in each pixel to the destination image pixel of type RGB24.
 *
 * @param DstImg Destination image
 * @param SrcImg Source image
 * @param BitPos Where to merge the bits (0 - 31)
 * @param BitDepth Bit depth of the SrcImage (1 - 8). If 0, then use the
 *        original bit depth of the source image.
 *
 * @return SUCCESS/ERR_NULL_PTR/ERR_INVALID_PARAM
 *
 */
int PTN_Merge(Image_t *DstImg, Image_t const *SrcImg, int BitPos, int BitDepth)
{
	int y;
	int x;
    int z;
    int Width;
    int Height;
    uint08 *DstPtr;
    uint08 *SrcPtr;

    if(DstImg == NULL || SrcImg == NULL)
        THROW(ERR_NULL_PTR);

	if(BitDepth == 0)
		BitDepth = SrcImg->BitDepth;

    if(BitPos > 31 || BitDepth > 16)
        THROW(ERR_INVALID_PARAM);

    Width = MIN(DstImg->Width, SrcImg->Width);
    Height = MIN(DstImg->Height, SrcImg->Height);
    DstPtr = DstImg->Buffer;
    SrcPtr = SrcImg->Buffer;

    int BytesPerPixel = 0;

    if(SrcImg->Format == PTN_RGB24)
    {
        BytesPerPixel = RGB24_BYTES_PER_PIX;
    }
    else if(SrcImg->Format == PTN_GRAYSCALE16)
    {
        BytesPerPixel = GRAYSCALE16_BYTES_PER_PIX;
    }
    Width *= BytesPerPixel;
    int DstWidth = DstImg->Width * RGB24_BYTES_PER_PIX;

    if((SrcImg->BitDepth == 24) && (SrcImg->Format == PTN_RGB24))
    {
        for(y = 0; y < Height; y++)
        {
            for(x = 0; x < Width; x += BytesPerPixel)
            {
                /* OR all the color channels before converting to 1-8 bit */
                uint32 Gray = SrcPtr[x] | SrcPtr[x+1] | SrcPtr[x+2];
                uint32 Dst = DstPtr[x] | DstPtr[x+1] << 8 | DstPtr[x+2] << 16;
                if(BitDepth <= 8)
                {
                    Gray >>= 8 - BitDepth;
                }
                else
                {
                    Gray <<= BitDepth - 8;
                }
                Dst |= Gray << BitPos;
                DstPtr[x] = Dst;
                DstPtr[x+1] = Dst >> 8;
                DstPtr[x+2] = Dst >> 16;
            }
            DstPtr += DstImg->LineWidth;
            SrcPtr += SrcImg->LineWidth;
        }
    }
    else if ((SrcImg->BitDepth == 16) && (SrcImg->Format == PTN_GRAYSCALE16))
    {
        for(y = 0; y < Height; y++)
        {
            for(x = 0, z = 0; x < Width, z < DstWidth; x += BytesPerPixel, z += RGB24_BYTES_PER_PIX)
            {
                /*  Get the Gray pixel */
                uint32 Gray = SrcPtr[x] | SrcPtr[x+1] << 8;
                uint32 Dst = DstPtr[z] | DstPtr[z+1] << 8 | DstPtr[z+2] << 16;
                Gray >>= 16 - BitDepth;
                Dst |= Gray << BitPos;
                DstPtr[z] = Dst;
                DstPtr[z+1] = Dst >> 8;
                DstPtr[z+2] = Dst >> 16;
            }
            DstPtr += DstImg->LineWidth;
            SrcPtr += SrcImg->LineWidth;
        }
    }

	return SUCCESS;
}

/**
 * Extracts the specific bits from the source image to create a new image.
 * The destination image bit depth will be equal to the extracted bits
 *
 * @param DstImg Destination image
 * @param SrcImg Source image from where to extract the bits
 * @param BitPos Bit position from where to extract the bits (0 - 31)
 * @param BitDepth Number of bits to extract (1 - 8). If 0 then use the
 *        Destination image bit depth
 *
 * @return SUCCESS/ERR_NULL_PTR/ERR_INVALID_PARAM
 */
int PTN_Extract(Image_t *DstImg, Image_t const *SrcImg, int BitPos, int BitDepth)
{
    int y;
    int x;
    uint08 OMask;
    int Width;
    int Height;
    uint08 *DstPtr;
    uint08 *SrcPtr;

    if(DstImg == NULL || SrcImg == NULL)
        THROW(ERR_NULL_PTR);

	if(BitDepth == 0)
		BitDepth = DstImg->BitDepth;

    if(BitDepth > 8)
        THROW(ERR_INVALID_PARAM);

    Width = MIN(DstImg->Width, SrcImg->Width);
    Height = MIN(DstImg->Height, SrcImg->Height);
    DstPtr = DstImg->Buffer;
    SrcPtr = SrcImg->Buffer;

    OMask = GEN_BIT_MASK(0, 8 - BitDepth);

    Width *= RGB24_BYTES_PER_PIX;

    for(y = 0; y < Height; y++)
    {
        for(x = 0; x < Width; x += RGB24_BYTES_PER_PIX)
        {
            uint32 Gray = SrcPtr[x] | SrcPtr[x+1] << 8 | SrcPtr[x+2] << 16;
            /* OR all the color channles before converting to 1-8 bit */
            Gray = ((Gray >> BitPos) << (8-BitDepth)) & 0xFF;

            if(Gray)
                Gray |= OMask;

            DstPtr[x] = Gray;
            DstPtr[x+1] = Gray;
            DstPtr[x+2] = Gray;
        }
        DstPtr += DstImg->LineWidth;
        SrcPtr += SrcImg->LineWidth;
    }

    DstImg->BitDepth = BitDepth;

    return SUCCESS;
}

/**
 * Quantize the source image to a smaller bit depth image and copy it to the
 * destination image.
 *
 * @param DstImg - Destination image
 * @param SrcImg - Source image
 * @param BitDepth - Bit depth to be quantized to (1 - 8). If 0 then use the
 *        Destination image bit depth
 *
 * @return SUCCESS/ERR_NULL_PTR/ERR_INVALID_PARAM
 */
int PTN_Quantize(Image_t *DstImg, Image_t const *SrcImg, int BitDepth)
{
	int y;
	int x;
    int Width = MIN(DstImg->Width, SrcImg->Width);
    int Height = MIN(DstImg->Height, SrcImg->Height);
    uint08 AMask;
    uint08 OMask;
    uint08 *DstPtr = DstImg->Buffer;
    uint08 *SrcPtr = SrcImg->Buffer;

    if(DstImg == NULL || SrcImg == NULL)
        THROW(ERR_NULL_PTR);

	if(BitDepth == 0)
		BitDepth = DstImg->BitDepth;

	if(BitDepth > 8)
		THROW(ERR_INVALID_PARAM);

    AMask = GEN_BIT_MASK(8 - BitDepth, BitDepth);
    if(BitDepth == 8)
        OMask = 0;
    else
        OMask = GEN_BIT_MASK(0, 8 - BitDepth);

    Width *= RGB24_BYTES_PER_PIX;

    for(y = 0; y < Height; y++)
	{
        for(x = 0; x < Width; x += RGB24_BYTES_PER_PIX)
		{
            /* OR all the color channels before converting to 1-8 bit */
            uint08 Gray = SrcPtr[x] | SrcPtr[x+1] | SrcPtr[x+2];
            Gray = Gray & AMask;
            if(Gray)
                Gray |= OMask;

            DstPtr[x] = Gray;
            DstPtr[x+1] = Gray;
            DstPtr[x+2] = Gray;
        }
        DstPtr += DstImg->LineWidth;
        SrcPtr += SrcImg->LineWidth;
	}

    DstImg->BitDepth = BitDepth;

	return SUCCESS;
}

/**
 * Fill the image with a given pixel value
 *
 * @param Image - Image to be filled
 * @param Fill - Byte pattern to be filled
 *
 * @return SUCCESS/ERR_NULL_PTR
 */
int PTN_Fill(Image_t *Image, uint08 Fill)
{
	int y;
	uint08 *Ptr;

	if(Image == NULL)
		THROW(ERR_NULL_PTR);

	Ptr = Image->Buffer;

	for(y = 0; y < Image->Height; y++)
	{
        memset(Ptr, Fill, Image->Width * RGB24_BYTES_PER_PIX);
		Ptr += Image->LineWidth;
	}

	return SUCCESS;
}


/**
 * Swaps the colors channels according to the user defined position
 *
 * @param Image Image to be updated
 * @param Red Red color should change to this
 * @param Green Green color should change to this
 * @param Blue Blue color should change to this
 *
 * @return SUCCESS/ERR_NULL_PTR
 */
int PTN_SwapColors(Image_t *Image, PTN_Color_t Red, PTN_Color_t Green, 
													PTN_Color_t Blue)
{
	uint08 *LinePtr =  Image->Buffer;
	uint08 *PixPtr;
    int X;
    int Y;

    if((Image == NULL) || (Image->Format == PTN_GRAYSCALE16))
		THROW(ERR_NULL_PTR);

	for(Y = 0; Y < Image->Height; Y++)
	{
		PixPtr = LinePtr;
		LinePtr += Image->LineWidth;
		for(X = 0; X < Image->Width; X++)
		{
			uint08 RedVal = PixPtr[PTN_COLOR_RED];
			uint08 GreenVal = PixPtr[PTN_COLOR_GREEN];
			uint08 BlueVal = PixPtr[PTN_COLOR_BLUE];

			PixPtr[Red] = RedVal;
			PixPtr[Green] = GreenVal;
			PixPtr[Blue] = BlueVal;
			PixPtr += 3;
		}
	}

	return SUCCESS;
}
