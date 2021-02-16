/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/

/**
 * This module provides RLE compression related APIs
 */

#include "common.h"
#include "error.h"
#include "compress.h"

static uint08 *AddCount(uint16 Count, uint08 *Out)
{
	if(Count < 128)
	{
		*Out++ = Count;
	}
	else
	{
		*Out++ = Count | 0x80;
		*Out++ = (Count >> 7) & 0xFF;
	}
	return Out;
}

static uint08 *AddPixel(uint08 const *Pix, uint08 *Out)
{
    *Out++ = Pix[2];
    *Out++ = Pix[0];
	*Out++ = Pix[1];

	return Out;
}

static uint08 *EncodePix(uint08 Repeat, uint08 const *Line, uint16 Count,
                            uint08 *Out, uint08 *EndPtr)
{
	uint16 i;

    if(Out == NULL)
        return NULL;

	if(Count > 0)
	{
		if(Repeat == 0 && Count == 1)
		{
			Repeat = 1;
		}

		if(Repeat == 2)
		{
            if(Out + 4 > EndPtr)
                return NULL;

			*Out++ = 0;
			*Out++ = 1;
			Out = AddCount(Count, Out);
		}
		else if(Repeat == 1)
		{
            if(Out + 5 > EndPtr)
                return NULL;

            Out = AddCount(Count, Out);
			Out = AddPixel(&Line[0], Out);
		}
		else
		{
            if(Out + Count*3+3 > EndPtr)
                return NULL;

            *Out++ = 0;
			Out = AddCount(Count, Out);
			Count *= 3;
			for(i = 0 ; i < Count; i += 3)
			{
				Out = AddPixel(&Line[i], Out);
			}
		}
	}
	return Out;
}

static uint16 FindRepeat(uint08 const *Line, uint16 x, uint16 Width)
{
	uint16 i = x * 3;
	uint32 Old = PARSE_WORD24_LE(Line + i);
	uint32 New;

	Width *= 3;

	while(i < Width)
	{
		i += 3;
		New = PARSE_WORD24_LE(Line + i);
		if(Old != New)
			break;
	}

	return i/3 - x;
}

static uint16 FindRepeat255(uint08 const *Line, uint16 x, uint16 Width)
{
	int i = 0;
	int Size;
	uint32 New;
	uint32 Old;

	Size = Width - x;
	Line += x * 3;
	Old = PARSE_WORD24_LE(Line);

	if(Size > 255)
		Size = 255;

	while(i < Size)
	{
		i++;
		Line += 3;
		New = PARSE_WORD24_LE(Line);
		if(Old != New)
			break;
	}

	return i;
}


static uint16 FindCopy(uint08 const *Line1, uint08 const *Line2, uint16 x, uint16 Width)
{
	uint16 i = x * 3;

	if(Line1 != NULL && Line2 != NULL)
	{
		Width *= 3;
		while(i < Width)
		{
			if(Line1[i] != Line2[i])
				break;
			i++;
		}
	}

	return i/3 - x;

}

