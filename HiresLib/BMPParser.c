/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/

/**
*
* @file BMPParser.c
*
* @brief This module handles parsing BMP Image content
*
* @note:
*
*/

#include <time.h>
#include <string.h>
#include "Common.h"
#include "Error.h"

#include "BMPParser.h"

/******************************* MACROS **************************************/
#define BMP_FILE_HEADER_SIZE        14
#define BMP_DIB_HEADER_SIZE         40
#define BMP_DIB_HEADER_SIZE1        108

#define BMP_SIGNATURE       MAKE_WORD16('M', 'B')

#define BMP_DATA_SKIP(Skip, Data, Index)  (Index) += (Skip)

#define BMP_DATA_PARSE1(Value, Data, Index)  \
                        do { \
                            (Value) = (Data)[Index]; \
                            (Index) += 1; \
                        } while(0)

#define BMP_DATA_PARSE2(Value, Data, Index)  \
                        do { \
                            (Value) = PARSE_WORD16_LE(Data + Index); \
                            (Index)+=2; \
                        } while(0)

#define BMP_DATA_PARSE4(Value, Data, Index)  \
                        do { \
                            (Value) = PARSE_WORD32_LE(Data + Index); \
                            (Index)+=4; \
                        } while(0)

#define BMP_ConvertEndianLE(Arr, Size)  (SUCCESS)

#define GET_LINE_BYTES(Image)   (ALIGN_BYTES_NEXT((Image)->Width * \
                                                (Image)->BitDepth, 32)/8)

/**************************** LOCAL TYPES ************************************/

/** BMP Header Information */
typedef struct
{
    uint32 Width;           /* Width of the BMP Image */
    uint32 Height;
    uint16 BitDepth;
    uint32 Compression;
    uint32 PixelOffset;
    uint32 PaletteOffset;
    uint32 PaletteSize;
    uint32 FileSize;
} BMP_ImageHeader_t;

/** BMP Image merging options */
typedef struct
{
    Image_t *Image;
    uint32 Mask;
    uint08 Shift;
} BMP_MergeOptions_t;

/********************* LOCAL FUNCTION PROTOTYPES *****************************/
static int BMP_ParseHeader(BMP_ImageHeader_t *Image, uint8 *Data,
                                                            uint32 DataSize);
static int BMP_PutData(BMP_DataFunc_t *PutData, void *DataParam,
                                                    uint32 Value, uint8 Size);
static int ReadFile(void *Param, uint8 *Data, uint32 Size);

static int WriteFile(void *Param, uint8 *Data, uint32 Size);

static int ReadBuffer(void *Param, int X, int Y,
                                uint8 *PixValue, uint32 Count);
static int WriteBuffer(void *Param, int X, int Y,
                                uint8 *PixValue, uint32 Count);
/****************************** VARIABLES *************************************/

/************************* FUNCTION DEFINITIONS********************************/

