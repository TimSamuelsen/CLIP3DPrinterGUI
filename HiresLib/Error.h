/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/
#ifndef ERROR_H_
#define ERROR_H_

/**
 * This module defines the error handling related definitions
 */

#include <stdio.h>

/* Current logging level */
#define CFG_DEBUG_LEVEL     3

#define DEBUG_LEVEL_ERR  1 /* Only log the error messages */
#define DEBUG_LEVEL_WRN  2 /* Log warning and error messages */
#define DEBUG_LEVEL_MSG  3 /* Log normal, warning and error messages */
#define DEBUG_LEVEL_NONE 0 /* Dont log any message */

#define PRINTF(...)         printf(__VA_ARGS__)

#define DEBUG_PRN(...)      PRINTF(__VA_ARGS__)

#if CFG_DEBUG_LEVEL >= DEBUG_LEVEL_MSG
#define DEBUG_MSG(...)      PRINTF(__VA_ARGS__)
#else
#define DEBUG_MSG(...)
#endif

#if CFG_DEBUG_LEVEL >= DEBUG_LEVEL_WRN
#define DEBUG_WRN(...)      PRINTF(__VA_ARGS__)
#else
#define DEBUG_WRN(...)
#endif

#if CFG_DEBUG_LEVEL >= DEBUG_LEVEL_ERR
#define DEBUG_ERR(...)      PRINTF(__VA_ARGS__), putchar('\n')
#else
#define DEBUG_ERR(...)      (void)0
#endif

/* Macro to be invoked on fatal error condition */
#define ERR_FATAL(...) do { DEBUG_ERR(__VA_ARGS__); exit(-1); } while(0)

/* Macro to get the file and line number string for the current line */
#define ERR_GET_STR(f, l)   ERR_GET_STR_(f, l)
#define ERR_GET_STR_(f, l)  f ":" #l " >> "
#define ERR_LOC_STR()       ERR_GET_STR(__FILE__,__LINE__)

/* Log the error message with format string and arguments */
#define ERR_LOG_MSG(...)    DEBUG_ERR(ERR_LOC_STR() __VA_ARGS__)

#define ERR_LOG_STR(...)    do { ERR_LOG_MSG(__VA_ARGS__); \
							sprintf(ERR_String, __VA_ARGS__); } while(0)

/* Get the last error string logged */
#define GET_ERR_STR()		((char const *)ERR_String)

/* Begin the try block */
#define TRY_BEGIN()			do { int try_error_code__ = SUCCESS;

/* End the try block, error variable will get the error code */
#define TRY_END(error)		goto try_end__; try_end__: \
							error = try_error_code__; } while(0)

/* Throw error code inside try block */
#define TRY_THROW(error)    do { \
							ERR_LOG_MSG(#error); \
							try_error_code__ = (error); \
							goto try_end__; \
							} while(0)

/* Throw error code with error string inside try block */
#define TRY_THROW_MSG(error,...) do { \
									ERR_LOG_STR(__VA_ARGS__); \
									try_error_code__ = (error); \
									goto try_end__; } while(0)

/* Execute the given expression and re-throw the error - if any */
#define TRY_TRY(x)			do {\
							int throw_if_code__ = (x);\
							if(throw_if_code__ < 0)\
								TRY_THROW(throw_if_code__);\
							} while(0)

/* Throw error code outside try block */
#define THROW(x)            do { \
							TRY_BLOCK_CHK(); \
                            ERR_LOG_MSG(#x); \
                            return (x); \
                            } while(0)

/* Throw error with error string outside try block */
#define THROW_MSG(x,...)	do {	\
							TRY_BLOCK_CHK(); \
							ERR_LOG_STR(__VA_ARGS__);  \
							return (x); \
							} while(0)

/* Throw error code without logging any error string */
#define THROW_S(x)          do { \
							TRY_BLOCK_CHK(); \
							return (x);\
							} while(0)

/* Execute the given expression and re-throw the error - if any */
#define TRY(x)             do { \
                           int e__ = (x); \
						   TRY_BLOCK_CHK(); \
                           if(e__ < 0) { \
                               ERR_LOG_MSG(#x); \
                               return (e__); \
                           } \
                           } while(0)

/* Enable this to check if THROW() is used inside TRY_BLOCK.
 * Disable before releasing the code */
#if 0
/* Declare an array in the same name as the error code used in TRY block */
extern int try_error_code__[1];
/* Inside a TRY block this variable is integer and hence will
 * generate compiler error */
#define TRY_BLOCK_CHK()		if((*try_error_code__) != 0) return 0
#else
#define TRY_BLOCK_CHK()
#endif

/** All error codes */
typedef enum
{
    PASS                 =  0,  /* No error */
    SUCCESS              =  0,  /* No error */
    FAIL                 = -1,  /* General failure */
    ERR_OUT_OF_RESOURCE  = -2,  /* Out of resource (memory,sem,...) */
    ERR_INVALID_PARAM    = -3,  /* Invalid parameter */
    ERR_NULL_PTR         = -4,  /* Null pointer encountered */
    ERR_NOT_INITIALIZED  = -5,  /* System/Module not initialized */
    ERR_DEVICE_FAIL      = -6,  /* Device failure */
    ERR_DEVICE_BUSY      = -7,  /* Device busy */
    ERR_FORMAT_ERROR     = -8,  /* Format of the data is incorrect */
    ERR_TIMEOUT          = -9,  /* Device timed out */
    ERR_NOT_SUPPORTED    = -10, /* Unsupported command */
    ERR_NOT_FOUND        = -11, /* Item not found */
	ERR_ABORT			 = -12  /* Operation aborted */
} ErrorCode_t;

/* Memory for storing last error string */
extern char ERR_String[];

#endif /* ERROR_H_ */