int RLE_CompressBMP(uint08 const *Input, uint16 Width, uint16 Height, 
											uint16 FullWidth, uint08 *Output)
{
	uint08 const *Line = Input;
	uint16 x;
	uint16 y;
	uint16 Raw;
	uint08 *OutStart = Output;
	uint08 *OutPtrEnd = Output + Width * Height * 3;
	uint08 const *CopyPtr;
	uint08 WordAlign;

	for(y = 0; y < Height; y++)
	{
		x = 0;
		Raw = 0;
		while(x < Width)
		{
			uint16 Repeat = FindRepeat255(Line, x, Width);

			if(Repeat > 1 || Raw == 255)
			{
				if(Raw)
				{
					CopyPtr = Line + (x - Raw) * 3;
					if(Raw == 1)
					{
						*Output++ = 1;
                        *Output++ = CopyPtr[2];
                        *Output++ = CopyPtr[0];
						*Output++ = CopyPtr[1];
					}
					else if (Raw > 1)
					{
						*Output++ = 0;
						*Output++ = Raw;
						while(Raw--)
						{
                            *Output++ = CopyPtr[2];
                            *Output++ = CopyPtr[0];
							*Output++ = CopyPtr[1];
							CopyPtr += 3;
						}
					}
				}

				if(Repeat > 1)
				{
					CopyPtr = Line + x * 3;
					*Output++ = Repeat;
                    *Output++ = CopyPtr[2];
                    *Output++ = CopyPtr[0];
					*Output++ = CopyPtr[1];

					x += Repeat;
				}

				Raw = 0;
			}
			else
			{
				x++;
				Raw++;
			}
			if(Output >= OutPtrEnd)
				return 0;
		}

		CopyPtr = Line + (x - Raw) * 3;
		if(Raw == 1)
		{
			*Output++ = 1;
            *Output++ = CopyPtr[2];
            *Output++ = CopyPtr[0];
			*Output++ = CopyPtr[1];
		}
		else if (Raw > 1)
		{
			*Output++ = 0;
			*Output++ = Raw;
			while(Raw--)
			{
                *Output++ = CopyPtr[2];
                *Output++ = CopyPtr[0];
				*Output++ = CopyPtr[1];
				CopyPtr += 3;
			}
		}

		if(y == Height - 1)
			break;

		/* End of line */
		*Output++ = 0;
		*Output++ = 0;

		/* Next line should start in 4 byte boundary */
		WordAlign = (4 - ((Output - OutStart) & 3)) & 3;

		while(WordAlign--)
		{
			*Output++ = 0;
		}

		if(Output >= OutPtrEnd)
			return 0;

		Line = Line + FullWidth;
	}

	*Output++ = 0;
	*Output++ = 1;

	WordAlign = (16 - ((Output - OutStart) & 0xF)) & 0xF;

	while(WordAlign--)
	{
		*Output++ = 0;
	}

	return Output - OutStart;
}

int RLE_CompressBMPSpl(uint08 const *Input, uint16 Width, uint16 Height, 
											uint16 FullWidth, uint08 *Output)
{
	uint08 const *Line = Input;
	uint08 const *PrevLine = NULL;
	uint16 x;
	uint16 y;
	uint16 Raw;
	uint08 *OutStart = Output;
    uint08 *EndPtr = Output + Width * Height * 3;

	for(y = 0; y < Height; y++)
	{
		x = 0;
		Raw = 0;
		while(x < Width)
		{
			uint16 Repeat = FindRepeat(Line, x, Width);
			uint16 Copy = FindCopy(PrevLine, Line, x, Width);

			if(Copy > 0 && Copy >= Repeat)
			{
				if(Raw)
                    Output = EncodePix(0, Line + (x - Raw)*3, Raw,
                                                    Output, EndPtr);

                Output = EncodePix(2, Line + x, Copy, Output, EndPtr);
				x += Copy;
				Raw = 0;
			}
			else if(Repeat > 1)
			{
				if(Raw)
                    Output = EncodePix(0, Line + (x - Raw)*3, Raw, Output,
                                                                    EndPtr);

                Output = EncodePix(1, Line + x * 3, Repeat, Output, EndPtr);
				x += Repeat;
				Raw = 0;
			}
			else
			{
				x++;
				Raw++;
			}
            if(Output == NULL)
				return 0;
		}
		if(Raw)
            Output = EncodePix(0, Line + (x - Raw)*3, Raw, Output, EndPtr);

        if(Output == NULL)
            return 0;

		PrevLine = Line;
		Line = Line + FullWidth;
	}

	*Output++ = 0;
	*Output++ = 1;
	*Output++ = 0;

	return Output - OutStart;
}

