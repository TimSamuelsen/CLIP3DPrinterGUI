/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/

/**
 * This module provides APIs for handling the splash image stored in flash
 */
#include "splash.h"
#include "compress.h"
#include "error.h"

#define BYTES_PER_PIXEL     3

/** Splash header format stored in flash */
typedef struct
{
    char   Signature[4];     /**< format 4 == "Spld" */
                             /**< (0x53, 0x70, 0x6c, 0x64) */

    uint16 Image_width;      /**< width of image in pixels */
    uint16 Image_height;     /**< height of image in pixels */
    uint32 Byte_count;       /**< number of bytes starting at "data" */
    uint32 Subimg_offset;    /**< byte-offset from "data" to 1st line */
                             /**< of sub-image, or 0xFFFFFFFF if none. */

    uint32 Subimg_end;       /**< byte-offset from "data" to end of */
                             /**< last line of sub-image, */
                             /**< or 0xFFFFFFFF if none. */

    uint32 Bg_color;         /**< unpacked 24-bit background color */
                             /**< (format: 0x00RRGGBB) */

    uint08 Pixel_format;     /**< format of pixel data */
                             /**< 0 = 24-bit unpacked: 0x00RRGGBB Not supported*/
                             /**< 1 = 24-bit packed:   RGB 8-8-8 */
                             /**< 2 = 16-bit:          RGB 5-6-5  Not supported*/
                             /**< 3 = 16-bit:          YCrCb 4:2:2 */

    uint08 Compression;      /**< compression of image */
                             /**< 0 = uncompressed */
                             /**< 1 = RLE */
                             /**< 2 = RLE1 */
                             /**< 3 = 2Pixel RLE */

    uint08 ByteOrder;        /**< 0 = pixel is 00RRGGBB - Not supported */
                             /**< 1 = pixel is 00GGRRBB */

    uint08 ChromaOrder;      /**< Indicates first pixel */
                             /**< 0 = Cr is first pixel (0xYYRR) */
                             /**< 1 = Cb is first pixel (0xYYBB) */

    uint08 Is3D;             /**< 0 = 2D, this image is independent */
                             /**< 1 = 3D, this image is part of a 2 image eye pair */

    uint08 IsLeftImage;      /**< 0 = if Is3D=1, this is the Right Image */
                             /**< 1 = if Is3D=1, this is the Left Image */

    uint08 IsVertSubSampled; /**< 0 = image is not vertically sub-sampled */
                             /**< 1 = image is vertically sub-sampled */

    uint08 IsHorzSubSampled; /**< 0 = image is not horizontally sub-sampled */
                             /**< 1 = image is horizontally sub-sampled */

    uint08 IsSPCheckerboard; /**< 0 = image is normal orthogonal image */
                             /**< 1 = image is Smooth Picture(tm) pre-merged checkerboard image */

    uint08 ChromaSwap;       /**< Indicates whether YUV source has chroma channels inverted */
                             /**< 0 = Source chroma channels are inverted. */
                             /**< 1 = Source chroma channels are not inverted.*/

    uint08  Pad[14];         /**< pad so that data starts at 16-byte boundary */
} SPL_Header_t;

/**
 * Copy the image as uncompressed splash image to the destination pointer
 *
 * @param Iamge Image to be copied
 * @param OutPtr Destination pointer to store the uncompressed image
 *
 * @return Number of bytes in the uncompressed image
 *
 */
static int Image2SplashUncompressed(Image_t const *Image, uint08 *OutPtr)
{
    uint08 *InPtr =  Image->Buffer;
    int x;
    int y;

    for(y = 0; y < Image->Height; y++)
    {
        for(x = 0; x < Image->Width * BYTES_PER_PIXEL; x+= BYTES_PER_PIXEL)
        {
            OutPtr[x + 0] = InPtr[x + 2];
            OutPtr[x + 1] = InPtr[x + 0];
            OutPtr[x + 2] = InPtr[x + 1];
        }
        OutPtr += ALIGN_BYTES_NEXT(Image->Width * BYTES_PER_PIXEL, 16);
        InPtr += Image->LineWidth;
    }
    return ((ALIGN_BYTES_NEXT(Image->Width * BYTES_PER_PIXEL, 16)) * Image->Height);
}

/**
 * Copy the image as uncompressed splash image to the destination pointer
 *
 * @param Iamge Image to be copied
 * @param OutPtr Destination pointer to store the uncompressed image
 *
 */
static void Splash2ImageUncompressed(uint08 const *Splash, Image_t *Image)
{
    uint08 *OutPtr =  Image->Buffer;
    int x;
    int y;

    for(y = 0; y < Image->Height; y++)
    {
        for(x = 0; x < Image->Width * BYTES_PER_PIXEL; x += BYTES_PER_PIXEL)
        {
            OutPtr[x + 0] = Splash[x + 1];
            OutPtr[x + 1] = Splash[x + 2];
            OutPtr[x + 2] = Splash[x + 0];
        }
        Splash += ALIGN_BYTES_NEXT(Image->Width * BYTES_PER_PIXEL, 16);
        OutPtr += Image->LineWidth;
    }
}