/**
*  This function parses the BMP image data received from GetData() function
*  and draws the pixels using DrawPixels() function
*
*  @param GetData - Function pointer for receiving BMP Data
*  @param DataParam - Parameter to be passed for GetData function
*  @param DrawPixels - Function pointer for drawing pixels
*  @param DrawParam - Parameter to be passed for DrawPixels function
*  @param InduxOnly - Only pass the palette index to the DrawPixels function
*                     If true the passed parameter one pixel per byte
*                     other wise it is one pixel per three bytes
*                     (the order is RGB)
*  return SUCCESS, FAIL
*/
int BMP_ParseImage(BMP_DataFunc_t *GetData, void *DataParam,
                                BMP_PixelFunc_t *DrawPixels, void *DrawParam,
                                uint8 OutBitDepth)
{
    int Error = SUCCESS;
    uint8 Header[BMP_FILE_HEADER_SIZE + BMP_DIB_HEADER_SIZE];
    BMP_ImageHeader_t Image;
    uint32 ReadIndex;
    uint8 *LineData = NULL;
    uint8 *LineOutput = NULL;
    uint32 *Palette = NULL;
    uint32 LineWidth;
    uint32 OutLineWidth;
    int Shift;
    unsigned PixelsPerByte;
    unsigned x;
    unsigned y;
    unsigned i;
    unsigned j;

    TRY_BEGIN()
    {
        if(GetData(DataParam, Header, sizeof(Header)))
            TRY_THROW_MSG(FAIL, "Error while reading header");

        ReadIndex = sizeof(Header);

        if(BMP_ParseHeader(&Image, Header, sizeof(Header)))
            TRY_THROW(FAIL);

#if 0
        DEBUG_MSG("BMP Parser\n");
        DEBUG_MSG("Image FileSize  = %d\n", Image.FileSize);
        DEBUG_MSG("Image Width     = %d\n", Image.Width);
        DEBUG_MSG("Image Height    = %d\n", Image.Height);
        DEBUG_MSG("Image BitDepth  = %d\n", Image.BitDepth);
        DEBUG_MSG("Output BitDepth = %d\n", OutBitDepth);
#endif

        if(GetData(DataParam, NULL, Image.PaletteOffset - ReadIndex))
            TRY_THROW_MSG(FAIL, "Error while reading palette");

        ReadIndex = Image.PaletteOffset;

        if(Image.BitDepth != 1 && Image.BitDepth != 2 && Image.BitDepth != 4 &&
                    Image.BitDepth != 8 && Image.BitDepth != 16 &&
                    Image.BitDepth != 24 && Image.BitDepth != 32)
            TRY_THROW_MSG(ERR_NOT_SUPPORTED,
                                "Error un supported bit depth");

        if(Image.BitDepth <= 8)
        {
            if(Image.PixelOffset - ReadIndex < Image.PaletteSize * 4)
                TRY_THROW_MSG(ERR_FORMAT_ERROR, "BMP File format error");

            Palette = (uint32 *)malloc(Image.PaletteSize * 4);
            if(Palette == NULL)
                TRY_THROW_MSG(ERR_OUT_OF_RESOURCE,
                                    "Unable to allocate memory for palette");

            if(GetData(DataParam, (uint8*)Palette, Image.PaletteSize * 4))
                TRY_THROW_MSG(FAIL, "Error while reading palette");

            if(OutBitDepth == 16)
            {
                for(i = 0; i < Image.PaletteSize; i++)
                {
                    uint32 color = Palette[i];
                    Palette[i] = ((color & 0x0000F8) >> 3) |
                                 ((color & 0x00FC00) >> 5) |
                                 ((color & 0xF80000) >> 8) ;
                }
            }
            else if(OutBitDepth <= 8)
            {
                for(i = 0; i < Image.PaletteSize; i++)
                {
                    uint32 color = Palette[i];
                    Palette[i] = (BYTE0(color) |
                                  BYTE1(color) |
                                  BYTE2(color) ) >> (8 - OutBitDepth);
                }
            }

            ReadIndex += Image.PaletteSize * 4;
        }

        if(Image.PixelOffset < ReadIndex)
            TRY_THROW_MSG(ERR_FORMAT_ERROR, "BMP File format error");

        /* Ignore the Palette */
        if(GetData(DataParam, NULL, Image.PixelOffset - ReadIndex))
            TRY_THROW_MSG(FAIL, "Error while reading palette");

        LineWidth = GET_LINE_BYTES(&Image);
        LineData = (uint8 *)malloc(LineWidth);
        if(LineData == NULL)
            TRY_THROW(ERR_OUT_OF_RESOURCE);

        if(OutBitDepth == 32)
            OutLineWidth = 4 * Image.Width;
        else if(OutBitDepth == 24)
            OutLineWidth = 3 * Image.Width;
        else if(OutBitDepth == 16)
            OutLineWidth = 2 * Image.Width;
        else
            OutLineWidth = Image.Width;

        LineOutput = (uint8 *)malloc(OutLineWidth);
        if(LineOutput == NULL)
            TRY_THROW(ERR_OUT_OF_RESOURCE);

        if(Image.BitDepth <= 8)
        {
            Shift = 8 - Image.BitDepth;
            PixelsPerByte = 8 / Image.BitDepth;
            for(y = Image.Height; y-- > 0;)
            {
                if(GetData(DataParam, LineData, LineWidth))
                    TRY_THROW_MSG(FAIL, "Error while reading pixels");

                if(OutBitDepth <= 8)
                {
                    for(x = 0, i = 0; i < LineWidth; i++)
                    {
                        uint8 PixData = LineData[i];
                        for(j = 0; j < PixelsPerByte && x < Image.Width; j++, x++)
                        {
                            LineOutput[x] = Palette[PixData >> Shift];
                            PixData <<= Image.BitDepth;
                        }
                    }
                }
                else if(OutBitDepth == 32)
                {
                    for(x = 0, i = 0; i < LineWidth; i++)
                    {
                        uint8 PixData = LineData[i];
                        for(j = 0; j < PixelsPerByte && x < OutLineWidth; j++)
                        {
                            uint32 Color = Palette[PixData >> Shift];
                            *(uint32 *)(LineOutput + x) = Color;
                            x += 4;
                            PixData <<= Image.BitDepth;
                        }
                    }
                }
                else if(OutBitDepth == 24)
                {
                    for(x = 0, i = 0; i < LineWidth; i++)
                    {
                        uint8 PixData = LineData[i];
                        for(j = 0; j < PixelsPerByte && x < OutLineWidth; j++)
                        {
                            uint32 Color = Palette[PixData >> Shift];
                            LineOutput[x++] = BYTE0(Color);
                            LineOutput[x++] = BYTE1(Color);
                            LineOutput[x++] = BYTE2(Color);
                            PixData <<= Image.BitDepth;
                        }
                    }
                }
                else if(OutBitDepth == 16)
                {
                    for(x = 0, i = 0; i < LineWidth; i++)
                    {
                        uint8 PixData = LineData[i];
                        for(j = 0; j < PixelsPerByte && x < OutLineWidth; j++)
                        {
                            uint32 Color = Palette[PixData >> Shift];
                            LineOutput[x++] = BYTE0(Color);
                            LineOutput[x++] = BYTE1(Color);
                            PixData <<= Image.BitDepth;
                        }
                    }
                }

                if(DrawPixels(DrawParam, 0, y, LineOutput, Image.Width))
                    TRY_THROW_MSG(FAIL, "Error while drawing pixel");

            }
        }
        else if(Image.BitDepth == 32 || Image.BitDepth == 24)
        {
            uint8 *Ptr;
			int Inc;

            if(Image.BitDepth == OutBitDepth)
                Ptr = LineData;
            else
                Ptr = LineOutput;

			Inc = Image.BitDepth/8;

            Shift = 8 - OutBitDepth;

            for(y = Image.Height; y-- > 0;)
            {
                if(GetData(DataParam, LineData, LineWidth))
                    TRY_THROW_MSG(FAIL, "Error while reading pixel");

                if(OutBitDepth <= 8)
                {
                    unsigned x1;
                    unsigned x2;
                    for(x1 = 0, x2 = 0; x1 < Image.Width; x1++, x2+=Inc)
                    {
                        LineOutput[x1] = (LineData[x2] | LineData[x2 + 1] |
                                                LineData[x2 + 2]) >> Shift;
                    }
                }
                else if(OutBitDepth == 16)
                {
                    unsigned x1;
                    unsigned x2;
                    for(x1 = 0, x2 = 0; x1 < Image.Width; x1++, x2+=Inc)
                    {
                        uint32 Color = LineData[x2] | (LineData[x2 + 1] << 8) |
                                        (LineData[x2 + 2] << 16);
                        ((uint16 *)LineOutput)[x1] = ((Color & 0xF8  ) >> 3) |
                                                     ((Color & 0xFC00) >> 5) |
                                                     ((Color & 0xF80000) >> 8);
                    }
                }
                else if(OutBitDepth == 24)
                {
                    unsigned x1;
                    unsigned x2;
                    for(x1 = 0, x2 = 0; x1 < Image.Width; x1+=3, x2+=Inc)
                    {
                        LineOutput[x1  ] = LineData[x2  ];
                        LineOutput[x1+1] = LineData[x2+1];
                        LineOutput[x1+2] = LineData[x2+2];
                    }
                }

                if(DrawPixels(DrawParam, 0, y, Ptr, Image.Width))
                    TRY_THROW_MSG(FAIL, "Error while drawing pixel");
            }
        }
        else if(Image.BitDepth == 16)
        {
            uint8 *Ptr;

            if(OutBitDepth == 16)
                Ptr = LineData;
            else
                Ptr = LineOutput;

            for(y = Image.Height; y-- > 0;)
            {
                if(GetData(DataParam, LineData, LineWidth))
                    TRY_THROW_MSG(FAIL, "Error while reading pixel");

                if(OutBitDepth <= 8)
                {
                    unsigned x1;
                    for(x1 = 0; x1 < Image.Width; x1++)
                    {
                        uint16 color = ((uint16 *)LineData)[x1];
                        color = ((color << 3) & 0xF8) |
                                ((color >> 3) & 0xFC) |
                                ((color >> 8) & 0xF8);
                        Ptr[x1] = color;
                    }
                }
                else if(OutBitDepth == 24)
                {
                    unsigned x1;
                    unsigned x2;

                    for(x1 = x2 = 0; x1 < Image.Width; x1++, x2 += 3)
                    {
                        uint16 color = ((uint16 *)LineData)[x1];

                        Ptr[x2 + 0] = (color << 3) & 0xF8;
                        Ptr[x2 + 1] = (color >> 3) & 0xFC;
                        Ptr[x2 + 2] = (color >> 8) & 0xF8;
                    }
                }
                else if(OutBitDepth == 32)
                {
                    unsigned x1;

                    for(x1 = 0; x1 < Image.Width; x1++)
                    {
                        uint16 color = ((uint16 *)LineData)[x1];

                        ((uint32 *)Ptr)[x1] = ((color << 3) & 0x0000F8) |
                                              ((color << 5) & 0x00FC00) |
                                              ((color << 8) & 0xF80000) ;
                    }
                }

                if(DrawPixels(DrawParam, 0, y, Ptr, Image.Width))
                    TRY_THROW_MSG(FAIL, "Error while drawing pixel");
            }
        }
    }
    TRY_END(Error);

    free(Palette);
    free(LineData);
    free(LineOutput);

    return Error;
}