/**
 * Special RLE decompression function.
 * Support repeat pixel, raw pixel, copy pixel from previous line
 *
 * InPtr - Compressed data input
 * InputWidth - Number of bytes in image line
 * OutPtr - Uncompressed data output
 * OutputWidth - Number of bytes in display line
 */
int RLE_DecompressBMPSpl(uint08 const *InPtr, uint16 InputWidth, 
										uint08 *OutPtr, uint16 OutputWidth)
{
	uint08 *CurLine = OutPtr;
	uint08 *PrevLine = OutPtr;
	uint16 LineIndex = 0;
	uint16 Count;

	InputWidth *= 3;

	while(CurLine != NULL)
	{
		Count = *InPtr++;
		if(Count == 0)
		{
			Count = *InPtr++;
			switch(Count)
			{
				case 0: /* End of line */
					if(LineIndex != 0)
						LineIndex = InputWidth;
					break;
				case 1:
					Count = *InPtr++;
					if(Count == 0) /* End of image */
					{
						CurLine = NULL; 
					}
					else /* Copy previous line */
					{
						if(Count & 0x80)
						{
							Count = (Count & 0x7F) | (*InPtr++ << 7);
						}

						Count *= 3;
						while(Count--)
						{
							CurLine[LineIndex] = PrevLine[LineIndex];
							LineIndex++;
						}
					}
					break;
				default: /* Copy following raw pixels */
					if(Count & 0x80)
					{
						Count = (Count & 0x7F) | (*InPtr++ << 7);
					}

					while(Count--)
					{
                        CurLine[LineIndex + 0] = InPtr[1];
						CurLine[LineIndex + 1] = InPtr[2];
                        CurLine[LineIndex + 2] = InPtr[0];
						InPtr += 3;
						LineIndex += 3;
					}
			}
		}
		else /* Repeat one pixel */
		{
			if(Count & 0x80)
			{
				Count = (Count & 0x7F) | (*InPtr++ << 7);
			}

			while(Count--)
			{
                CurLine[LineIndex + 0] = InPtr[1];
				CurLine[LineIndex + 1] = InPtr[2];
                CurLine[LineIndex + 2] = InPtr[0];
				LineIndex += 3;
			}
			InPtr += 3;
		}

		if(LineIndex >= InputWidth)
		{
			OutPtr += OutputWidth;
            PrevLine = CurLine;
			CurLine = OutPtr;
			LineIndex = 0;
		}
	}

	return SUCCESS;
}

int RLE_DecompressBMP(uint08 const *InPtr, uint08 *OutPtr, uint16 ImageWidth)
/**
 * RLE decompression function.
 * Support repeat pixel, raw pixel
 *
 * InPtr - Compressed data input
 * OutPtr - Uncompressed data output
 * OutputWidth - Number of bytes in display line
 */
{
	uint08 *NextLine = OutPtr + ImageWidth;
	uint08 Count;

	while(InPtr != NULL)
	{
		Count = *InPtr++;
		if(Count == 0)
		{
			Count = *InPtr++;
			switch(Count)
			{
				case 0: /* End of line */
					InPtr = (uint08 *)ALIGN_BYTES_NEXT((uint32)InPtr, 4);
					OutPtr = NextLine;
					NextLine = NextLine + ImageWidth; 
					break;
				case 1: /* End of image */
					InPtr = NULL;
					break;
				default: /* Copy following raw pixels */
					while(Count--)
					{
                        OutPtr[0] = InPtr[1];
						OutPtr[1] = InPtr[2];
                        OutPtr[2] = InPtr[0];
						InPtr += 3;
						OutPtr += 3;
					}
			}
		}
		else /* Repeat one pixel */
		{
			while(Count--)
			{
                OutPtr[0] = InPtr[1];
				OutPtr[1] = InPtr[2];
                OutPtr[2] = InPtr[0];
				OutPtr += 3;
			}
			InPtr += 3;
		}
	}

	return PASS;
}