/**
 * Converts the given input image to Splash image
 *
 * @param Image Input image (Should be in 00RRGGBB foramt)
 * @param Compression Compression type
 * @param Splash [out] Splash image. The memory should be freed by the caller
 *
 * @return FAIL,
 *
 */
int SPL_ConvImageToSplash(Image_t const *Image, SPL_Compression_t Compression,
                                                                uint08 *Splash)
{
    SPL_Header_t *Header;
    int Size;

    if(Image == NULL || Splash == NULL)
        THROW(ERR_NULL_PTR);

    Header = (SPL_Header_t *)Splash;

    Splash = (uint08 *)(Header + 1);
    switch(Compression)
    {
        case SPL_COMP_RLE:
            Size = RLE_CompressBMP(Image->Buffer, Image->Width,
                                    Image->Height, Image->LineWidth, Splash);
            if(Size <= 0)
                THROW_MSG(FAIL, "The selected compression scheme not perfoming well; please use Uncompressed compression type.");
            break;

        case SPL_COMP_RLE1:
            Size = RLE_CompressBMPSpl(Image->Buffer, Image->Width,
                                    Image->Height, Image->LineWidth, Splash);
            if(Size <= 0)
                THROW_MSG(FAIL, "The selected compression scheme not performing well; please use Uncompressed compression type.");
            break;

        case SPL_COMP_NONE:
            Size = Image2SplashUncompressed(Image, Splash);
            break;

        default:
        case SPL_COMP_AUTO:
            Size = RLE_CompressBMPSpl(Image->Buffer, Image->Width,
                                    Image->Height, Image->LineWidth, Splash);
            if(Size <= 0)
            {
                Compression = SPL_COMP_NONE;
                Size = Image2SplashUncompressed(Image, Splash);
            }
            else
            {
                Compression = SPL_COMP_RLE1;
            }
            break;
    }

    memset(Header, 0, sizeof(*Header));
    memcpy(Header->Signature, "Spld", 4);
    Header->Image_width = Image->Width;
    Header->Image_height = Image->Height;
    Header->Compression = Compression;
    Header->Byte_count = Size;
    Header->IsLeftImage = 1;
    Header->Pixel_format = 1;
    Header->Subimg_offset = 0xFFFFFFFF;
    Header->Subimg_end = 0xFFFFFFFF;
    Header->ByteOrder = 1;

    return Size + sizeof(SPL_Header_t);
}

/**
 * Converts the given splash image to BMP image
 *
 * @param Splash Splash image
 * @param Image Converted Image
 *
 * @return FAIL,
 *
 */
int SPL_ConvSplashToImage(uint08 const *Splash, Image_t *Image)
{
    SPL_Header_t *Header;

    if(Splash == NULL)
        THROW(ERR_NULL_PTR);

    Header = (SPL_Header_t *)Splash;

    Splash = (uint08 *)(Header + 1); /* Go to end of header */

    if(memcmp(Header->Signature, "Spld", 4) != 0)
        THROW_MSG(FAIL, "Invalid Splash image");

    if(Image->Width < Header->Image_width)
        THROW_MSG(FAIL, "Splash image width is bigger than target image");

    switch(Header->Compression)
    {
        case SPL_COMP_RLE:
            if(RLE_DecompressBMP(Splash, Image->Buffer,
                                            Image->LineWidth) < 0)
                THROW_MSG(FAIL, "RLE Decompression failed");
            break;

        case SPL_COMP_RLE1:
            if(RLE_DecompressBMPSpl(Splash, Header->Image_width,
                                    Image->Buffer, Image->LineWidth) < 0)
                THROW_MSG(FAIL, "Special RLE Decompression failed");
            break;

        case SPL_COMP_NONE:
            Splash2ImageUncompressed(Splash, Image);
            break;

        default:
            THROW_MSG(ERR_NOT_SUPPORTED, "Un Supported Compression type");
            break;
    }
    return SUCCESS;
}

int SPL_GetSplashImageInfo(uint08 const *Splash, SPL_Info_t *Info)
{
    SPL_Header_t *Header;

    if(Splash == NULL || Info == NULL)
        THROW(ERR_NULL_PTR);

    Header = (SPL_Header_t *)Splash;

    Splash = (uint08 *)(Header + 1); /* Go to end of header */

    if(memcmp(Header->Signature, "Spld", 4) != 0)
        THROW_MSG(FAIL, "Invalid Splash image");

    Info->Width = Header->Image_width;
    Info->Height = Header->Image_height;
    Info->CompType = Header->Compression;

    return SUCCESS;
}

/**
 * Allocates memory for storing the splash image
 * @param Image Image to be stored
 *
 * @return Allocated memory for the splash image. Should be deallocated
 *         using free.
 */
uint08 *SPL_AllocSplash(int Width, int Height)
{

    return malloc((ALIGN_BYTES_NEXT(Width * BYTES_PER_PIXEL, 16)) * Height + sizeof(SPL_Header_t));
}

/**
 * Free the memory allocated by \SPL_AllocSplash()
 * @param Splash Pointer to the allocated memory
 *
 */
void SPL_Free(uint08 *Splash)
{
    free(Splash);
    Splash = NULL;
}