/**
 * Allocate memory for BMP data structure for the given image dimension
 *
 * @param Width Width of the image in pixels
 * @param Height Height of the image in pixels
 * @param BitDepth Bits per pixel
 *
 * @return SUCCESS/FAIL
 */
Image_t *BMP_AllocImage(int Width, int Height, uint08 BitDepth)
{
    Image_t *Image;
    int BytesPerPix = DIV_CEIL(BitDepth, 8);
    int LineWidth = ALIGN_BYTES_NEXT(Width * BytesPerPix, 4);

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

    return Image;
}

/**
 * Free the memory allocated by BMP_AllocImage();
 *
 * @param Image
 */
void BMP_FreeImage(Image_t *Image)
{
    free(Image);
}

/**
* This function returns the BMP file size of the given image
* @param Image - Image structure
*
*  return Image file size
*/
uint32 BMP_BMPFileSize(Image_t *Image)
{
    uint32 NumColors = (Image->BitDepth <= 8) ? POW_OF_2(Image->BitDepth) : 0;
    uint32 PixOffset = NumColors * 4 + BMP_FILE_HEADER_SIZE +
                                        BMP_DIB_HEADER_SIZE;
    uint32 DataSize = GET_LINE_BYTES(Image) * Image->Height;
    uint32 FileSize =  DataSize + PixOffset;

    return FileSize;
}

