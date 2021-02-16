/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/
#ifndef SPLASH_H_
#define SPLASH_H_

/**
 * This module provides the splash image related functions
 */

#include "common.h"
#include "Pattern.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Enumeration of all the compression types */
typedef enum
{
    SPL_COMP_NONE,     /**< Uncompressed */
    SPL_COMP_RLE,      /**< RLE Compression */
    SPL_COMP_RLE1,     /**< Special RLE Compression */
	SPL_COMP_AUTO,     /**< Auto select the compression type */
    SPL_NUM_COMP_TYPES 
} SPL_Compression_t;

/** Splash info structure */
typedef struct
{
	int Width;                  /**< Width of the splash image */
	int Height;                 /**< Height of the splash image */
	SPL_Compression_t CompType; /**< Compression type used */
} SPL_Info_t;

int SPL_ConvImageToSplash(Image_t const *Image, SPL_Compression_t Compression, 
															uint08 *Splash);
int SPL_ConvSplashToImage(uint08 const *Splash, Image_t *Image);
int SPL_GetSplashImageInfo(uint08 const *Splash, SPL_Info_t *Info);
uint08 *SPL_AllocSplash(int Width, int Height);
void SPL_Free(uint08 *Splash);

#ifdef __cplusplus
}
#endif
#endif /* SPLASH_H_ */
