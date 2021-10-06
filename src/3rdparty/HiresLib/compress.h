/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/
#ifndef COMPRESS_H_
#define COMPRESS_H_

/**
 * This module provides RLE compression related APIs
 */

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

int RLE_DecompressBMPSpl(uint08 const *CompData, uint16 InputWidth, uint08 *OutPtr,
                                                        uint16 OutputWidth);
int RLE_CompressBMPSpl(uint08 const *Input, uint16 Width, uint16 Height,
                                            uint16 FullWidth, uint08 *Output);

int RLE_CompressBMP(uint08 const *Input, uint16 Width, uint16 Height, 
											uint16 FullWidth, uint08 *Output);

int RLE_DecompressBMP(uint08 const *InPtr, uint08 *OutPtr, uint16 ImageWidth);

int RLE_TestCompression(void);

#ifdef __cplusplus
}
#endif
#endif /* COMPRESS_H_ */