/**
*  This function stores the image formation received from GetPixels function and
*  stores it in BMP file format using PutData function
*
*  @param Image - Image structure
*  @param PutData - Function pointer for store the BMP file data
*  @param DataParam - Parameter to be passed for PutData function
*  @param GetPixels - Function pointer to get the image pixels
*  @param PixelParam - Parameter to be passed for GetPixels function
*
*  return SUCCESS, FAIL
*/
int BMP_StoreImage(Image_t *Image, BMP_DataFunc_t *PutData,
                            void *DataParam, BMP_PixelFunc_t *GetPixels,
                            void *PixelParam)
{
    int Error = SUCCESS;
    uint08 BitDepth = Image->BitDepth;
    uint32 NumColors = (BitDepth <= 8) ? POW_OF_2(BitDepth) : 0;
    int HeaderSize = BitDepth == 16 ? BMP_DIB_HEADER_SIZE1 : BMP_DIB_HEADER_SIZE;
    uint32 PixOffset = NumColors * 4 + BMP_FILE_HEADER_SIZE + HeaderSize;
    uint32 DataSize = GET_LINE_BYTES(Image) * Image->Height;
    uint32 FileSize =  DataSize + PixOffset;
    uint8 *Buffer = NULL;
    uint8 *LineOut = NULL;
    unsigned x;
    unsigned y;
    unsigned i;
    unsigned j;
    unsigned Mask;
    unsigned LineWidth;
    unsigned PixelsPerByte;

	if(BitDepth == 0)
		THROW(ERR_INVALID_PARAM);
	else if(BitDepth == 1)
		BitDepth = 1;
	else if(BitDepth <= 4)
		BitDepth = 4;
	else if(BitDepth <= 8)
		BitDepth = 8;
	else if(BitDepth == 16)
		BitDepth = 16;
	else if(BitDepth == 24)
		BitDepth = 24;
	else if(BitDepth == 32)
		BitDepth = 32;
	else 
		THROW_MSG(ERR_INVALID_PARAM, "Bitdepth %d Invalid", BitDepth);

    TRY_BEGIN()
    {
        if(BMP_PutData(PutData, DataParam, BMP_SIGNATURE, 2))
            TRY_THROW(FAIL);
        if(BMP_PutData(PutData, DataParam, FileSize, 4))
            TRY_THROW(FAIL);
        if(BMP_PutData(PutData, DataParam, 0, 4)) /* Reserved */
            TRY_THROW(FAIL);
        if(BMP_PutData(PutData, DataParam, PixOffset, 4))
            TRY_THROW(FAIL);
        if(BMP_PutData(PutData, DataParam, HeaderSize, 4))
            TRY_THROW(FAIL);
        if(BMP_PutData(PutData, DataParam, Image->Width, 4))
            TRY_THROW(FAIL);
        if(BMP_PutData(PutData, DataParam, Image->Height, 4))
            TRY_THROW(FAIL);
        if(BMP_PutData(PutData, DataParam, 1, 2)) /* Number of color planes */
            TRY_THROW(FAIL);
        if(BMP_PutData(PutData, DataParam, Image->BitDepth, 2))
            TRY_THROW(FAIL);
        /* Compression = None */
        if(BMP_PutData(PutData, DataParam, Image->BitDepth == 16 ? 3 : 0, 4))
            TRY_THROW(FAIL);
        if(BMP_PutData(PutData, DataParam, DataSize, 4))
            TRY_THROW(FAIL);
        if(BMP_PutData(PutData, DataParam, 2835, 4)) /* H Res pix/meter */
            TRY_THROW(FAIL);
        if(BMP_PutData(PutData, DataParam, 2835, 4)) /* V Res pix/meter */
            TRY_THROW(FAIL);
        /* Palette Size */
        if(BMP_PutData(PutData, DataParam, NumColors, 4))
            TRY_THROW(FAIL);
        if(BMP_PutData(PutData, DataParam, 0, 4)) /* Important colors = All */
            TRY_THROW(FAIL);

        for(i = 0; i < NumColors; i++)
        {
            uint8 Index = i;
            uint8 Val;
            uint32  Color;
#if 0
            if(Image->ReversePalette)
                Index = NumColors - i - 1;
#endif
            Val = (0xFF * Index)/(NumColors-1);
            Color = MAKE_WORD32(255, Val, Val, Val);
            if(BMP_PutData(PutData, DataParam, Color, 4))
                TRY_THROW(FAIL);
        }

        if(Image->BitDepth == 16)
        {
            if(BMP_PutData(PutData, DataParam, 0xF800, 4))
                TRY_THROW(FAIL);

            if(BMP_PutData(PutData, DataParam, 0x07E0, 4))
                TRY_THROW(FAIL);

            if(BMP_PutData(PutData, DataParam, 0x001F, 4))
                TRY_THROW(FAIL);

            if(BMP_PutData(PutData, DataParam, 0, 4))
                TRY_THROW(FAIL);

            if(BMP_PutData(PutData, DataParam, 0x57696E20, 4))
                TRY_THROW(FAIL);

            for(i = 0; i < 12; i++)
            {
                if(BMP_PutData(PutData, DataParam, 0, 4))
                    TRY_THROW(FAIL);
            }
        }

        if(Image->BitDepth <= 8)
        {
            Buffer = (uint8 *)malloc(Image->Width);
            if(Buffer == NULL)
                TRY_THROW(ERR_OUT_OF_RESOURCE);
        }

        Mask = GEN_BIT_MASK(0, Image->BitDepth);
        LineWidth = GET_LINE_BYTES(Image);
        LineOut = (uint8 *)malloc(LineWidth);
        if(LineOut == NULL)
            TRY_THROW(ERR_OUT_OF_RESOURCE);

        PixelsPerByte = 8/Image->BitDepth;

        if(Image->BitDepth <= 8)
        {
            for(y = Image->Height; y-- > 0;)
            {
                if(GetPixels(PixelParam, 0, y, Buffer, Image->Width))
                    TRY_THROW(FAIL);

                for(i = 0, x = 0; x < LineWidth; x++)
                {
                    uint8 Byte = 0;
                    for(j = 0; j < PixelsPerByte; j++)
                    {
                        Byte <<= Image->BitDepth;
                        if(i < Image->Width)
                        {
                            Byte |= Buffer[i++] & Mask;
                        }
                    }
                    LineOut[x] = Byte;
                }
                if(PutData(DataParam, LineOut, LineWidth))
                    TRY_THROW_MSG(FAIL, "Error while drawing pixel");
            }
        }
        else
        {
            for(y = Image->Height; y-- > 0;)
            {
                if(GetPixels(PixelParam, 0, y, LineOut, Image->Width))
                    TRY_THROW(FAIL);

                if(PutData(DataParam, LineOut, LineWidth))
                    TRY_THROW_MSG(FAIL, "Error while drawing pixel");
            }
        }
    }
    TRY_END(Error);

    free(LineOut);
    free(Buffer);

    return Error;
}

/**
 * Load a BMP File to the Image_t data structure
 *
 * @param FileName BMP File to be loaded
 * @param Image Image data structure
 *
 * @return SUCCESS/ERR_NOT_FOUND/FAIL
 */
int BMP_LoadFromFile(const char *FileName, Image_t *Image)
{
    int Error = SUCCESS;
    FILE *fp = NULL;

    TRY_BEGIN()
    {
        if((fp = fopen(FileName, "rb")) == 0)
            TRY_THROW(ERR_NOT_FOUND);

        TRY_TRY(BMP_ParseImage(ReadFile, fp, WriteBuffer,
                                    Image, Image->BitDepth));
    }
    TRY_END(Error);

    if(fp)
        fclose(fp);

    return Error;
}

/**
 * Save the image data to a BMP file
 *
 * @param Image Image data structure
 * @param FileName BMP File name to be saved to
 *
 * @return SUCCESS/FAIL
 */
int BMP_SaveToFile(Image_t *Image, const char *FileName)
{
    int Error = SUCCESS;
    FILE *fp = NULL;

    TRY_BEGIN()
    {
        if((fp = fopen(FileName, "wb")) == 0)
            TRY_THROW(ERR_NOT_FOUND);

        TRY_TRY(BMP_StoreImage(Image, WriteFile, fp, ReadBuffer, Image));
    }
    TRY_END(Error);

    if(fp)
        fclose(fp);

    return Error;
}


/**
 * Read image information from a BMP file. This does not load the full file
 *
 * @param FileName BMP File name
 * @param [out] BMPHeader Data structure to store the image info
 *
 * @return SUCCESS/FAIL/ERR_NOT_FOUND
 */
int BMP_GetFileInfo(const char *FileName, Image_t *BMPHeader)
{
    uint8 Header[BMP_FILE_HEADER_SIZE + BMP_DIB_HEADER_SIZE];
    BMP_ImageHeader_t HeaderInfo;
    FILE *fp = fopen(FileName, "rb");
    int Error = SUCCESS;

    TRY_BEGIN()
    {
        if(fp == NULL)
            TRY_THROW(ERR_NOT_FOUND);

        if(fread(Header, 1, sizeof(Header), fp) <= 0)
            TRY_THROW(ERR_DEVICE_FAIL);

        if(BMP_ParseHeader(&HeaderInfo, Header, sizeof(Header)))
            TRY_THROW(FAIL);

        BMPHeader->Width = HeaderInfo.Width;
        BMPHeader->Height = HeaderInfo.Height;
        BMPHeader->BitDepth = HeaderInfo.BitDepth;
    }
    TRY_END(Error);

    if(fp)
    {
        fclose(fp);
    }

    return Error;
}

/****************************** LOCAL FUNCTIONS *******************************/
/**
*  This function parses the BMP header present in memroy
*
*  @param Image - The BMP Image header structure to be filled
*  @param Data - The pointer to the BMP header data
*  @param DataSize - The BMP Data size
*
*  return SUCCESS, FAIL
*/
static int BMP_ParseHeader(BMP_ImageHeader_t *Image, uint8 *Data,
                                                                uint32 DataSize)
{
    int Error = SUCCESS;
    uint32 Value;
    uint32 Index = 0;

    TRY_BEGIN()
    {
        if(Image == NULL || Data == NULL ||
                    DataSize < BMP_FILE_HEADER_SIZE + BMP_DIB_HEADER_SIZE)
            TRY_THROW(ERR_INVALID_PARAM);

        BMP_DATA_PARSE2(Value, Data, Index);
        if(Value  != BMP_SIGNATURE)
            TRY_THROW_MSG(ERR_FORMAT_ERROR,
                            "BMP File signature mismatch");

        BMP_DATA_PARSE4(Image->FileSize, Data, Index); /* File Size */

        BMP_DATA_SKIP(4, Data, Index);

        /* Offset to pixel data */
        BMP_DATA_PARSE4(Image->PixelOffset, Data, Index);
        if(Image->PixelOffset >= Image->FileSize)
           TRY_THROW_MSG(ERR_FORMAT_ERROR, "Pixel start error");

        BMP_DATA_PARSE4(Value, Data, Index); /* DIB Header Size */
        if(Value < BMP_DIB_HEADER_SIZE)
           TRY_THROW_MSG(ERR_FORMAT_ERROR, "DIB Header Size error");

        Image->PaletteOffset = Value + BMP_FILE_HEADER_SIZE;

        BMP_DATA_PARSE4(Image->Width, Data, Index);
        BMP_DATA_PARSE4(Image->Height, Data, Index);

        BMP_DATA_PARSE2(Value, Data, Index); /* Num Color planes */
        if(Value != 1)
           TRY_THROW_MSG(ERR_FORMAT_ERROR, "Color planes error");

        BMP_DATA_PARSE2(Image->BitDepth, Data, Index);

        BMP_DATA_PARSE4(Image->Compression, Data, Index);

        if(Image->Compression != 0 && Image->Compression != 3)
           TRY_THROW_MSG(ERR_NOT_SUPPORTED,
                                "Image compression not supported");

        BMP_DATA_SKIP(12, Data, Index);

        BMP_DATA_PARSE4(Image->PaletteSize, Data, Index);

        if(Image->PaletteSize == 0)
            Image->PaletteSize = 1 << Image->BitDepth;

        (void)Index;
    }
    TRY_END(Error);

    return Error;
}

/**
 * Callback function for reading the image  data from file
 *
 * @param Param Callback param (File pointer)
 * @param Data Pointer to store the read data
 * @param Size Number of byte to read
 *
 * @return SUCCESS/FAIL
 */
static int ReadFile(void *Param, uint8 *Data, uint32 Size)
{
    FILE *fp = (FILE *)Param;

    if(fp == NULL)
        return ERR_INVALID_PARAM;

    if(Data == NULL)
    {
        if(fseek(fp, Size, SEEK_CUR))
            return ERR_DEVICE_FAIL;
    }
    else
    {
        if(fread(Data, Size, 1, fp) != 1)
            return ERR_DEVICE_FAIL;
    }
    return SUCCESS;
}

/**
 * Callback function for writing the pixel data to buffer
 *
 * @param Param Callback param (Image pointer)
 * @param X X Pixel coordinate
 * @param Y Y Pixel coordinate
 * @param PixValue Pixel value array
 * @param Count Number of pixels in the array
 *
 * @return SUCCESS/FAIL
 */
static int WriteBuffer(void *Param, int X, int Y,
                                uint8 *PixValue, uint32 Count)
{
    Image_t *Image = (Image_t *)Param;
    int BytePerPix = Image->BitDepth/8;

    if(X >= Image->Width || Y >= Image->Height)
        return SUCCESS;

    if(X + Count > Image->Width)
        Count = Image->Width - X;

    memcpy(Image->Buffer + Y * Image->LineWidth + X * BytePerPix,
                                PixValue, Count * BytePerPix);
    return SUCCESS;
}


/**
 * Callback function for writing BMP data to file
 *
 * @param Param Callback param (File pointer)
 * @param Data Pointer to data buffer
 * @param Size Number of bytes in the buffer
 *
 * @return SUCCESS/FAIL
 */
static int WriteFile(void *Param, uint8 *Data, uint32 Size)
{
    FILE *fp = (FILE *)Param;

    if(fp == NULL)
        return ERR_INVALID_PARAM;

    if(Data == NULL)
    {
        if(fseek(fp, Size, SEEK_CUR))
            return ERR_DEVICE_FAIL;
    }
    else
    {
        if(fwrite(Data, Size, 1, fp) != 1)
            return ERR_DEVICE_FAIL;
    }

    return SUCCESS;
}

/**
 * Callback function for reading pixel data from image buffer
 *
 * @param Param Callback param (Image pointer)
 * @param X X Coordinate of the pixel
 * @param Y Y Coordinate of the pixel
 * @param PixValue Pixel value array
 * @param Count Number of pixels
 *
 * @return SUCCESS/FAIL
 */
static int ReadBuffer(void *Param, int X, int Y,
                                uint8 *PixValue, uint32 Count)
{
    Image_t *Image = (Image_t *)Param;
    int BytePerPix = DIV_CEIL(Image->BitDepth, 8);

    if(X >= Image->Width || Y >= Image->Height)
        return SUCCESS;

    if(X + Count > Image->Width)
        Count = Image->Width - X;

    memcpy(PixValue, Image->Buffer + Y * Image->LineWidth + X * BytePerPix,
                                Count * BytePerPix);
    return SUCCESS;
}


/**
 * Serialize the integer data to the output function
 *
 * @param PutData Function pointer for sending the data
 * @param DataParam Parameter to the callback function
 * @param Value Integer value
 * @param Size Number of bytes in the integer to be serialized
 *
 * @return SUCCESS/FAIL
 */
static int BMP_PutData(BMP_DataFunc_t *PutData, void *DataParam,
                                                    uint32 Value, uint8 Size)
{
    uint8 Data[4];
    uint8 i;
    for(i = 0; i < Size; i++)
    {
        Data[i] = Value & 0xFF;
        Value >>= 8;
    }
    return PutData(DataParam, Data, Size);
}

