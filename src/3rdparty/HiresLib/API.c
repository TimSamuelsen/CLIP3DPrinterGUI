/*
 * This module provides C callable APIs for each of the command supported by
 * LightCrafter9000 platform and detailed in the programmer's guide.
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#include "API.h"
#include "string.h"
#include "usb.h"
#include "Common.h"
#include <stdlib.h>
#include <stdio.h>



#define STAT_BIT_FLASH_BUSY     BIT3

typedef struct
{
    const char* name;       /**< Name of the command */
    uint08 I2CCMD;			/**< I2C Command Number */
    uint16 USBCMD;			/**< USB Command Number */
    uint16 len;             /**< Command payload length */
	uint08 varLen;          /**< Variable payload length bytes */
} CmdFormat;

typedef enum
{
    SOURCE_SEL,
    PIXEL_FORMAT,
    CLK_SEL,
    CHANNEL_SWAP,
    FPD_MODE,
    POWER_CONTROL,
    FLIP_LONG,
    FLIP_SHORT,
    TPG_SEL,
    PWM_INVERT,
    LED_ENABLE,
    GET_VERSION,
    SW_RESET,
    DMD_PARK,
    STATUS_HW,
    STATUS_SYS,
    STATUS_MAIN,
    PWM_ENABLE,
    PWM_SETUP,
    PWM_CAPTURE_CONFIG,
    GPIO_CONFIG,
    LED_CURRENT,
    LED_FREQUENCY,
    DISP_CONFIG,
    DISP_MODE,
    TRIG_OUT1_CTL,
    TRIG_OUT2_CTL,
    RED_LED_ENABLE_DLY,
    GREEN_LED_ENABLE_DLY,
    BLUE_LED_ENABLE_DLY,
    PAT_START_STOP,
    TRIG_IN1_CTL,
    TRIG_IN2_CTL,
    INVERT_DATA,
    PAT_CONFIG,
    MBOX_ADDRESS,
    MBOX_CONTROL,
    MBOX_DATA,
    SPLASH_LOAD,
    GPCLK_CONFIG,
    TPG_COLOR,
    PWM_CAPTURE_READ,
    I2C_PASSTHRU,
    PATMEM_LOAD_INIT_MASTER,
    PATMEM_LOAD_DATA_MASTER,
    PATMEM_LOAD_INIT_SLAVE,
    PATMEM_LOAD_DATA_SLAVE,
    BATCHFILE_NAME,
    BATCHFILE_EXECUTE,
    DELAY,
    DEBUG,
    I2C_CONFIG,
    CURTAIN_COLOR,
    VIDEO_CONT_SEL,
    READ_ERROR_CODE,
    READ_ERROR_MSG,
    READ_FRMW_VERSION,
    DMD_BLOCKS,
    DMD_IDLE,
    LED_PULSE_WIDTH,
    MIN_EXPOSURE,
    LOADNGO_INIT_MASTER,
    LOADNGO_DATA_MASTER,
    LOADNGO_INIT_SLAVE,
    LOADNGO_DATA_SLAVE,
    LOADNGO_SRAM,
    READ_WRITE_SRAM,
    BL_STATUS,
    BL_SPL_MODE,
    BL_GET_FLASH_PRESENT,
    BL_GET_MANID,
    BL_GET_DEVID,
    BL_GET_CHKSUM,
    BL_SET_SECTADDR,
    BL_SECT_ERASE,
    BL_SET_DNLDSIZE,
    BL_DNLD_DATA,
    BL_FLASH_TYPE,
    BL_CALC_CHKSUM,
    BL_PROG_MODE,
    BL_MASTER_SLAVE,
    BL_SET_SECTADDR_4BYTE,
    BL_SET_DNLDSIZE_4BYTE,
    BUSY_GPIO_CONFIG,
    FLASH_TEST,
    DEGAMMA_SET,
    PARALLEL_PORT,
    STANDBY_DELAY
}LCR_CMD;

static unsigned char OutputBuffer[USB_MAX_PACKET_SIZE + 1];
static unsigned char InputBuffer[USB_MAX_PACKET_SIZE + 1];

static CmdFormat CmdList[] =
{
	{ "SOURCE_SEL"              , 0x00, 0x1A00, 0x01, 0 },
	{ "PIXEL_FORMAT"            , 0x02, 0x1A02, 0x01, 0 },
	{ "CLK_SEL"                 , 0x03, 0x1A03, 0x01, 0 },
	{ "CHANNEL_SWAP"            , 0x04, 0x1A37, 0x01, 0 },
	{ "FPD_MODE"                , 0x05, 0x1A04, 0x01, 0 },
	{ "POWER_CONTROL"           , 0x07, 0x0200, 0x01, 0 },
	{ "FLIP_LONG"               , 0x08, 0x1008, 0x01, 0 },
	{ "FLIP_SHORT"              , 0x09, 0x1009, 0x01, 0 },
	{ "TPG_SEL"                 , 0x0A, 0x1203, 0x01, 0 },
	{ "PWM_INVERT"              , 0x0B, 0x1A05, 0x01, 0 },
	{ "LED_ENABLE"              , 0x10, 0x1A07, 0x01, 0 },
	{ "GET_VERSION"             , 0x11, 0x0205, 0x00, 0 },
	{ "SW_RESET"                , 0x13, 0x0802, 0x01, 0 },
    { "DMD_PARK"                , 0x14, 0x0609, 0x01, 0 },
	{ "STATUS_HW"               , 0x20, 0x1A0A, 0x00, 0 },
	{ "STATUS_SYS"              , 0x21, 0x1A0B, 0x00, 0 },
	{ "STATUS_MAIN"             , 0x22, 0x1A0C, 0x00, 0 },
	{ "PWM_ENABLE"              , 0x40, 0x1A10, 0x01, 0 },
	{ "PWM_SETUP"               , 0x41, 0x1A11, 0x06, 0 },
	{ "PWM_CAPTURE_CONFIG"      , 0x43, 0x1A12, 0x05, 0 },
	{ "GPIO_CONFIG"             , 0x44, 0x1A38, 0x02, 0 },
	{ "LED_CURRENT"             , 0x4B, 0x0B01, 0x03, 0 },
    { "LED_FREQUENCY"           , 0x4C, 0x0B02, 0x04, 0 },
	{ "DISP_CONFIG"             , 0x7E, 0x1000, 0x10, 0 },
	{ "DISP_MODE"               , 0x69, 0x1A1B, 0x01, 0 },
	{ "TRIG_OUT1_CTL"           , 0x6A, 0x1A1D, 0x05, 0 },
	{ "TRIG_OUT2_CTL"           , 0x6B, 0x1A1E, 0x05, 0 },
    { "RED_LED_ENABLE_DLY"      , 0x6C, 0x1A1F, 0x05, 0 },
    { "GREEN_LED_ENABLE_DLY"    , 0x6D, 0x1A20, 0x05, 0 },
    { "BLUE_LED_ENABLE_DLY"     , 0x6E, 0x1A21, 0x05, 0 },
	{ "PAT_START_STOP"          , 0x65, 0x1A24, 0x01, 0 },
	{ "TRIG_IN1_CTL"            , 0x79, 0x1A35, 0x03, 0 },
	{ "TRIG_IN2_CTL"            , 0x7A, 0x1A36, 0x01, 0 },
	{ "INVERT_DATA"             , 0x74, 0x1A30, 0x01, 0 },
	{ "PAT_CONFIG"              , 0x75, 0x1A31, 0x06, 0 },
	{ "MBOX_ADDRESS"            , 0x76, 0x1A32, 0x02, 0 },
	{ "MBOX_CONTROL"            , 0x77, 0x1A33, 0x01, 0 },
	{ "MBOX_DATA"               , 0x78, 0x1A34, 0x0C, 0 },
	{ "SPLASH_LOAD"             , 0x7F, 0x1A39, 0x01, 0 },
	{ "GPCLK_CONFIG"            , 0x48, 0x0807, 0x03, 0 },
	{ "TPG_COLOR"               , 0x1A, 0x1204, 0x0C, 0 },
	{ "PWM_CAPTURE_READ"        , 0x4E, 0x1A13, 0x05, 0 },
	{ "I2C_PASSTHRU"            , 0x4F, 0x1A4F, 0x05, 2 },
	{ "PATMEM_LOAD_INIT_MASTER" , 0x2A, 0x1A2A, 0x06, 0 },
	{ "PATMEM_LOAD_DATA_MASTER" , 0x2B, 0x1A2B, 0x00, 2 },
	{ "PATMEM_LOAD_INIT_SLAVE"  , 0x2C, 0x1A2C, 0x06, 0 },
	{ "PATMEM_LOAD_DATA_SLAVE"  , 0x2D, 0x1A2D, 0x00, 2 },
	{ "BATCHFILE_NAME"          , 0x3A, 0x1A14, 0x01, 0 },
	{ "BATCHFILE_EXECUTE"       , 0x3B, 0x1A15, 0x01, 0 },
	{ "DELAY"                   , 0x3C, 0x1A16, 0x04, 0 },
	{ "DEBUG"                   , 0x00, 0x1A5B, 0x05, 0 },
	{ "I2C_CONFIG"              , 0x45, 0x1A4E, 0x05, 0 },
	{ "CURTAIN_COLOR"           , 0x06, 0x1100, 0x06, 0 },
	{ "VIDEO_CONT_SEL"          , 0x0C, 0x1A01, 0x01, 0 },
	{ "READ_ERROR_CODE"         , 0x32, 0x0100, 0x01, 0 },
	{ "READ_ERROR_MSG"          , 0x33, 0x0101, 0x80, 0 },
	{ "READ_FRMW_VERSION"       , 0x12, 0x0206, 0x21, 0 },
	{ "DMD_BLOCKS"              , 0x60, 0x1A40, 0x02, 0 },
    { "DMD_IDLE"                , 0x0D, 0x0201, 0x01, 0 },
    { "LED_PULSE_WIDTH"         , 0x64, 0x1A43, 0x04, 0 },
    { "MIN_EXPOSURE"            , 0x63, 0x1A42, 0x02, 0 },
    { "LOADNGO_INIT_MASTER"     , 0x14, 0x1A43, 0x04, 0 },
    { "LOADNGO_DATA_MASTER"     , 0x15, 0x1A44, 0x02, 2 },
    { "LOADNGO_INIT_SLAVE"      , 0x16, 0x1A45, 0x04, 0 },
    { "LOADNGO_DATA_SLAVE"      , 0x17, 0x1A46, 0x02, 2 },
    { "LOADNGO_SRAM"            , 0x18, 0x1A47, 0x01, 0 },
    { "READ_WRITE_SRAM"         , 0x5D, 0x1A70, 0x09, 0 },
    { "BL_STATUS"               , 0x01, 0x0000, 0x00, 0 },
	{ "BL_SPL_MODE"             , 0x23, 0x0023, 0x01, 0 },
	{ "BL_GET_FLASH_PRESENT"    , 0x0B, 0x0015, 0x01, 0 },
	{ "BL_GET_MANID"            , 0x0C, 0x0015, 0x01, 0 },
	{ "BL_GET_DEVID"            , 0x0D, 0x0015, 0x01, 0 },
	{ "BL_GET_CHKSUM"           , 0x00, 0x0015, 0x01, 0 },
	{ "BL_SET_SECTADDR"         , 0x29, 0x0029, 0x04, 0 },
	{ "BL_SECT_ERASE"           , 0x28, 0x0028, 0x00, 0 },
	{ "BL_SET_DNLDSIZE"         , 0x2C, 0x002C, 0x04, 0 },
	{ "BL_DNLD_DATA"            , 0x25, 0x0025, 0x00, 0 },
	{ "BL_FLASH_TYPE"           , 0x2F, 0x002F, 0x01, 0 },
	{ "BL_CALC_CHKSUM"          , 0x26, 0x0026, 0x00, 0 },
	{ "BL_PROG_MODE"            , 0x30, 0x0030, 0x01, 0 },
	{ "BL_MASTER_SLAVE"         , 0x31, 0x0031, 0x01, 0 },
	{ "BL_SET_SECTADDR_4BYTE"   , 0x32, 0x0032, 0x04, 0 },
    { "BL_SET_DNLDSIZE_4BYTE"   , 0x33, 0x0033, 0x04, 0 },
    { "BUSY_GPIO_CONFIG"        , 0x5E, 0x1A5E, 0x01, 0 },
    { "FLASH_TEST"              , 0x5F, 0x1A3A, 0x08, 0 },
    { "DE_GAMMA_SET"            , 0x61, 0x1A3B, 0x02, 0 },
    { "PARALLEL_PORT"           , 0x64, 0x1A3C, 0x12, 0 },
    { "STANDBY_DELAY"           , 0x66, 0x1A25, 0x01, 0 }
};

static unsigned char seqNum=0;
static unsigned char PatLut[12][512];
static unsigned int PatLutIndex = 0;
static API_DataCallback_t *LCR_DataCallback;
static void *LCR_CallbackParam;
static unsigned char bufferIndex = 1;


int LCR_SetOutputBufferIndex(int index)
{
    if( index == USBHID )
        bufferIndex = 1;
    else
        bufferIndex = 0;

    return 0;
}

int LCR_Write(BOOL ackRequired)
{
    if (!ackRequired)
        return USB_Write(OutputBuffer);

    int ret = USB_Write(OutputBuffer);
    if (ret > 0)
    {
        if(USB_Read(InputBuffer) <= 0)
            return -2;

        hidMessageStruct *pMsg = (hidMessageStruct *)InputBuffer;
        if (pMsg->head.flags.nack)
            return -2;

        return ret;
    }
    return -1;
}

int LCR_Read()
/**
 * This function is private to this file. This function is called to write the read control command and then read back 64 bytes over USB
 * to InputBuffer.
 *
 * @return  number of bytes read
 *          -2 = nack from target
 *          -1 = error reading
 *
 */
{
    int ret_val;
    hidMessageStruct *pMsg = (hidMessageStruct *)InputBuffer;
    if(USB_Write(OutputBuffer) > 0)
    {
        ret_val =  USB_Read(InputBuffer);

        if((pMsg->head.flags.nack == 1) || (pMsg->head.length == 0))
            return -2;
        else
            return ret_val;
    }
    return -1;
}

int LCR_ContinueRead()
{
    return USB_Read(InputBuffer);
}

int LCR_SendMsg(hidMessageStruct *pMsg)
/**
 * This function is private to this file. This function is called to send a message over USB; in chunks of 64 bytes.
 *
 * @return  number of bytes sent
 *          -1 = FAIL
 *
 */
{
    int dataBytesSent;
               int totalLen;
    BOOL ackRequired = pMsg->head.flags.reply;

    /* Disable Read back for BL_DNLD_DATA, BL_CALC_CHKSUM, PATMEM_LOAD_DATAcommands */
    if ((pMsg->text.cmd == 0x25) || (pMsg->text.cmd == 0x26) ||
            (pMsg->text.cmd == 0x1A2B) || (pMsg->text.cmd == 0x1A2D) || (pMsg->text.cmd == 0x1A32))
    {
        ackRequired = FALSE;
        pMsg->head.flags.reply = 0;
    }

    OutputBuffer[0]=0; // First byte is the report number
    dataBytesSent = 0;

    totalLen = sizeof(pMsg->head) + pMsg->head.length;

    while(dataBytesSent < totalLen)
    {
        memcpy(&OutputBuffer[bufferIndex], (uint08 *)pMsg + dataBytesSent, USB_MAX_PACKET_SIZE);
        if(LCR_Write(ackRequired) < 0)
            return -1;
        dataBytesSent += USB_MAX_PACKET_SIZE;
    }

    if (LCR_DataCallback != NULL)
    {
        uint16 i;
        for(i = 0; i < ARRAY_SIZE(CmdList); i++)
        {
            if(CmdList[i].USBCMD == pMsg->text.cmd)
			{
				API_CommandInfo_t CmdInfo;

				CmdInfo.I2CCmd = CmdList[i].I2CCMD;
				CmdInfo.USBCmd = CmdList[i].USBCMD;
				CmdInfo.Read = pMsg->head.flags.rw;
				CmdInfo.CmdName = CmdList[i].name;
                CmdInfo.Payload = pMsg->text.data + 2;
                CmdInfo.PayloadLen = pMsg->head.length - 2;
				LCR_DataCallback(LCR_CallbackParam, &CmdInfo);
				break;
			}
        }
    }

    return dataBytesSent+sizeof(pMsg->head);
}


int LCR_PrepReadCmd(LCR_CMD cmd)
/*
 * This function is private to this file. Prepares the read-control command
 * packet for the given command code and copies it to OutputBuffer.
 *
 * @param   cmd  - I - USB command code.
 *
 * @return  0 = PASS
 *          -1 = FAIL
 *
 */
{
    hidMessageStruct msg;

    msg.head.flags.rw = 1; //Read
    msg.head.flags.reply = 1; //Host wants a reply from device
    msg.head.flags.dest = 0; //Projector Control Endpoint
    msg.head.flags.reserved = 0;
    msg.head.flags.nack = 0;
    msg.head.seq = 0;

    msg.text.cmd = CmdList[cmd].USBCMD;
    msg.head.length = 2;

    if (cmd == BL_GET_FLASH_PRESENT)
    {
        msg.text.data[2] = 0x0B;
        msg.head.length += 1;
    }
    else if(cmd == BL_GET_MANID)
    {
        msg.text.data[2] = 0x0C;
        msg.head.length += 1;
    }
    else if (cmd == BL_GET_DEVID)
    {
        msg.text.data[2] = 0x0D;
        msg.head.length += 1;
    }
    else if (cmd == BL_GET_CHKSUM)
    {
        msg.text.data[2] = 0x00;
        msg.head.length += 1;
    }

    OutputBuffer[0]=0; // First byte is the report number
    memcpy(&OutputBuffer[bufferIndex], &msg, (sizeof(msg.head)+sizeof(msg.text.cmd) + msg.head.length));
    return 0;
}

int LCR_PrepReadCmdWithParam(LCR_CMD cmd, unsigned char param)
/**
 * This function is private to this file. Prepares the read-control command packet for the given command code and parameter and copies it to OutputBuffer.
 *
 * @param   cmd  - I - USB command code.
 * @param   param - I - parameter to be used for tis read command.
 *
 * @return  0 = PASS
 *          -1 = FAIL
 *
 */
{
    hidMessageStruct msg;

    msg.head.flags.rw = 1; //Read
    msg.head.flags.reply = 1; //Host wants a reply from device
    msg.head.flags.dest = 0; //Projector Control Endpoint
    msg.head.flags.reserved = 0;
    msg.head.flags.nack = 0;
    msg.head.seq = 0;

    msg.text.cmd = CmdList[cmd].USBCMD;
    msg.head.length = 3;

    msg.text.data[2] = param;

    OutputBuffer[0]=0; // First byte is the report number
    memcpy(&OutputBuffer[bufferIndex], &msg, (sizeof(msg.head)+sizeof(msg.text.cmd) + msg.head.length));
    return 0;
}

int LCR_PrepReadCmdWithWordParam(LCR_CMD cmd, uint32 param, uint08 type)
/**
 * This function is private to this file. Prepares the read-control command packet for the given command code and parameter and copies it to OutputBuffer.
 *
 * @param   cmd  - I - USB command code.
 * @param   param - I - parameter to be used for tis read command.
 *
 * @return  0 = PASS
 *          -1 = FAIL
 *
 */
{
    hidMessageStruct msg;
    uint32 *p = (uint32*)&msg.text.data[2];

    msg.head.flags.rw = 1; //Read
    msg.head.flags.reply = 1; //Host wants a reply from device
    msg.head.flags.dest = 0; //Projector Control Endpoint
    msg.head.flags.reserved = 0;
    msg.head.flags.nack = 0;
    msg.head.seq = 0;

    msg.text.cmd = CmdList[cmd].USBCMD;
    msg.head.length = 7;
    msg.text.data[6] = type;

    *p = param;

    OutputBuffer[0]=0; // First byte is the report number
    memcpy(&OutputBuffer[bufferIndex], &msg, (sizeof(msg.head)+sizeof(msg.text.cmd) + msg.head.length));
    return 0;
}

int LCR_PrepWriteCmd(hidMessageStruct *pMsg, LCR_CMD cmd)
/**
 * This function is private to this file. Prepares the write command packet with given command code in the message structure pointer passed.
 *
 * @param   cmd  - I - USB command code.
 * @param   pMsg - I - Pointer to the message.
 *
 * @return  0 = PASS
 *          -1 = FAIL
 *
 */
{
    pMsg->head.flags.rw = 0; //Write
    pMsg->head.flags.reply = 0; //Host wants a reply from device
    pMsg->head.flags.dest = 0; //Projector Control Endpoint
    pMsg->head.flags.reserved = 0;
    pMsg->head.flags.nack = 0;
    pMsg->head.seq = seqNum++;

    pMsg->text.cmd = CmdList[cmd].USBCMD;
    pMsg->head.length = CmdList[cmd].len + 2;

    return 0;
}

int LCR_GetVersion(unsigned int *pApp_ver, unsigned int *pAPI_ver, unsigned int *pSWConfig_ver, unsigned int *pSeqConfig_ver)
/**
 * This command reads the version information of the DLPC900 firmware.
 * (I2C: 0x11)
 * (USB: CMD2: 0x02, CMD3: 0x05)
 *
 * @param   pApp_ver  - O - Application Software Revision BITS 0:15 PATCH NUMBER, BITS 16:23 MINOR REVISION, BIS 24:31 MAJOR REVISION
 * @param   pAPI_ver  - O - API Software Revision BITS 0:15 PATCH NUMBER, BITS 16:23 MINOR REVISION, BIS 24:31 MAJOR REVISION
 * @param   pSWConfig_ver  - O - Software Configuration Revision BITS 0:15 PATCH NUMBER, BITS 16:23 MINOR REVISION, BIS 24:31 MAJOR REVISION
 * @param   pSeqConfig_ver  - O - Sequence Configuration Revision BITS 0:15 PATCH NUMBER, BITS 16:23 MINOR REVISION, BIS 24:31 MAJOR REVISION
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(GET_VERSION);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);


        *pApp_ver = msg.text.data[0];
        *pApp_ver |= (unsigned int)msg.text.data[1] << 8;
        *pApp_ver |= (unsigned int)msg.text.data[2] << 16;
        *pApp_ver |= (unsigned int)msg.text.data[3] << 24;

        *pAPI_ver = msg.text.data[4];
        *pAPI_ver |= (unsigned int)msg.text.data[5] << 8;
        *pAPI_ver |= (unsigned int)msg.text.data[6] << 16;
        *pAPI_ver |= (unsigned int)msg.text.data[7] << 24;

        *pSWConfig_ver = msg.text.data[8];
        *pSWConfig_ver |= (unsigned int)msg.text.data[9] << 8;
        *pSWConfig_ver |= (unsigned int)msg.text.data[10] << 16;
        *pSWConfig_ver |= (unsigned int)msg.text.data[11] << 24;

        *pSeqConfig_ver = msg.text.data[12];
        *pSeqConfig_ver |= (unsigned int)msg.text.data[13] << 8;
        *pSeqConfig_ver |= (unsigned int)msg.text.data[14] << 16;
        *pSeqConfig_ver |= (unsigned int)msg.text.data[15] << 24;

        return 0;
    }
    return -1;
}

int LCR_GetLedEnables(BOOL *pSeqCtrl, BOOL *pRed, BOOL *pGreen, BOOL *pBlue)
/**
 * This command reads back the state of LED control method as well as the enabled/disabled status of all LEDs.
 * (I2C: 0x10)
 * (USB: CMD2: 0x1A, CMD3: 0x07)
 *
 * @param   pSeqCtrl  - O - 1 - All LED enables are controlled by the Sequencer and ignore the other LED enable settings.
 *                          0 - All LED enables are controlled by pRed, pGreen and pBlue seetings and ignore Sequencer control
 * @param   pRed  - O - 0 - Red LED is disabled
 *                      1 - Red LED is enabled
 * @param   pGreen  - O - 0 - Green LED is disabled
 *                      1 - Green LED is enabled
 * @param   pBlue  - O - 0 - Blue LED is disabled
 *                      1 - Blue LED is enabled]
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(LED_ENABLE);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);

        if(msg.text.data[0] & BIT0)
            *pRed = TRUE;
        else
            *pRed = FALSE;

        if(msg.text.data[0] & BIT1)
            *pGreen = TRUE;
        else
            *pGreen = FALSE;

        if(msg.text.data[0] & BIT2)
            *pBlue = TRUE;
        else
            *pBlue = FALSE;

        if(msg.text.data[0] & BIT3)
            *pSeqCtrl = TRUE;
        else
            *pSeqCtrl = FALSE;
        return 0;
    }
    return -1;
}


int LCR_SetLedEnables(BOOL SeqCtrl, BOOL Red, BOOL Green, BOOL Blue)
/**
 * This command sets the state of LED control method as well as the enabled/disabled status of all LEDs.
 * (I2C: 0x10)
 * (USB: CMD2: 0x1A, CMD3: 0x07)
 *
 * @param   pSeqCtrl  - I - 1 - All LED enables are controlled by the Sequencer and ignore the other LED enable settings.
 *                          0 - All LED enables are controlled by pRed, pGreen and pBlue seetings and ignore Sequencer control
 * @param   pRed  - I - 0 - Red LED is disabled
 *                      1 - Red LED is enabled
 * @param   pGreen  - I - 0 - Green LED is disabled
 *                      1 - Green LED is enabled
 * @param   pBlue  - I - 0 - Blue LED is disabled
 *                      1 - Blue LED is enabled]
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;
    unsigned char Enable=0;

    if(SeqCtrl)
        Enable |= BIT3;
    if(Red)
        Enable |= BIT0;
    if(Green)
        Enable |= BIT1;
    if(Blue)
        Enable |= BIT2;

    msg.text.data[2] = Enable;
    LCR_PrepWriteCmd(&msg, LED_ENABLE);

    return LCR_SendMsg(&msg);
}

int LCR_SetIT6535PowerMode(API_VideoConnector_t powerMode)
{
    hidMessageStruct msg;
    msg.text.data[2] = powerMode;
    LCR_PrepWriteCmd(&msg, VIDEO_CONT_SEL);

    return LCR_SendMsg(&msg);
}

int LCR_GetIT6535PowerMode(API_VideoConnector_t *pPowerMode)
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(VIDEO_CONT_SEL);
    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);

        *pPowerMode = msg.text.data[0];

        return 0;
    }
    return -1;
}

int LCR_GetLedCurrents(unsigned char *pRed, unsigned char *pGreen, unsigned char *pBlue)
/**
 * (I2C: 0x4B)
 * (USB: CMD2: 0x0B, CMD3: 0x01)
 * This parameter controls the pulse duration of the specific LED PWM modulation output pin. The resolution
 * is 8 bits and corresponds to a percentage of the LED current. The PWM value can be set from 0 to 100%
 * in 256 steps . If the LED PWM polarity is set to normal polarity, a setting of 0xFF gives the maximum
 * PWM current. The LED current is a function of the specific LED driver design.
 *
 * @param   pRed  - O - Red LED PWM current control Valid range, assuming normal polarity of PWM signals, is:
 *                      0x00 (0% duty cycle → Red LED driver generates no current
 *                      0xFF (100% duty cycle → Red LED driver generates maximum current))
 *                      The current level corresponding to the selected PWM duty cycle is a function of the specific LED driver design and thus varies by design.
 * @param   pGreen  - O - Green LED PWM current control Valid range, assuming normal polarity of PWM signals, is:
 *                      0x00 (0% duty cycle → Red LED driver generates no current
 *                      0xFF (100% duty cycle → Red LED driver generates maximum current))
 *                      The current level corresponding to the selected PWM duty cycle is a function of the specific LED driver design and thus varies by design.
 * @param   pBlue  - O - Blue LED PWM current control Valid range, assuming normal polarity of PWM signals, is:
 *                      0x00 (0% duty cycle → Red LED driver generates no current
 *                      0xFF (100% duty cycle → Red LED driver generates maximum current))
 *                      The current level corresponding to the selected PWM duty cycle is a function of the specific LED driver design and thus varies by design.
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(LED_CURRENT);
    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);

        *pRed = msg.text.data[0];
        *pGreen = msg.text.data[1];
        *pBlue = msg.text.data[2];

        return 0;
    }
    return -1;
}


int LCR_GetLedFrequency(unsigned int *pFrequency)
/**
 * (I2C: 0x4C)
 * (USB: CMD2: 0x0B, CMD3: 0x02)
 * Gets LED PWM frequency.
 *
 * @param   pfrequency  - O - pointer to Frequency
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(LED_FREQUENCY);
    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);

        *pFrequency =   (msg.text.data[3] << 24)|
                        (msg.text.data[2] << 16)|
                        (msg.text.data[1] << 8) |
                        (msg.text.data[0]);
        return 0;
    }
    return -1;
}

int LCR_SetLedCurrents(unsigned char RedCurrent, unsigned char GreenCurrent, unsigned char BlueCurrent)
/**
 * (I2C: 0x4B)
 * (USB: CMD2: 0x0B, CMD3: 0x01)
 * This parameter controls the pulse duration of the specific LED PWM modulation output pin. The resolution
 * is 8 bits and corresponds to a percentage of the LED current. The PWM value can be set from 0 to 100%
 * in 256 steps . If the LED PWM polarity is set to normal polarity, a setting of 0xFF gives the maximum
 * PWM current. The LED current is a function of the specific LED driver design.
 *
 * @param   RedCurrent  - I - Red LED PWM current control Valid range, assuming normal polarity of PWM signals, is:
 *                      0x00 (0% duty cycle → Red LED driver generates no current
 *                      0xFF (100% duty cycle → Red LED driver generates maximum current))
 *                      The current level corresponding to the selected PWM duty cycle is a function of the specific LED driver design and thus varies by design.
 * @param   GreenCurrent  - I - Green LED PWM current control Valid range, assuming normal polarity of PWM signals, is:
 *                      0x00 (0% duty cycle → Red LED driver generates no current
 *                      0xFF (100% duty cycle → Red LED driver generates maximum current))
 *                      The current level corresponding to the selected PWM duty cycle is a function of the specific LED driver design and thus varies by design.
 * @param   BlueCurrent  - I - Blue LED PWM current control Valid range, assuming normal polarity of PWM signals, is:
 *                      0x00 (0% duty cycle → Red LED driver generates no current
 *                      0xFF (100% duty cycle → Red LED driver generates maximum current))
 *                      The current level corresponding to the selected PWM duty cycle is a function of the specific LED driver design and thus varies by design.
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = RedCurrent;
    msg.text.data[3] = GreenCurrent;
    msg.text.data[4] = BlueCurrent;

    LCR_PrepWriteCmd(&msg, LED_CURRENT);

    return LCR_SendMsg(&msg);
}

int LCR_SetLedFrequency(unsigned int Frequency)
/**
 * (I2C: 0x4C)
 * (USB: CMD2: 0x0B, CMD3: 0x02)
 * Sets LED PWM frequency
 *
 * @param   Frequency  - I - PWM frequency
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = Frequency;
    msg.text.data[3] = Frequency >> 8;
    msg.text.data[4] = Frequency >> 16;
    msg.text.data[5] = Frequency >> 24;

    LCR_PrepWriteCmd(&msg, LED_FREQUENCY);

    return LCR_SendMsg(&msg);
}

BOOL LCR_GetLongAxisImageFlip(void)
/**
 * (I2C: 0x08)
 * (USB: CMD2: 0x10, CMD3: 0x08)
 * The Long-Axis Image Flip defines whether the input image is flipped across the long axis of the DMD. If
 * this parameter is changed while displaying a still image, the input still image should be re-sent. If the
 * image is not re-sent, the output image might be slightly corrupted. In Structured Light mode, the image
 * flip will take effect on the next bit-plane, image, or video frame load.
 *
 * @return  TRUE = Image flipped along long axis    <BR>
 *          FALSE = Image not flipped  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(FLIP_LONG);
    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);

        if ((msg.text.data[0] & BIT0) == BIT0)
            return TRUE;
        else
            return FALSE;
    }
    return FALSE;
}

BOOL LCR_GetShortAxisImageFlip(void)
/**
 * (I2C: 0x09)
 * (USB: CMD2: 0x10, CMD3: 0x09)
 * The Short-Axis Image Flip defines whether the input image is flipped across the short axis of the DMD. If
 * this parameter is changed while displaying a still image, the input still image should be re-sent. If the
 * image is not re-sent, the output image might be slightly corrupted. In Structured Light mode, the image
 * flip will take effect on the next bit-plane, image, or video frame load.
 *
 * @return  TRUE = Image flipped along short axis    <BR>
 *          FALSE = Image not flipped  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(FLIP_SHORT);
    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);

        if ((msg.text.data[0] & BIT0) == BIT0)
            return TRUE;
        else
            return FALSE;
    }
    return FALSE;
}


int LCR_SetLongAxisImageFlip(BOOL Flip)
/**
 * (I2C: 0x08)
 * (USB: CMD2: 0x10, CMD3: 0x08)
 * The Long-Axis Image Flip defines whether the input image is flipped across the long axis of the DMD. If
 * this parameter is changed while displaying a still image, the input still image should be re-sent. If the
 * image is not re-sent, the output image might be slightly corrupted. In Structured Light mode, the image
 * flip will take effect on the next bit-plane, image, or video frame load.
 *
 * @param   Flip -I TRUE = Image flipped along long axis enable    <BR>
 *                  FALSE = Do not flip image <BR>
 *
 * @return >=0 PASS <BR>
 *         <0 FAIL <BR>
 *
 */
{
    hidMessageStruct msg;

    if(Flip)
        msg.text.data[2] = BIT0;
    else
        msg.text.data[2] = 0;

    LCR_PrepWriteCmd(&msg, FLIP_LONG);

    return LCR_SendMsg(&msg);
}

int LCR_SetShortAxisImageFlip(BOOL Flip)
/**
 * (I2C: 0x09)
 * (USB: CMD2: 0x10, CMD3: 0x09)
 * The Long-Axis Image Flip defines whether the input image is flipped across the long axis of the DMD. If
 * this parameter is changed while displaying a still image, the input still image should be re-sent. If the
 * image is not re-sent, the output image might be slightly corrupted. In Structured Light mode, the image
 * flip will take effect on the next bit-plane, image, or video frame load.
 *
 * @param   Flip -I TRUE = Image flipped along long axis enable    <BR>
 *                  FALSE = Do not flip image <BR>
 *
 * @return >=0 PASS <BR>
 *         <0 FAIL <BR>
 *
 */
{
    hidMessageStruct msg;

    if(Flip)
        msg.text.data[2] = BIT0;
    else
        msg.text.data[2] = 0;

    LCR_PrepWriteCmd(&msg, FLIP_SHORT);

    return LCR_SendMsg(&msg);
}

int LCR_EnterProgrammingMode()
/**
 * This function is to be called to put the unit in programming mode. Only programming mode APIs will work once
 * in this mode.
 *
 * @return >=0 PASS <BR>
 *         <0 FAIL <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = 1;

    LCR_PrepWriteCmd(&msg, BL_PROG_MODE);

    return LCR_SendMsg(&msg);
}

int LCR_ExitProgrammingMode(void)
/**
 * This function works only in prorgamming mode.
 * This function is to be called to exit programming mode and resume normal operation with the new downloaded firmware.
 *
 * @return >=0 PASS <BR>
 *         <0 FAIL <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = 2;
    LCR_PrepWriteCmd(&msg, BL_PROG_MODE);

    return LCR_SendMsg(&msg);
}

int LCR_EnableMasterSlave(BOOL MasterEnable, BOOL SlaveEnable)
/**
 * Enable/Disable Master/Slave ASIC for the following bootloader commands
 *
 * @param MasterEnable TRUE = Enable Master, FALSE = Disable Master
 * @param SlaveEnable TRUE = Enable Slave, FALSE = Disable Slave
 *
 * @return 0 PASS, <0 FAIL
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = ((!MasterEnable) | ((!SlaveEnable) << 1)) & 3;
    LCR_PrepWriteCmd(&msg, BL_MASTER_SLAVE);

    return LCR_SendMsg(&msg);
}

int LCR_GetFlashPresent(BOOL *pFlashAtCS0, BOOL *pFlashAtCS1, BOOL *pFlashAtCS2)
/**
 * This function works only in prorgamming mode.
 * This function returns the number of flash present.
 *
 * @param pFlashAtCS0 - O - TRUE  = Flash is present at CS0   <BR>
 *                          FALSE = Flash is NOT present at CS0 <BR>
 *
 * @param pFlashAtCS1 - O - TRUE  = Flash is present at CS1   <BR>
 *                          FALSE = Flash is NOT present at CS1 <BR>
 *
 * @param pFlashAtCS2 - O - TRUE  = Flash is present at CS2   <BR>
 *                          FALSE = Flash is NOT present at CS2 <BR>
 *
 *
 * @return 0 PASS <BR>
 *         -1 FAIL <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(BL_GET_FLASH_PRESENT);
    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);

        *pFlashAtCS0 = (msg.text.data[6] & BIT0) ? TRUE : FALSE;
        *pFlashAtCS1 = (msg.text.data[6] & BIT1) ? TRUE : FALSE;
        *pFlashAtCS2 = (msg.text.data[6] & BIT2) ? TRUE : FALSE;

        return 0;
    }
    return -1;
}


int LCR_GetFlashManID(unsigned short *pManID)
/**
 * This function works only in prorgamming mode.
 * This function returns the manufacturer ID of the flash part interfaced with the controller.
 *
 * @param pManID - O - Manufacturer ID of the flash part
 *
 * @return 0 PASS <BR>
 *         -1 FAIL <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(BL_GET_MANID);
    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);

        *pManID = msg.text.data[6];
        *pManID |= (unsigned short)msg.text.data[7] << 8;
        return 0;
    }
    return -1;
}

int LCR_GetFlashDevID(unsigned long long *pDevID)
/**
 * This function works only in prorgamming mode.
 * This function returns the device ID of the flash part interfaced with the controller.
 *
 * @param pDevID - O - Device ID of the flash part
 *
 * @return 0 PASS <BR>
 *         -1 FAIL <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(BL_GET_DEVID);
    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);

        *pDevID = msg.text.data[6];
        *pDevID |= (unsigned long long)msg.text.data[7] << 8;
        *pDevID |= (unsigned long long)msg.text.data[8] << 16;
        *pDevID |= (unsigned long long)msg.text.data[9] << 24;
        *pDevID |= (unsigned long long)msg.text.data[12] << 32;
        *pDevID |= (unsigned long long)msg.text.data[13] << 40;
        *pDevID |= (unsigned long long)msg.text.data[14] << 48;
        *pDevID |= (unsigned long long)msg.text.data[15] << 56;
        return 0;
    }
    return -1;
}

int LCR_SetFlashAddr(unsigned int Addr)
/**
 * This function works only in prorgamming mode.
 * This function is to be called to set the address prior to calling LCR_FlashSectorErase or LCR_DownloadData APIs.
 * This function only accepts 3-byte address the MSB byete is discarded.
 *
 * @param Addr - I - 32-bit absolute address.
 *
 * @return >=0 PASS <BR>
 *         <0 FAIL <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = Addr;
    msg.text.data[3] = Addr >> 8;
    msg.text.data[4] = Addr >> 16;
    msg.text.data[5] = Addr >> 24;

    LCR_PrepWriteCmd(&msg, BL_SET_SECTADDR);

    return LCR_SendMsg(&msg);
}

int LCR_SetFlashAddr4Byte(unsigned int Addr)
/**
 * This function works only in prorgamming mode.
 * This function is to be called to set the address prior to calling LCR_FlashSectorErase or LCR_DownloadData APIs.
 * This function accepts full 4-byte address
 * @param Addr - I - 32-bit absolute address.
 *
 * @return >=0 PASS <BR>
 *         <0 FAIL <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = Addr;
    msg.text.data[3] = Addr >> 8;
    msg.text.data[4] = Addr >> 16;
    msg.text.data[5] = Addr >> 24;

    LCR_PrepWriteCmd(&msg, BL_SET_SECTADDR_4BYTE);

    return LCR_SendMsg(&msg);
}



int LCR_FlashSectorErase(void)
/**
  * This function works only in prorgamming mode.
  * This function is to be called to erase a sector of flash. The address of the sector to be erased
  * is to be set by using the LCR_SetFlashAddr() API
  *
  * @return >=0 PASS <BR>
  *         <0 FAIL <BR>
  *
  */
{
    hidMessageStruct msg;

    LCR_PrepWriteCmd(&msg, BL_SECT_ERASE);
    return LCR_SendMsg(&msg);
}

int LCR_SetDownloadSize(unsigned int dataLen)
/**
 * This function works only in prorgamming mode.
 * This function is to be called to set the payload size of data to be sent using LCR_DownloadData API.
 * This function only accepts 3-byte address the MSB byete is discarded.
 * @param dataLen -I - length of download data payload in bytes.
 *
 * @return >=0 PASS <BR>
 *         <0 FAIL <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = dataLen;
    msg.text.data[3] = dataLen >> 8;
    msg.text.data[4] = dataLen >> 16;
    msg.text.data[5] = dataLen >> 24;

    LCR_PrepWriteCmd(&msg, BL_SET_DNLDSIZE);

    return LCR_SendMsg(&msg);
}

int LCR_SetDownloadSize4Byte(unsigned int dataLen)
/**
 * This function works only in prorgamming mode.
 * This function is to be called to set the payload size of data to be sent using LCR_DownloadData API.
 * This function accepts data size upto 4-bytes
 * @param dataLen -I - length of download data payload in bytes.
 *
 * @return >=0 PASS <BR>
 *         <0 FAIL <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = dataLen;
    msg.text.data[3] = dataLen >> 8;
    msg.text.data[4] = dataLen >> 16;
    msg.text.data[5] = dataLen >> 24;

    LCR_PrepWriteCmd(&msg, BL_SET_DNLDSIZE_4BYTE);

    return LCR_SendMsg(&msg);
}


int LCR_DownloadData(unsigned char *pByteArray, unsigned int dataLen)
/**
 * This function works only in prorgamming mode.
 * This function sends one payload of data to the controller at a time. takes the total size of payload
 * in the parameter dataLen and returns the actual number of bytes that was sent in the return value.
 * This function needs to be called multiple times until all of the desired bytes are sent.
 *
 * @param pByteArray - I - Pointer to where the data to be downloaded is to be fetched from
 * @param dataLen -I - length in bytes of the total payload data to download.
 *
 * @return number of bytes actually downloaded <BR>
 *         <0 FAIL <BR>
 *
 */
{
    hidMessageStruct msg;
    int retval;

    if(dataLen > API_MAX_PARAM_SIZE)
        dataLen = API_MAX_PARAM_SIZE;

    CmdList[BL_DNLD_DATA].len = dataLen;
    memcpy(&msg.text.data[2], pByteArray, dataLen);

    LCR_PrepWriteCmd(&msg, BL_DNLD_DATA);

    retval = LCR_SendMsg(&msg);
    if(retval > 0)
        return dataLen;

    return -1;
}

int LCR_GetBLStatus(BootLoaderStaus *pBL_Status)
/**
 * This function works only in prorgamming mode.
 * This function returns the device ID of the flash part interfaced with the controller.
 *
 * @param pBL_Status - O - Pouplate the BootLoaderStaus structure pointer
 *                         with value returned from the DLPC900 in bootmode
 *                         The information contain Program Mode, Busy, BootLoader
 *                         version.
 *
 * @return 0 PASS <BR>
 *         -1 FAIL <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(BL_STATUS);
    if(LCR_Read() > 0)
    {
		int i;
        memcpy(&msg, InputBuffer, 65);

        pBL_Status->blstatus = msg.text.data[0];
        pBL_Status->blVerMajor = ((msg.text.data[1] >> 4) & 0x0F);
        pBL_Status->blVerMinor = (msg.text.data[1] & 0xF);
        pBL_Status->blVerPatch = (msg.text.data[2] & 0x0F);
        pBL_Status->blASICID = msg.text.data[3];
        for(i = 0; i<7;i++)
            pBL_Status->rsvd[i] = msg.text.data[4+i];

        return 0;
    }

    return -1;
}

void LCR_WaitForFlashReady()
/**
 * This function works only in prorgamming mode.
 * This function polls the status bit and returns only when the controller is ready for next command.
 *
 */
{
   BootLoaderStaus BLstatus;

   BLstatus.blstatus = STAT_BIT_FLASH_BUSY;
   do
   {
        LCR_GetBLStatus(&BLstatus);
   }
   while((BLstatus.blstatus & STAT_BIT_FLASH_BUSY) == STAT_BIT_FLASH_BUSY);//Wait for flash busy flag to go off
}

int LCR_SetFlashType(unsigned char Type)
/**
 * This function works only in prorgamming mode.
 * This function is to be used to set the programming type of the flash device attached to the controller.
 *
 * @param Type - I - Type of the flash device.
 *
 * @return >=0 PASS <BR>
 *         <0 FAIL <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = Type;

    LCR_PrepWriteCmd(&msg, BL_FLASH_TYPE);

    return LCR_SendMsg(&msg);
}

int LCR_CalculateFlashChecksum(void)
/**
 * This function works only in prorgamming mode.
 * This function is to be issued to instruct the controller to calculate the flash checksum.
 * LCR_WaitForFlashReady() is then to be called to ensure that the controller is done and then call
 * LCR_GetFlashChecksum() API to retrieve the actual checksum from the controller.
 *
 * @return 0 = PASS <BR>
 *         -1 = FAIL <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepWriteCmd(&msg, BL_CALC_CHKSUM);

    if(LCR_SendMsg(&msg) <= 0)
        return -1;

    return 0;

}

int LCR_GetFlashChecksum(unsigned int*checksum)
/**
 * This function works only in prorgamming mode.
 * This function is to be used to retrieve the flash checksum from the controller.
 * LCR_CalculateFlashChecksum() and LCR_WaitForFlashReady() must be called before using this API.
 *
 * @param checksum - O - variable in which the flash checksum is to be returned
 *
 * @return >=0 PASS <BR>
 *         <0 FAIL <BR>
 *
 */
{
    hidMessageStruct msg;
#if 0
    LCR_PrepWriteCmd(&msg, BL_CALC_CHKSUM);

    if(LCR_SendMsg(&msg) <= 0)
        return -1;

    LCR_WaitForFlashReady();
#endif
    LCR_PrepReadCmd(BL_GET_CHKSUM);
    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);

        *checksum = msg.text.data[6];
        *checksum |= (unsigned int)msg.text.data[7] << 8;
        *checksum |= (unsigned int)msg.text.data[8] << 16;
        *checksum |= (unsigned int)msg.text.data[9] << 24;
        return 0;
    }
    return -1;
}

int LCR_GetStatus(unsigned char *pHWStatus, unsigned char *pSysStatus, unsigned char *pMainStatus)
/**
 * This function is to be used to check the various status indicators from the controller.
 * Refer to DLPC900 Programmer's guide section 2.1 "DLPC900 Status Commands" for detailed description of each byte.
 *
 * Refer to DLPC900 Programmer's guide section 2.1 "DLPC900 Status Commands" for detailed description of each byte.
 *
 * @param pHWStatus - O - provides status information on the DLPC900's sequencer, DMD controller and initialization.
 * @param pSysStatus - O - provides DLPC900 status on internal memory tests..
 * @param pMainStatus - O - provides DMD park status and DLPC900 sequencer, frame buffer, and gamma correction status.
 *
 *
 * @return 0 PASS <BR>
 *         -1 FAIL <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(STATUS_HW);
    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);

        *pHWStatus = msg.text.data[0];
    }
    else
        return -1;

    LCR_PrepReadCmd(STATUS_SYS);
    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);

        *pSysStatus = msg.text.data[0];
    }
    else
        return -1;

    LCR_PrepReadCmd(STATUS_MAIN);
    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);

        *pMainStatus = msg.text.data[0];
    }
    else
        return -1;

    return 0;
}

int LCR_SoftwareReset(void)
/**
 * Use this API to reset the controller
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = 0x01;
    LCR_PrepWriteCmd(&msg, SW_RESET);

    return LCR_SendMsg(&msg);
}

int LCR_SoftwareDMDPark(int DmdPark)
/**
 * Use this API to DMD park
 * @param DmdPark - I - 0x01 - Park DMD
 *                      0x00 - Unpark DMD
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = DmdPark;
    LCR_PrepWriteCmd(&msg, DMD_PARK);

    return LCR_SendMsg(&msg);
}


int LCR_GetSoftwareDMDPark(int *pDmdPark)
/**
 * Use this API to DMD park
 * @param *pDmdPark - O - 0x01 - DMD is parked state
 *                        0x00 - DMD is unparked state
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(DMD_PARK);
    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pDmdPark = msg.text.data[0];
        return 0;
    }
    return -1;
}

int LCR_SetMode(API_DisplayMode_t SLmode)
/**
 * The Display Mode Selection Command enables the DLPC900 internal image processing functions for
 * video mode or bypasses them for pattern display mode. This command selects between video or pattern
 * display mode of operation.
 *
 * @param   SLmode  - I - 0 - PTN_MODE_DISABLE = Disable pattern mode
 *                        1 - PTN_MODE_SPLASH  = Patterns are loaded from flash
 *                        2 - PTN_MODE_VIDEO   = Patterns are streamed through video port
 *                        3 - PTN_MODE_OTF     = Patterns are loaded on the fly through USB/I2C
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = SLmode;
    LCR_PrepWriteCmd(&msg, DISP_MODE);

    return LCR_SendMsg(&msg);
}

int LCR_GetMode(API_DisplayMode_t *pMode)
/**
 * The Display Mode Selection Command enables the DLPC900 internal image processing functions for
 * video mode or bypasses them for pattern display mode. This command selects between video or pattern
 * display mode of operation.
 *
 * @param   pMode   - I - 0 - PTN_MODE_DISABLE = Disable pattern mode
 *                        1 - PTN_MODE_SPLASH  = Patterns are loaded from flash
 *                        2 - PTN_MODE_VIDEO   = Patterns are streamed through video port
 *                        3 - PTN_MODE_OTF     = Patterns are loaded on the fly through USB/I2C
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(DISP_MODE);
    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pMode = msg.text.data[0];
        return 0;
    }
    return -1;
}

int LCR_GetPowerMode(BOOL *Standby)
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(POWER_CONTROL);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *Standby = (msg.text.data[0] != 0);
        return 0;
    }
    return -1;
}

int LCR_SetPowerMode(unsigned char Mode)
/**
 * (I2C: 0x07)
 * (USB: CMD2: 0x02, CMD3: 0x00)
 * The Power Control places the DLPC900 in a low-power state and powers down the DMD interface.
 * Standby mode should only be enabled after all data for the last frame to be displayed has been
 * transferred to the DLPC900. Standby mode must be disabled prior to sending any new data.
 *
 * @param   Standby  - I - TRUE = Standby mode. Places DLPC900 in low power state and powers down the DMD interface
 *                         FALSE = Normal operation. The selected external source will be displayed
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    if (Mode > 0x2)
        return -1;
    msg.text.data[2] = Mode;
    LCR_PrepWriteCmd(&msg, POWER_CONTROL);

    return LCR_SendMsg(&msg);
}

int LCR_SetRedLEDStrobeDelay(BOOL invert, short rising, short falling)
/**
 * (I2C: 0x6C)
 * (USB: CMD2: 0x1A, CMD3: 0x1F)
 * The Red LED Enable Delay Control command sets the rising and falling edge delay of the Red LED enable signal.
 *
 * @param   invert  - I - Invert Red Led output
 * @param   rising  - I - Red LED enable rising edge delay control. Each bit adds 107.2 ns.
 *                        0x00 = -20.05 μs, 0x01 = -19.9428 μs, ......0xBB=0.00 μs, ......, 0xFE = +7.1828 μs, 0xFF = +7.29 μs
 * @param   falling  - I - Red LED enable falling edge delay control. Each bit adds 107.2 ns.
 *                        0x00 = -20.05 μs, 0x01 = -19.9428 μs, ......0xBB=0.00 μs, ......, 0xFE = +7.1828 μs, 0xFF = +7.29 μs
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = invert;
    msg.text.data[3] = rising & 0xFF;
    msg.text.data[4] = (rising >> 8) & 0xFF;
    msg.text.data[5] = falling & 0xFF;
    msg.text.data[6] = (falling >> 8) & 0xFF;

    LCR_PrepWriteCmd(&msg, RED_LED_ENABLE_DLY);

    return LCR_SendMsg(&msg);
}

int LCR_SetGreenLEDStrobeDelay(BOOL invert, short rising, short falling)
/**
 * (I2C: 0x6D)
 * (USB: CMD2: 0x1A, CMD3: 0x20)
 * The Green LED Enable Delay Control command sets the rising and falling edge delay of the Green LED enable signal.
 *
 * @param   invert  - I - Invert Green Led output
 * @param   rising  - I - Green LED enable rising edge delay control. Each bit adds 107.2 ns.
 *                        0x00 = -20.05 μs, 0x01 = -19.9428 μs, ......0xBB=0.00 μs, ......, 0xFE = +7.1828 μs, 0xFF = +7.29 μs
 * @param   falling  - I - Green LED enable falling edge delay control. Each bit adds 107.2 ns.
 *                        0x00 = -20.05 μs, 0x01 = -19.9428 μs, ......0xBB=0.00 μs, ......, 0xFE = +7.1828 μs, 0xFF = +7.29 μs
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = invert;
    msg.text.data[3] = rising & 0xFF;
    msg.text.data[4] = (rising >> 8) & 0xFF;
    msg.text.data[5] = falling & 0xFF;
    msg.text.data[6] = (falling >> 8) & 0xFF;

    LCR_PrepWriteCmd(&msg, GREEN_LED_ENABLE_DLY);

    return LCR_SendMsg(&msg);
}

int LCR_SetBlueLEDStrobeDelay(BOOL invert, short rising, short falling)
/**
 * (I2C: 0x6E)
 * (USB: CMD2: 0x1A, CMD3: 0x21)
 * The Blue LED Enable Delay Control command sets the rising and falling edge delay of the Blue LED enable signal.
 *
 * @param   invert  - I - Invert Red Led output
 * @param   rising  - I - Blue LED enable rising edge delay control. Each bit adds 107.2 ns.
 *                        0x00 = -20.05 μs, 0x01 = -19.9428 μs, ......0xBB=0.00 μs, ......, 0xFE = +7.1828 μs, 0xFF = +7.29 μs
 * @param   falling  - I - Blue LED enable falling edge delay control. Each bit adds 107.2 ns.
 *                        0x00 = -20.05 μs, 0x01 = -19.9428 μs, ......0xBB=0.00 μs, ......, 0xFE = +7.1828 μs, 0xFF = +7.29 μs
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = invert;
    msg.text.data[3] = rising & 0xFF;
    msg.text.data[4] = (rising >> 8) & 0xFF;
    msg.text.data[5] = falling & 0xFF;
    msg.text.data[6] = (falling >> 8) & 0xFF;

    LCR_PrepWriteCmd(&msg, BLUE_LED_ENABLE_DLY);

    return LCR_SendMsg(&msg);
}
int LCR_SetDMDSaverMode(short mode)
/**
 * (I2C: 0x0D)
 * (USB: CMD2: 0x02, CMD3: 0x01)
 * The Blue LED Enable Delay Control command sets the rising and falling edge delay of the Blue LED enable signal.
 *
 * @param   mode     - 1 byte : 0 – Disable DMD Saver Mode, 1 – Enable DMD Saver mode
 *
 *
 *
 * @return   0 = Saver Mode Disabled     <BR>
 *           1 = DMD Saver Mode Enabled   <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = mode;


    LCR_PrepWriteCmd(&msg, DMD_IDLE);
    return LCR_SendMsg(&msg);
    return 0;
}
int LCR_GetDMDSaverMode()
/**
 * (I2C: 0x0D)
 * (USB: CMD2: 0x02, CMD3: 0x01)
 * The Blue LED Enable Delay Control command sets the rising and falling edge delay of the Blue LED enable signal.
 *
 * @param   mode     - 1 byte : 0 – Disable DMD Saver Mode, 1 – Enable DMD Saver mode
 *
 *
 *
 * @return   0 = Saver Mode Disabled     <BR>
 *           1 = DMD Saver Mode Enabled   <BR>
 *
 */
{
    hidMessageStruct msg;

     LCR_PrepReadCmd(DMD_IDLE);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        short mode = msg.text.data[0];
        return mode;
    }

    return -1;
}
int LCR_GetRedLEDStrobeDelay(BOOL *pInvert, short *pRising, short *pFalling)
/**
 * (I2C: 0x6C)
 * (USB: CMD2: 0x1A, CMD3: 0x1F)
 * This command reads back the rising and falling edge delay of the Red LED enable signal.
 *
 * @param   pInvert  - O - Invert Red Led output
 * @param   pRising  - O - Red LED enable rising edge delay value. Each bit adds 107.2 ns.
 *                        0x00 = -20.05 μs, 0x01 = -19.9428 μs, ......0xBB=0.00 μs, ......, 0xFE = +7.1828 μs, 0xFF = +7.29 μs
 * @param   pFalling  - O - Red LED enable falling edge delay value. Each bit adds 107.2 ns.
 *                        0x00 = -20.05 μs, 0x01 = -19.9428 μs, ......0xBB=0.00 μs, ......, 0xFE = +7.1828 μs, 0xFF = +7.29 μs
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(RED_LED_ENABLE_DLY);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pInvert = msg.text.data[0];
        *pRising = msg.text.data[1] | (msg.text.data[2] << 8);
        *pFalling = msg.text.data[3] | (msg.text.data[4] << 8);
        return 0;
    }
    return -1;
}

int LCR_GetGreenLEDStrobeDelay(BOOL *pInvert, short *pRising, short *pFalling)
/**
 * (I2C: 0x6D)
 * (USB: CMD2: 0x1A, CMD3: 0x20)
 * This command reads back the rising and falling edge delay of the Green LED enable signal.
 *
 * @param   pInvert  - O - Invert Green Led output
 * @param   pRising  - O - Green LED enable rising edge delay value. Each bit adds 107.2 ns.
 *                        0x00 = -20.05 μs, 0x01 = -19.9428 μs, ......0xBB=0.00 μs, ......, 0xFE = +7.1828 μs, 0xFF = +7.29 μs
 * @param   pFalling  - O - Green LED enable falling edge delay value. Each bit adds 107.2 ns.
 *                        0x00 = -20.05 μs, 0x01 = -19.9428 μs, ......0xBB=0.00 μs, ......, 0xFE = +7.1828 μs, 0xFF = +7.29 μs
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(GREEN_LED_ENABLE_DLY);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pInvert = msg.text.data[0];
        *pRising = msg.text.data[1] | (msg.text.data[2] << 8);
        *pFalling = msg.text.data[3] | (msg.text.data[4] << 8);
        return 0;
    }
    return -1;
}

int LCR_GetBlueLEDStrobeDelay(BOOL *pInvert, short *pRising, short *pFalling)
/**
 * (I2C: 0x6E)
 * (USB: CMD2: 0x1A, CMD3: 0x21)
 * This command reads back the rising and falling edge delay of the Blue LED enable signal.
 *
 * @param   pInvert  - O - Invert Red Led output
 * @param   pRising  - O - Blue LED enable rising edge delay value. Each bit adds 107.2 ns.
 *                        0x00 = -20.05 μs, 0x01 = -19.9428 μs, ......0xBB=0.00 μs, ......, 0xFE = +7.1828 μs, 0xFF = +7.29 μs
 * @param   pFalling  - O - Blue LED enable falling edge delay value. Each bit adds 107.2 ns.
 *                        0x00 = -20.05 μs, 0x01 = -19.9428 μs, ......0xBB=0.00 μs, ......, 0xFE = +7.1828 μs, 0xFF = +7.29 μs
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(BLUE_LED_ENABLE_DLY);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pInvert = msg.text.data[0];
        *pRising = msg.text.data[1] | (msg.text.data[2] << 8);
        *pFalling = msg.text.data[3] | (msg.text.data[4] << 8);
        return 0;
    }
    return -1;
}

int LCR_SetInputSource(unsigned int source, unsigned int portWidth)
/**
 * (I2C: 0x00)
 * (USB: CMD2: 0x1A, CMD3: 0x00)
 * The Input Source Selection command selects the input source to be displayed by the DLPC900: 30-bit
 * Parallel Port, Internal Test Pattern, Flash memory, or FPD-link interface.
 *
 * @param   source  - I - Select the input source and interface mode:
 *                        0 = Parallel interface with 8-bit, 16-bit, 20-bit, 24-bit, or 30-bit RGB or YCrCb data formats
 *                        1 = Internal test pattern; Use LCR_SetTPGSelect() API to select pattern
 *                        2 = Flash. Images are 24-bit single-frame, still images stored in flash that are uploaded on command.
 *                        3 = FPD-link interface
 * @param   portWidth  - I - Parallel Interface bit depth
 *                           0 = 30-bits
 *                           1 = 24-bits
 *                           2 = 20-bits
 *                           3 = 16-bits
 *                           4 = 10-bits
 *                           5 = 8-bits
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = source;
    msg.text.data[2] |= portWidth << 3;
    LCR_PrepWriteCmd(&msg, SOURCE_SEL);

    return LCR_SendMsg(&msg);
}

int LCR_SetCurtainColor(unsigned int red,unsigned int green, unsigned int blue)
{
    hidMessageStruct msg;
    msg.text.data[2] = red & 0xff;
    msg.text.data[3] = (red >> 8) & 0x03;
    msg.text.data[4] = green & 0xff;
    msg.text.data[5] = (green >> 8) & 0x03;
    msg.text.data[6] = blue & 0xff;
    msg.text.data[7] = (blue >> 8) & 0x03;

    LCR_PrepWriteCmd(&msg, CURTAIN_COLOR);

    return LCR_SendMsg(&msg);
}

int LCR_GetCurtainColor(unsigned int *pRed,unsigned int *pGreen, unsigned int *pBlue)
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(CURTAIN_COLOR);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pRed   = msg.text.data[0] | msg.text.data[1]<<8;
        *pGreen = msg.text.data[2] | msg.text.data[3]<<8;
        *pBlue  = msg.text.data[4] | msg.text.data[5]<<8;

        return 0;
    }
    return -1;
}

int LCR_GetInputSource(unsigned int *pSource, unsigned int *pPortWidth)
/**
 * (I2C: 0x00)
 * (USB: CMD2: 0x1A, CMD3: 0x00)
 * Thisn command reads back the input source to be displayed by the DLPC900
 *
 * @param   pSource  - O - Input source and interface mode:
 *                        0 = Parallel interface with 8-bit, 16-bit, 20-bit, 24-bit, or 30-bit RGB or YCrCb data formats
 *                        1 = Internal test pattern; Use LCR_SetTPGSelect() API to select pattern
 *                        2 = Flash. Images are 24-bit single-frame, still images stored in flash that are uploaded on command.
 *                        3 = FPD-link interface
 * @param   pPortWidth  - O - Parallel Interface bit depth
 *                           0 = 30-bits
 *                           1 = 24-bits
 *                           2 = 20-bits
 *                           3 = 16-bits
 *                           4 = 10-bits
 *                           5 = 8-bits
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(SOURCE_SEL);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pSource = msg.text.data[0] & (BIT0 | BIT1 | BIT2);
        *pPortWidth = msg.text.data[0] >> 3;
        return 0;
    }
    return -1;
}

int LCR_SetPixelFormat(unsigned int format)
/**
 * (I2C: 0x02)
 * (USB: CMD2: 0x1A, CMD3: 0x02)
 * This API defines the pixel data format input into the DLPC900.Refer to programmer's guide for supported pixel formats
 * for each source type.
 *
 * @param   format  - I - Select the pixel data format:
 *                        0 = RGB 4:4:4 (30-bit)
 *                        1 = YCrCb 4:4:4 (30-bit)
 *                        2 = YCrCb 4:2:2
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = format;
    LCR_PrepWriteCmd(&msg, PIXEL_FORMAT);

    return LCR_SendMsg(&msg);
}

int LCR_GetPixelFormat(unsigned int *pFormat)
/**
 * (I2C: 0x02)
 * (USB: CMD2: 0x1A, CMD3: 0x02)
 * This API returns the defined the pixel data format input into the DLPC900.Refer to programmer's guide for supported pixel formats
 * for each source type.
 *
 * @param   pFormat  - O - Pixel data format:
 *                        0 = RGB 4:4:4 (30-bit)
 *                        1 = YCrCb 4:4:4 (30-bit)
 *                        2 = YCrCb 4:2:2
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(PIXEL_FORMAT);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pFormat = msg.text.data[0] & (BIT0 | BIT1 | BIT2);
        return 0;
    }
    return -1;
}

int LCR_SetDeGammaIndex(BOOL enable, unsigned int index)
/**
 * (I2C: 0x61)
 * (USB: CMD2: 0x1A, CMD3: 0x3A)
 * This API sets the enable state and the index of the de-Gamma LUT for display data in video mode
 *
 * @param   enable  - I - The enable state of DeGamma
 *
 * @param   index  - I - The table index that needs to be loaded
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = enable;
    msg.text.data[3] = index;
    LCR_PrepWriteCmd(&msg, DEGAMMA_SET);

    return LCR_SendMsg(&msg);
}

int LCR_GetDeGammaIndex(BOOL *pEnable, unsigned int *pIndex)
/**
 * (I2C: 0x61)
 * (USB: CMD2: 0x1A, CMD3: 0x3A)
 * This API returns the enable state of Degamma and the current table index
 *
 * @param   enable  - I - The enable state of DeGamma
 *
 * @param   index  - I - The current table index that is loaded
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(DEGAMMA_SET);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pEnable = msg.text.data[0] & (BIT0);
        *pIndex = msg.text.data[1] & (BIT0 | BIT1 | BIT2);
        return 0;
    }
    return -1;
}

int LCR_SetParallelPortConfig(uint16 TAPPL, uint16 TAPLF, uint16 AAFP, uint16 AAFL, uint16 AAPPL, uint16 AAPLF, uint16 BFFL, uint16 PixelClockFreq)
/**
 * (I2C: 0x62)
 * (USB: CMD2: 0x1A, CMD3: 0x3C)
 * This API sets the enable state and the index of the Parallel port for display data in video mode
 *
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

	msg.text.data[2] = TAPPL & 0xFF;
    msg.text.data[3] = (TAPPL >> 8) & 0xFF;
	
    msg.text.data[4] = TAPLF & 0xFF;
    msg.text.data[5] = (TAPLF >> 8) & 0xFF;
	
	msg.text.data[6] = AAPPL & 0xFF;
    msg.text.data[7] = (AAPPL >> 8) & 0xFF;
	
	msg.text.data[8] = AAPLF & 0xFF;
    msg.text.data[9] = (AAPLF >> 8) & 0xFF;
	
    msg.text.data[10] = AAFP & 0xFF;
    msg.text.data[11] = (AAFP >> 8) & 0xFF;
	
    msg.text.data[12] = AAFL & 0xFF;
    msg.text.data[13] = (AAFL >> 8) & 0xFF;
		
    msg.text.data[14] = BFFL & 0xFF;
    msg.text.data[15] = (BFFL >> 8) & 0xFF;
	
    msg.text.data[16] = PixelClockFreq & 0xFF;
    msg.text.data[17] = (PixelClockFreq >> 8) & 0xFF;
    msg.text.data[18] = (PixelClockFreq >> 16) & 0xFF;
    msg.text.data[19] = (PixelClockFreq >> 24) & 0xFF;
	

    LCR_PrepWriteCmd(&msg, PARALLEL_PORT);

    return LCR_SendMsg(&msg);
}

int LCR_GetParallelPortConfig(uint16 *TAPPL, uint16 *TAPLF, uint16 *AAFP, uint16 *AAFL, uint16 *AAPPL, uint16 *AAPLF, uint16 *BFFL, uint32 *PixelClockFreq)
/**
 * (I2C: 0x62)
 * (USB: CMD2: 0x1A, CMD3: 0x3C)
 * This API returns the values of Parallel port settings
 *
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(PARALLEL_PORT);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
		
		
		*TAPPL = msg.text.data[0] | (msg.text.data[1]<<8);
        *TAPLF = msg.text.data[2] | (msg.text.data[3]<<8);
		*AAPPL = msg.text.data[4] | (msg.text.data[5]<<8);
		*AAPLF = msg.text.data[6] | (msg.text.data[7]<<8);
        *AAFP  = msg.text.data[8] | (msg.text.data[9]<<8);
        *AAFL  = msg.text.data[10] | (msg.text.data[11]<<8);
        *BFFL  = msg.text.data[12] | (msg.text.data[13]<<8);
        *PixelClockFreq = msg.text.data[14] | (msg.text.data[15]<<8) | (msg.text.data[16]<<16) | (msg.text.data[17]<<24);
        
		return 0;
    }
    return -1;
}

int LCR_SetPortConfig(unsigned int dataPort,unsigned int pixelClock,unsigned int dataEnable,unsigned int syncSelect)
/**
 * (I2C: 0x03)
 * (USB: CMD2: 0x1A, CMD3: 0x03)
 * This command sets on which port the RGB data is on and which pixel clock, data enable,
 * and syncs to use.
 *
 * @param   dataPort     0 - Data Port 1 (Valid when bit 0 = 0)Single Pixel mode
 *                       1 - Data Port 2 (Valid when bit 0 = 0)Single Pixel mode
 *                       2 - Data Port 1-2 (Valid when bit 0 = 1. Even pixel on port 1, Odd pixel on port 2)Dual Pixel mode
 *                       3 - Data Port 2-1 (Valid when bit 0 = 1. Even pixel on port 2, Odd pixel on port 1)Dual Pixel mode
 * @param   pixelClock   0 - Pixel Clock 1
 *                       1 - Pixel Clock 2
 *                       2 - Pixel Clock 3
 * @param   dataEnable   0 - Data Enable 1
 *                       1 - Data Enable 2
 * @param   syncSelect   0 - P1 VSync and P1 HSync
 *                       1 - P2 VSync and P2 HSync
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    unsigned int data = 0;
    data = dataPort | (pixelClock << 2) | (dataEnable << 4) | (syncSelect << 5);

    hidMessageStruct msg;

    msg.text.data[2] = data;
    LCR_PrepWriteCmd(&msg, CLK_SEL);

    return LCR_SendMsg(&msg);
}

int LCR_GetPortConfig(unsigned int *pDataPort,unsigned int *pPixelClock,unsigned int *pDataEnable,unsigned int *pSyncSelect)
/**
 * (I2C: 0x03)
 * (USB: CMD2: 0x1A, CMD3: 0x03)
 * This command reads on which port the RGB data is on and which pixel clock, data enable,
 * and syncs is used.
 *
 * @param   pDataPort     0 - Data Port 1 (Valid when bit 0 = 0)Single Pixel mode
 *                       1 - Data Port 2 (Valid when bit 0 = 0)Single Pixel mode
 *                       2 - Data Port 1-2 (Valid when bit 0 = 1. Even pixel on port 1, Odd pixel on port 2)Dual Pixel mode
 *                       3 - Data Port 2-1 (Valid when bit 0 = 1. Even pixel on port 2, Odd pixel on port 1)Dual Pixel mode
 * @param   pPixelClock   0 - Pixel Clock 1
 *                       1 - Pixel Clock 2
 *                       2 - Pixel Clock 3
 * @param   pDataEnable   0 - Data Enable 1
 *                       1 - Data Enable 2
 * @param   pSyncSelect   0 - P1 VSync and P1 HSync
 *                       1 - P2 VSync and P2 HSync
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(CLK_SEL);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pDataPort = (msg.text.data[0] & (BIT0 | BIT1));
        *pPixelClock = (msg.text.data[0] & (BIT2 | BIT3)) >> 2;
        *pDataEnable = (msg.text.data[0] & BIT4) >> 4;
        *pSyncSelect = (msg.text.data[0] & BIT5) >> 5;

        return 0;
    }
    return -1;
}

int LCR_SetDataChannelSwap(unsigned int port, unsigned int swap)
/**
 * (I2C: 0x04)
 * (USB: CMD2: 0x1A, CMD3: 0x37)
 * This API configures the specified input data port and map the data subchannels.
 * The DLPC900 interprets Channel A as Green, Channel B as Red, and Channel C as Blue.
 *
 * @param   port  - I - Selects the port:
 *                        0 = Port1, parallel interface
 *                        1 = Port2, FPD-link interface
 * @param   swap - I - Swap Data Sub-Channel:
 *                     0 - ABC = ABC, No swapping of data sub-channels
 *                     1 - ABC = CAB, Data sub-channels are right shifted and circularly rotated
 *                     2 - ABC = BCA, Data sub-channels are left shifted and circularly rotated
 *                     3 - ABC = ACB, Data sub-channels B and C are swapped
 *                     4 - ABC = BAC, Data sub-channels A and B are swapped
 *                     5 - ABC = CBA, Data sub-channels A and C are swapped
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = port;
    msg.text.data[2] |= (swap & (BIT0 | BIT1 | BIT2))<<1;
    LCR_PrepWriteCmd(&msg, CHANNEL_SWAP);

    return LCR_SendMsg(&msg);
}

int LCR_GetDataChannelSwap(unsigned int Port, unsigned int *pSwap)
/**
 * (I2C: 0x04)
 * (USB: CMD2: 0x1A, CMD3: 0x37)
 * This API reads the data subchannel mapping for the specified input data port and map the data subchannels
 *
 * @param   Port  - O - Selected port:
 *                        0 = Port1, parallel interface
 *                        1 = Port2, FPD-link interface
 * @param   *pSwap - O - Swap Data Sub-Channel:
 *                     0 - ABC = ABC, No swapping of data sub-channels
 *                     1 - ABC = CAB, Data sub-channels are right shifted and circularly rotated
 *                     2 - ABC = BCA, Data sub-channels are left shifted and circularly rotated
 *                     3 - ABC = ACB, Data sub-channels B and C are swapped
 *                     4 - ABC = BAC, Data sub-channels A and B are swapped
 *                     5 - ABC = CBA, Data sub-channels A and C are swapped
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmdWithParam(CHANNEL_SWAP, (unsigned char)Port);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pSwap = (msg.text.data[0] & (BIT1 | BIT2 | BIT3))>>1;
        return 0;
    }
    return -1;
}

int LCR_SetFPD_Mode_Field(unsigned int PixelMappingMode, BOOL SwapPolarity, unsigned int FieldSignalSelect)
/**
 * (I2C: 0x05)
 * (USB: CMD2: 0x1A, CMD3: 0x04)
 * The FPD-Link Mode and Field Select command configures the FPD-link pixel map, polarity, and signal select.
 *
 * @param   PixelMappingMode  - I - FPD-link Pixel Mapping Mode: See table 2-21 in programmer's guide for more details
 *                                  0 = Mode 1
 *                                  1 = Mode 2
 *                                  2 = Mode 3
 *                                  3 = Mode 4
 * @param   SwapPolarity - I - Polarity select
 *                             TRUE = swap polarity
 *                             FALSE = do not swap polarity
 *
 * @param   FieldSignalSelect -I -  Field Signal Select
 *                              0 - Map FPD-Link output from CONT1 onto Field Signal for FPD-link interface port
 *                              1 - Map FPD-Link output from CONT2 onto Field Signal for FPD-link interface port
 *                              2 - Force 0 onto Field Signal for FPD-link interface port
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = PixelMappingMode << 6;
    msg.text.data[2] |= FieldSignalSelect & (BIT0 | BIT1 | BIT2);
    if(SwapPolarity)
        msg.text.data[2] |= BIT3;
    LCR_PrepWriteCmd(&msg, FPD_MODE);

    return LCR_SendMsg(&msg);
}

int LCR_GetFPD_Mode_Field(unsigned int *pPixelMappingMode, BOOL *pSwapPolarity, unsigned int *pFieldSignalSelect)
/**
 * (I2C: 0x05)
 * (USB: CMD2: 0x1A, CMD3: 0x04)
 * This command reads back the configuration of FPD-link pixel map, polarity, and signal select.
 *
 * @param   PixelMappingMode  - O - FPD-link Pixel Mapping Mode: See table 2-21 in programmer's guide for more details
 *                                  0 = Mode 1
 *                                  1 = Mode 2
 *                                  2 = Mode 3
 *                                  3 = Mode 4
 * @param   SwapPolarity - O - Polarity select
 *                             TRUE = swap polarity
 *                             FALSE = do not swap polarity
 *
 * @param   FieldSignalSelect - O -  Field Signal Select
 *                              0 - Map FPD-Link output from CONT1 onto Field Signal for FPD-link interface port
 *                              1 - Map FPD-Link output from CONT2 onto Field Signal for FPD-link interface port
 *                              2 - Force 0 onto Field Signal for FPD-link interface port
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(FPD_MODE);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pFieldSignalSelect = msg.text.data[0] & (BIT0 | BIT1 | BIT2);
        if(msg.text.data[0] & BIT3)
            *pSwapPolarity = 1;
        else
            *pSwapPolarity = 0;
        *pPixelMappingMode = msg.text.data[0] >> 6;
        return 0;
    }
    return -1;
}

int LCR_SetTPGSelect(unsigned int pattern)
/**
 * (I2C: 0x0A)
 * (USB: CMD2: 0x12, CMD3: 0x03)
 * When the internal test pattern is the selected input, the Internal Test Patterns Select defines the test
 * pattern displayed on the screen. These test patterns are internally generated and injected into the
 * beginning of the DLPC900 image processing path. Therefore, all image processing is performed on the
 * test images. All command registers should be set up as if the test images are input from an RGB 8:8:8
 * external source.
 *
 * @param   pattern  - I - Selects the internal test pattern:
 *                         0x0 = Solid Field
 *                         0x1 = Horizontal Ramp
 *                         0x2 = Vertical Ramp
 *                         0x3 = Horizontal Lines
 *                         0x4 = Diagonal Lines
 *                         0x5 = Vertical Lines
 *                         0x6 = Grid
 *                         0x7 = Checkerboard
 *                         0x8 = RGB Ramp
 *                         0x9 = Color Bars
 *                         0xA = Step Bars
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = pattern;
    LCR_PrepWriteCmd(&msg, TPG_SEL);

    return LCR_SendMsg(&msg);
}

int LCR_GetTPGSelect(unsigned int *pPattern)
/**
 * (I2C: 0x0A)
 * (USB: CMD2: 0x12, CMD3: 0x03)
 * This command reads back the selected internal test pattern.
 *
 * @param   pattern  - O - Selected internal test pattern:
 *                         0x0 = Solid Field
 *                         0x1 = Horizontal Ramp
 *                         0x2 = Vertical Ramp
 *                         0x3 = Horizontal Lines
 *                         0x4 = Diagonal Lines
 *                         0x5 = Vertical Lines
 *                         0x6 = Grid
 *                         0x7 = Checkerboard
 *                         0x8 = RGB Ramp
 *                         0x9 = Color Bars
 *                         0xA = Step Bars
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(TPG_SEL);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pPattern = msg.text.data[0] & (BIT0 | BIT1 | BIT2 | BIT3);
        return 0;
    }
    return -1;
}

int LCR_LoadSplash(unsigned int index)
/**
 * (I2C: 0x7F)
 * (USB: CMD2: 0x1A, CMD3: 0x39)
 * This command loads an image from flash memory and then performs a buffer swap to display the loaded
 * image on the DMD.
 *
 * @param   index  - I - Image Index. Loads the image at this index from flash.
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = index;
    LCR_PrepWriteCmd(&msg, SPLASH_LOAD);

    return LCR_SendMsg(&msg);
}

int LCR_GetSplashIndex(unsigned int *pIndex)
/**
 * (I2C: 0x7F)
 * (USB: CMD2: 0x1A, CMD3: 0x39)
 * This command loads reads back the index that was loaded most recently via LCR_LoadSplash() API.
 *
 * @param   *pIndex  - O - Image Index. Image at this index is loaded from flash.
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(SPLASH_LOAD);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pIndex = msg.text.data[0];
        return 0;
    }
    return -1;
}

int LCR_SetDisplay(rectangle croppedArea, rectangle displayArea)
/**
 * (I2C: 0x7E)
 * (USB: CMD2: 0x10, CMD3: 0x00)
 * The Input Display Resolution command defines the active input resolution and active output (displayed)
 * resolution. The maximum supported input and output resolutions for the DLP4500 0.45 WXGA DMD is
 * 1280 pixels (columns) by 800 lines (rows). This command provides the option to define a subset of active
 * input frame data using pixel (column) and line (row) counts relative to the source-data enable signal
 * (DATEN). In other words, this feature allows the source image to be cropped as the first step in the
 * processing chain.
 *
 * @param croppedArea - I - The rectagle structure contains the following
 *          parameters to describe the area to be cropped:
 *              - FirstPixel <BR>
 *              - FirstLine <BR>
 *              - PixelsPerLine <BR>
 *              - LinesPerFrame <BR>
 * @param displayArea - I - The rectagle structure contains the following
 *          parameters to describe the display area:
 *              - FirstPixel <BR>
 *              - FirstLine <BR>
 *              - PixelsPerLine <BR>
 *              - LinesPerFrame <BR>
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = croppedArea.firstPixel & 0xFF;
    msg.text.data[3] = croppedArea.firstPixel >> 8;
    msg.text.data[4] = croppedArea.firstLine & 0xFF;
    msg.text.data[5] = croppedArea.firstLine >> 8;
    msg.text.data[6] = croppedArea.pixelsPerLine & 0xFF;
    msg.text.data[7] = croppedArea.pixelsPerLine >> 8;
    msg.text.data[8] = croppedArea.linesPerFrame & 0xFF;
    msg.text.data[9] = croppedArea.linesPerFrame >> 8;
    msg.text.data[10] = displayArea.firstPixel & 0xFF;
    msg.text.data[11] = displayArea.firstPixel >> 8;
    msg.text.data[12] = displayArea.firstLine & 0xFF;
    msg.text.data[13] = displayArea.firstLine >> 8;
    msg.text.data[14] = displayArea.pixelsPerLine & 0xFF;
    msg.text.data[15] = displayArea.pixelsPerLine >> 8;
    msg.text.data[16] = displayArea.linesPerFrame & 0xFF;
    msg.text.data[17] = displayArea.linesPerFrame >> 8;

    LCR_PrepWriteCmd(&msg, DISP_CONFIG);

    return LCR_SendMsg(&msg);
}

int LCR_GetDisplay(rectangle *pCroppedArea, rectangle *pDisplayArea)
/**
 * (I2C: 0x7E)
 * (USB: CMD2: 0x10, CMD3: 0x00)
 * This command reads back the active input resolution and active output (displayed) resolution.
 *
 * @param *pCroppedArea - O - The rectagle structure contains the following
 *          parameters to describe the area to be cropped:
 *              - FirstPixel <BR>
 *              - FirstLine <BR>
 *              - PixelsPerLine <BR>
 *              - LinesPerFrame <BR>
 * @param *pDisplayArea - O - The rectagle structure contains the following
 *          parameters to describe the display area:
 *              - FirstPixel <BR>
 *              - FirstLine <BR>
 *              - PixelsPerLine <BR>
 *              - LinesPerFrame <BR>
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(DISP_CONFIG);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        pCroppedArea->firstPixel = msg.text.data[0] | msg.text.data[1] << 8;
        pCroppedArea->firstLine = msg.text.data[2] | msg.text.data[3] << 8;
        pCroppedArea->pixelsPerLine = msg.text.data[4] | msg.text.data[5] << 8;
        pCroppedArea->linesPerFrame = msg.text.data[6] | msg.text.data[7] << 8;
        pDisplayArea->firstPixel = msg.text.data[8] | msg.text.data[9] << 8;
        pDisplayArea->firstLine = msg.text.data[10] | msg.text.data[11] << 8;
        pDisplayArea->pixelsPerLine = msg.text.data[12] | msg.text.data[13] << 8;
        pDisplayArea->linesPerFrame = msg.text.data[14] | msg.text.data[15] << 8;

        return 0;
    }
    return -1;
}

int LCR_SetTPGColor(unsigned short redFG, unsigned short greenFG, unsigned short blueFG, unsigned short redBG, unsigned short greenBG, unsigned short blueBG)
/**
 * (I2C: 0x1A)
 * (USB: CMD2: 0x12, CMD3: 0x04)
 * When the internal test pattern is the selected input, the Internal Test Patterns Color Control defines the
 * colors of the test pattern displayed on the screen. The foreground color setting affects all test patterns. The background color
 * setting affects those test patterns that have a foreground and background component, such as, Horizontal
 * Lines, Diagonal Lines, Vertical Lines, Grid, and Checkerboard.
 *
 * @param   redFG  - I - Red Foreground Color intensity in a scale from 0 to 1023
 *                       0x0 = No Red Foreground color intensity
 *                       0x3FF = Full Red Foreground color intensity
 * @param   greenFG  - I - Green Foreground Color intensity in a scale from 0 to 1023
 *                       0x0 = No Green Foreground color intensity
 *                       0x3FF = Full Green Foreground color intensity
 * @param   blueFG  - I - Blue Foreground Color intensity in a scale from 0 to 1023
 *                       0x0 = No Blue Foreground color intensity
 *                       0x3FF = Full Blue Foreground color intensity
 * @param   redBG  - I - Red Foreground Color intensity in a scale from 0 to 1023
 *                       0x0 = No Red Foreground color intensity
 *                       0x3FF = Full Red Foreground color intensity
 * @param   greenBG  - I - Green Foreground Color intensity in a scale from 0 to 1023
 *                       0x0 = No Green Foreground color intensity
 *                       0x3FF = Full Red Foreground color intensity
 * @param   blueBG  - I - Red Foreground Color intensity in a scale from 0 to 1023
 *                       0x0 = No Blue Foreground color intensity
 *                       0x3FF = Full Blue Foreground color intensity
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = (char)redFG;
    msg.text.data[3] = (char)(redFG >> 8);
    msg.text.data[4] = (char)greenFG;
    msg.text.data[5] = (char)(greenFG >> 8);
    msg.text.data[6] = (char)blueFG;
    msg.text.data[7] = (char)(blueFG >> 8);
    msg.text.data[8] = (char)redBG;
    msg.text.data[9] = (char)(redBG >> 8);
    msg.text.data[10] = (char)greenBG;
    msg.text.data[11] = (char)(greenBG >> 8);
    msg.text.data[12] = (char)blueBG;
    msg.text.data[13] = (char)(blueBG >> 8);

    LCR_PrepWriteCmd(&msg, TPG_COLOR);

    return LCR_SendMsg(&msg);
}

int LCR_GetTPGColor(unsigned short *pRedFG, unsigned short *pGreenFG, unsigned short *pBlueFG, unsigned short *pRedBG, unsigned short *pGreenBG, unsigned short *pBlueBG)
/**
 * (I2C: 0x1A)
 * (USB: CMD2: 0x12, CMD3: 0x04)
 * When the internal test pattern is the selected input, the Internal Test Patterns Color Control defines the
 * colors of the test pattern displayed on the screen. The foreground color setting affects all test patterns. The background color
 * setting affects those test patterns that have a foreground and background component, such as, Horizontal
 * Lines, Diagonal Lines, Vertical Lines, Grid, and Checkerboard.
 *
 * @param   *pRedFG  - O - Red Foreground Color intensity in a scale from 0 to 1023
 *                       0x0 = No Red Foreground color intensity
 *                       0x3FF = Full Red Foreground color intensity
 * @param   *pGreenFG  - O - Green Foreground Color intensity in a scale from 0 to 1023
 *                       0x0 = No Green Foreground color intensity
 *                       0x3FF = Full Green Foreground color intensity
 * @param   *pBlueFG  - O - Blue Foreground Color intensity in a scale from 0 to 1023
 *                       0x0 = No Blue Foreground color intensity
 *                       0x3FF = Full Blue Foreground color intensity
 * @param   *pRedBG  - O - Red Foreground Color intensity in a scale from 0 to 1023
 *                       0x0 = No Red Foreground color intensity
 *                       0x3FF = Full Red Foreground color intensity
 * @param   *pGreenBG  - O - Green Foreground Color intensity in a scale from 0 to 1023
 *                       0x0 = No Green Foreground color intensity
 *                       0x3FF = Full Red Foreground color intensity
 * @param   *pBlueBG  - O - Red Foreground Color intensity in a scale from 0 to 1023
 *                       0x0 = No Blue Foreground color intensity
 *                       0x3FF = Full Blue Foreground color intensity
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(TPG_COLOR);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pRedFG = msg.text.data[0] | msg.text.data[1] << 8;
        *pGreenFG = msg.text.data[2] | msg.text.data[3] << 8;
        *pBlueFG = msg.text.data[4] | msg.text.data[5] << 8;
        *pRedBG = msg.text.data[6] | msg.text.data[7] << 8;
        *pGreenBG = msg.text.data[8] | msg.text.data[9] << 8;
        *pBlueBG = msg.text.data[10] | msg.text.data[11] << 8;

        return 0;
    }
    return -1;
}

int LCR_ClearPatLut(void)
/**
 * This API does not send any commands to the controller.It clears the locally (in the GUI program) stored pattern LUT.
 * See table 2-65 in programmer's guide for detailed desciprtion of pattern LUT entries.
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    PatLutIndex = 0;
    return 0;
}

int LCR_AddToPatLut(int PatNum, int ExpUs, BOOL ClearPat, int BitDepth, int LEDSelect, BOOL WaitForTrigger, int DarkTime, BOOL DisTrigOut2, int SplashIndex, int BitIndex)
/**
 * This API does not send any commands to the controller.
 * It makes an entry (appends) in the locally stored (in the GUI program) pattern LUT as per the input arguments passed to this function.
 * See table 2-65 in programmer's guide for detailed desciprtion of pattern LUT entries.
 *
 * @param   PatNum  - I - Pattern number (0 based index). For pattern number 0x3F, there is no
 *                          pattern display. The maximum number supported is 24 for 1 bit-depth
 *                          patterns. Setting the pattern number to be 25, with a bit-depth of 1 will insert
 *                          a white-fill pattern. Inverting this pattern will insert a black-fill pattern. These w
 *                          patterns will have the same exposure time as defined in the Pattern Display
 *                          Exposure and Frame Period command. Table 2-66 in the programmer's guide illustrates which bit
 *                          planes are illuminated by each pattern number.
 * @param   ExpUs  - I - Exposure time of the pattern in us
 *
 * @param   ClearPat  - I - Clear the pattern after exposure (only applicable for 1 bit patterns)
 *                          0 = Pattern not cleared
 *                          1 = Pattern cleared
 *
 * @param   BitDepth  - I - Select desired bit-depth
 *                          0 = Reserved
 *                          1 = 1-bit
 *                          2 = 2-bit
 *                          3 = 3-bit
 *                          4 = 4-bit
 *                          5 = 5-bit
 *                          6 = 6-bit
 *                          7 = 7-bit
 *                          8 = 8-bit
 *                          10 = 10-bit
 *                          12 = 12-bit
 *                          14 = 14-bit
 *                          16 = 16-bit
 * @param   LEDSelect  - I -  Choose the LEDs that are on: b0 = Red, b1 = Green, b2 = Blue
 *                          0 = No LED (Pass Through)
 *                          1 = Red
 *                          2 = Green
 *                          3 = Yellow (Green + Red)
 *                          4 = Blue
 *                          5 = Magenta (Blue + Red)
 *                          6 = Cyan (Blue + Green)
 *                          7 = White (Red + Blue + Green)
 * @param   WaitForTrigger  - I - Wait for trigger input for displaying pattern
 *                          0 = Pattern does not wait for trigger input to get displayed
 *                          1 = Pattern waits for trigger input to get displayed
 * @param   DarkTime  - I -  Dark time of the pattern in us
 * @param   DisTrigOut2  - I - Disable trigger output enable while displaying pattern
 *                          0 = Enable trigger output for this pattern
 *                          1 = Disable trigger output for this pattern
 * @param   SplashIndex  - I -  Index of the splash image where the pattern exists in flash
 * @param   BitIndex  - I -  Index of the bit in splash image where the pattern to be displayed starts
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    unsigned char lutByte = 0;
    unsigned char extendedBitdepth = 0;
    unsigned short lutShort = 0;

    if( (BitDepth > 16) || (BitDepth < 1))
        return -1;

    if(LEDSelect > 7)
        return -1;

    PatLut[0][PatLutIndex] = PatNum & 0xFF;
    PatLut[1][PatLutIndex] = (PatNum >> 8) & 0xFF;
    PatLut[2][PatLutIndex] = ExpUs & 0xFF;
    PatLut[3][PatLutIndex] = (ExpUs >> 8) & 0xFF;
    PatLut[4][PatLutIndex] = (ExpUs >> 16) & 0xFF;

    if (ClearPat)
        lutByte = BIT0;

    lutByte |= ((BitDepth - 1) & 0x7) << 1;
    if(BitDepth > 8)
    {
        extendedBitdepth = 1;
    }

    lutByte |= (LEDSelect & 0x7) << 4;
    if(WaitForTrigger)
        lutByte |= BIT7;

    PatLut[5][PatLutIndex] = lutByte;
    PatLut[6][PatLutIndex] = DarkTime & 0xFF;
    PatLut[7][PatLutIndex] = (DarkTime >> 8) & 0xFF;
    PatLut[8][PatLutIndex] = (DarkTime >> 16) & 0xFF;

    lutByte = (extendedBitdepth) << 1;
    if (!DisTrigOut2)
        lutByte |= BIT0;

    PatLut[9][PatLutIndex] = lutByte;

    lutShort = SplashIndex & 0x7FF;
    lutShort |= (BitIndex & 0x1F) << 11;
    printf("BitIndex: %d,", BitIndex);
    printf("lutShort: %x,", lutShort);

    PatLut[10][PatLutIndex] = lutShort & 0xFF;
    PatLut[11][PatLutIndex] = (lutShort >> 8) & 0xFF;

    PatLutIndex++;

    return 0;
}


int LCR_OpenMailbox(int MboxNum)
/**
 * (I2C: 0x77)
 * (USB: CMD2: 0x1A, CMD3: 0x33)
 * This API opens the specified Mailbox within the DLPC900 controller. This API must be called
 * before sending data to the mailbox/LUT using LCR_SendPatLut() or LCR_SendSplashLut() APIs.
 *
 * @param MboxNum - I - 1 = Open the mailbox for image index configuration
 *                      2 = Open the mailbox for pattern definition.
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = MboxNum;
    LCR_PrepWriteCmd(&msg, MBOX_CONTROL);

    return LCR_SendMsg(&msg);
}

int LCR_CloseMailbox(void)
/**
 * (I2C: 0x77)
 * (USB: CMD2: 0x1A, CMD3: 0x33)
 * This API is internally used by other APIs within this file. There is no need for user application to
 * call this API separately.
 * This API closes all the Mailboxes within the DLPC900 controller.
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = 0;
    LCR_PrepWriteCmd(&msg, MBOX_CONTROL);

    return LCR_SendMsg(&msg);
}

int LCR_MailboxSetAddr(int Addr)
/**
 * (I2C: 0x76)
 * (USB: CMD2: 0x1A, CMD3: 0x32)
 * This API defines the offset location within the DLPC900 mailboxes to write data into or to read data from
 *
 * @param Addr - I - 0-127 - Defines the offset within the selected (opened) LUT to write/read data to/from.
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    if(Addr > 127)
        return -1;

    msg.text.data[2] = Addr;
    msg.text.data[3] = 0x0;
    LCR_PrepWriteCmd(&msg, MBOX_ADDRESS);

    return LCR_SendMsg(&msg);
}

int LCR_SendPatLut(void)
/**
 * (I2C: 0x78)
 * (USB: CMD2: 0x1A, CMD3: 0x34)
 * This API sends the pattern LUT created by calling LCR_AddToPatLut() API to the DLPC900 controller.
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    unsigned int i;

    for(i=0; i<PatLutIndex; i++)
    {
		int j;
        hidMessageStruct msg;

        CmdList[MBOX_DATA].len = 12;
        LCR_PrepWriteCmd(&msg, MBOX_DATA);

        for (j = 0; j < 12; j++)
            msg.text.data[2 + j] = PatLut[j][i];

        if (LCR_SendMsg(&msg) < 0)
            return -2;
    }

    return 0;
}

int LCR_SendPatReorderUpdate(uint8* payload, int size)
/**
 * (I2C: 0x76)
 * (USB: CMD2: 0x1A, CMD3: 0x32)
 * This API sends the pattern LUT reorder created by calling LCR_AddToPatReorder() API to the DLPC900 controller.
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    /* TODO: If size exceeds 64 bytes send multiple times */
    int i;
    hidMessageStruct msg;

    CmdList[MBOX_ADDRESS].len = size ;
    LCR_PrepWriteCmd(&msg, MBOX_ADDRESS);

    for (i = 0; i < size; i++)
        msg.text.data[2 + i] = payload[i];

    if (LCR_SendMsg(&msg) < 0)
        return -2;


    return 0;
}

int LCR_PatternDisplay(int Action)
/**
 * (I2C: 0x65)
 * (USB: CMD2: 0x1A, CMD3: 0x24)
 * This API starts or stops the programmed patterns sequence.
 *
 * @param   Action - I - Pattern Display Start/Stop Pattern Sequence
 *                          0 = Stop Pattern Display Sequence. The next "Start" command will
 *                              restart the pattern sequence from the beginning.
 *                          1 = Pause Pattern Display Sequence. The next "Start" command will
 *                              start the pattern sequence by re-displaying the current pattern in the sequence.
 *                          2 = Start Pattern Display Sequence
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = Action;
    LCR_PrepWriteCmd(&msg, PAT_START_STOP);

    return LCR_SendMsg(&msg);
}

int LCR_SetPatternConfig(unsigned int numLutEntries, unsigned int repeat)
/* (I2C: 0x75)
 * (USB: CMD2: 0x1A, CMD3: 0x31)
 * This API controls the execution of patterns stored in the lookup table.
 * Before using this API, stop the current pattern sequence using LCR_PatternDisplay() API
 * After calling this API, send the Validation command using the API LCR_ValidatePatLutData() before starting the pattern sequence
 *
 * @param   numLutEntries - I - Number of LUT entries
 * @param   repeat - I - 0 = execute the pattern sequence once; 1 = repeat the pattern sequnce.
 * @param   numPatsForTrigOut2 - I - Number of patterns to display(range 1 through 256).
 *                                   If in repeat mode, then this value dictates how often TRIG_OUT_2 is generated.
 * @param   numSplash - I - Number of Image Index LUT Entries(range 1 through 64).
 *                          This Field is irrelevant for Pattern Display Data Input Source set to a value other than internal.
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = numLutEntries & 0xFF;
    msg.text.data[3] = (numLutEntries >> 8) & 0xFF;
    msg.text.data[4] = repeat & 0xFF;
    msg.text.data[5] = (repeat >> 8) & 0xFF;
    msg.text.data[6] = (repeat >> 16) & 0xFF;
    msg.text.data[7] = (repeat >> 24) & 0xFF;
    LCR_PrepWriteCmd(&msg, PAT_CONFIG);

    msg.head.flags.reply = 1;

    return LCR_SendMsg(&msg);
}

int LCR_GetPatternConfig(unsigned int *pNumLutEntries, BOOL *pRepeat, unsigned int *pNumPatsForTrigOut2, unsigned int *pNumSplash)
/**
 * (I2C: 0x75)
 * (USB: CMD2: 0x1A, CMD3: 0x31)
 * This API controls the execution of patterns stored in the lookup table.
 * Before using this API, stop the current pattern sequence using LCR_PatternDisplay() API
 * After calling this API, send the Validation command using the API LCR_ValidatePatLutData() before starting the pattern sequence
 *
 * @param   *pNumLutEntries - O - Number of LUT entries
 * @param   *pRepeat - O - 0 = execute the pattern sequence once; 1 = repeat the pattern sequnce.
 * @param   *pNumPatsForTrigOut2 - O - Number of patterns to display(range 1 through 256).
 *                                   If in repeat mode, then this value dictates how often TRIG_OUT_2 is generated.
 * @param   *pNumSplash - O - Number of Image Index LUT Entries(range 1 through 64).
 *                          This Field is irrelevant for Pattern Display Data Input Source set to a value other than internal.
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(PAT_CONFIG);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pNumLutEntries = msg.text.data[0] + 1; /* +1 because the firmware gives 0-based indices (0 means 1) */
        *pRepeat = (msg.text.data[1] != 0);
        *pNumPatsForTrigOut2 = msg.text.data[2]+1;    /* +1 because the firmware gives 0-based indices (0 means 1) */
        *pNumSplash = msg.text.data[3]+1;     /* +1 because the firmware gives 0-based indices (0 means 1) */
        return 0;
    }
    return -1;
}

int LCR_SetTrigIn1Config(BOOL invert, unsigned int trigDelay)
/**
 * (I2C: 0x79)
 * (USB: CMD2: 0x1A, CMD3: 0x35)
 * The Trigger In1 command sets the rising edge delay of the DLPC900's TRIG_IN_1 signal compared to
 * when the pattern is displayed on the DMD. The polarity of TRIG_IN_1 is set in the lookup table of the
 * pattern sequence. Before executing this command, stop the current pattern sequence.
 *
 * @param   invert - I - Pattern advance on rising/falling edge
 * @param   trigDelay - I - Trigger 1 delay in micro seconds.
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = trigDelay & 0xFF;
    msg.text.data[3] = (trigDelay>>8) & 0xFF;
    msg.text.data[4] = invert;
    LCR_PrepWriteCmd(&msg, TRIG_IN1_CTL);

    return LCR_SendMsg(&msg);
}

int LCR_GetTrigIn1Config(BOOL *pInvert, unsigned int *pTrigDelay)
/**
 * (I2C: 0x79)
 * (USB: CMD2: 0x1A, CMD3: 0x35)
 * The Trigger In1 command sets the rising edge delay of the DLPC900's TRIG_IN_1 signal compared to
 * when the pattern is displayed on the DMD. The polarity of TRIG_IN_1 is set in the lookup table of the
 * pattern sequence. Before executing this command, stop the current pattern sequence.
 *
 * @param   pInvert - I - Pattern advance on rising/falling edge
 * @param   pTrigDelay - I - Trigger 1 delay in micro seconds.
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(TRIG_IN1_CTL);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        
        *pTrigDelay = msg.text.data[0] | msg.text.data[1] << 8;
        *pInvert = msg.text.data[2];
        return 0;
    }
    return -1;
}

int LCR_SetTrigIn2Config(BOOL invert)
/**
 * (I2C: 0x7A)
 * (USB: CMD2: 0x1A, CMD3: 0x36)
 * In Video Pattern and Pre-Stored Pattern modes, TRIG_IN_2 acts as a start or stop signal. If the sequence
 * was not already started already by a software command, the rising edge on TRIG_IN_2 signal input will
 * start or resume the pattern sequence. If the pattern sequence is active, the falling edge on TRIG_IN_2
 * signal input stops the pattern sequence. Before executing this command, stop the current pattern
 * sequence.
 *
 * @param   invert - 0 – Pattern started on rising edge stopped on falling edge
 *                   1 – Pattern started on falling edge stopped on rising edge
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = invert;
    LCR_PrepWriteCmd(&msg, TRIG_IN2_CTL);

    return LCR_SendMsg(&msg);
}

int LCR_GetTrigIn2Config(BOOL *pInvert)
/**
 * (I2C: 0x7A)
 * (USB: CMD2: 0x1A, CMD3: 0x36)
 * In Video Pattern and Pre-Stored Pattern modes, TRIG_IN_2 acts as a start or stop signal. If the sequence
 * was not already started already by a software command, the rising edge on TRIG_IN_2 signal input will
 * start or resume the pattern sequence. If the pattern sequence is active, the falling edge on TRIG_IN_2
 * signal input stops the pattern sequence. Before executing this command, stop the current pattern
 * sequence.
 *
 * @param   invert - 0 – Pattern started on rising edge stopped on falling edge
 *                   1 – Pattern started on falling edge stopped on rising edge
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(TRIG_IN2_CTL);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);

        *pInvert = msg.text.data[0];
        return 0;
    }
    return -1;
}

int LCR_SetTrigOutConfig(unsigned int trigOutNum, BOOL invert, short rising, short falling)
/**
 * (I2C: 0x6A)
 * (USB: CMD2: 0x1A, CMD3: 0x1D)
 * This API sets the polarity, rising edge delay, and falling edge delay of the DLPC900's TRIG_OUT_1 or TRIG_OUT_2 signal.
 * The delays are compared to when the pattern is displayed on the DMD. Before executing this command,
 * stop the current pattern sequence. After executing this command, call LCR_ValidatePatLutData() API before starting the pattern sequence.
 *
 * @param   trigOutNum - I - 1 = TRIG_OUT_1; 2 = TRIG_OUT_2
 * @param   invert - I - 0 = active High signal; 1 = Active Low signal
 * @param   rising - I - rising edge delay control. Each bit adds 107.2 ns
 *                      0x00 = -20.05 μs, 0x01 = -19.9428 μs, ......0xBB=0.00 μs, ......, 0xD4 = +2.68 μs, 0xD5 = +2.787 μs
 * @param   falling- I - falling edge delay control. Each bit adds 107.2 ns (This field is not applcable for TRIG_OUT_2)
 *                      0x00 = -20.05 μs, 0x01 = -19.9428 μs, ......0xBB=0.00 μs, ......, 0xD4 = +2.68 μs, 0xD5 = +2.787 μs
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = invert & 0x1;
    msg.text.data[3] = rising & 0xFF;
    msg.text.data[4] = (rising >> 8) & 0xFF;
    msg.text.data[5] = falling & 0xFF;
    msg.text.data[6] = (falling >> 8) & 0xFF;

    if(trigOutNum == 1)
        LCR_PrepWriteCmd(&msg, TRIG_OUT1_CTL);
    else if(trigOutNum==2)
        LCR_PrepWriteCmd(&msg, TRIG_OUT2_CTL);

    return LCR_SendMsg(&msg);
}

int LCR_GetTrigOutConfig(unsigned int trigOutNum, BOOL *pInvert, short *pRising, short *pFalling)
/**
 * (I2C: 0x6A)
 * (USB: CMD2: 0x1A, CMD3: 0x1D)
 * This API readsback the polarity, rising edge delay, and falling edge delay of the DLPC900's TRIG_OUT_1 or TRIG_OUT_2 signal.
 * The delays are compared to when the pattern is displayed on the DMD.
 *
 * @param   trigOutNum - I - 1 = TRIG_OUT_1; 2 = TRIG_OUT_2
 * @param   *pInvert - O - 0 = active High signal; 1 = Active Low signal
 * @param   *pRising - O - rising edge delay control. Each bit adds 107.2 ns
 *                      0x00 = -20.05 μs, 0x01 = -19.9428 μs, ......0xBB=0.00 μs, ......, 0xD4 = +2.68 μs, 0xD5 = +2.787 μs
 * @param   *pFalling- O - falling edge delay control. Each bit adds 107.2 ns (This field is not applcable for TRIG_OUT_2)
 *                      0x00 = -20.05 μs, 0x01 = -19.9428 μs, ......0xBB=0.00 μs, ......, 0xD4 = +2.68 μs, 0xD5 = +2.787 μs
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    if(trigOutNum == 1)
        LCR_PrepReadCmd(TRIG_OUT1_CTL);
    else if(trigOutNum==2)
        LCR_PrepReadCmd(TRIG_OUT2_CTL);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pInvert = (msg.text.data[0] != 0);
        *pRising = msg.text.data[1] | (msg.text.data[2] << 8);//| (0xff << 16) | (0xff << 24);
        *pFalling = msg.text.data[3] | (msg.text.data[4] << 8);// | (0xff << 16) | (0xff << 24);

        return 0;
    }
    return -1;
}


int LCR_SetInvertData(BOOL invert)
/**
 * (I2C: 0x74)
 * (USB: CMD2: 0x1A, CMD3: 0x30)
 * This API dictates how the DLPC900 interprets a value of 0 or 1 to control mirror position for displayed patterns.
 * Before executing this command, stop the current pattern sequence. After executing this command, call
 * LCR_ValidatePatLutData() API before starting the pattern sequence.
 *
 * @param   invert - I - Pattern Display Invert Data
 *                      0 = Normal operation. A data value of 1 will flip the mirror to output light,
 *                          while a data value of 0 will flip the mirror to block light
 *                      1 = Inverted operation. A data value of 0 will flip the mirror to output light,
 *                          while a data value of 1 will flip the mirror to block light
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = invert;
    LCR_PrepWriteCmd(&msg, INVERT_DATA);

    return LCR_SendMsg(&msg);
}

int LCR_GetInvertData(BOOL *pInvert)
/**
 * (I2C: 0x74)
 * (USB: CMD2: 0x1A, CMD3: 0x30)
 * This API dictates how the DLPC900 interprets a value of 0 or 1 to control mirror position for displayed patterns.
 * Before executing this command, stop the current pattern sequence. After executing this command, call
 * LCR_ValidatePatLutData() API before starting the pattern sequence.
 *
 * @param   invert - I - Pattern Display Invert Data
 *                      0 = Normal operation. A data value of 1 will flip the mirror to output light,
 *                          while a data value of 0 will flip the mirror to block light
 *                      1 = Inverted operation. A data value of 0 will flip the mirror to output light,
 *                          while a data value of 1 will flip the mirror to block light
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;
    LCR_PrepReadCmd(INVERT_DATA);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pInvert = msg.text.data[0];
        return 0;
    }
    return -1;
}

int LCR_SetPWMConfig(unsigned int channel, unsigned int pulsePeriod, unsigned int dutyCycle)
/**
 * (I2C: 0x41)
 * (USB: CMD2: 0x1A, CMD3: 0x11)
 * This API sets the clock period and duty cycle of the specified PWM channel. The PWM
 * frequency and duty cycle is derived from an internal 18.67MHz clock. To calculate the desired PWM
 * period, divide the desired clock frequency from the internal 18.67Mhz clock. For example, a PWM
 * frequency of 2kHz, requires pulse period to be set to 18666667 / 2000 = 9333.
 *
 * @param   channel - I - PWM Channel Select
 *                      0 - PWM channel 0 (GPIO_0)
 *                      1 - Reserved
 *                      2 - PWM channel 2 (GPIO_2)
 *
 * @param   pulsePeriod - I - Clock Period in increments of 53.57ns. Clock Period = (value + 1) * 53.5ns
 *
 * @param   dutyCycle - I - Duty Cycle = (value + 1)% Value range is 1%-99%
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = channel;
    msg.text.data[3] = pulsePeriod;
    msg.text.data[4] = pulsePeriod >> 8;
    msg.text.data[5] = pulsePeriod >> 16;
    msg.text.data[6] = pulsePeriod >> 24;
    msg.text.data[7] = dutyCycle;

    LCR_PrepWriteCmd(&msg, PWM_SETUP);

    return LCR_SendMsg(&msg);
}

int LCR_GetPWMConfig(unsigned int channel, unsigned int *pPulsePeriod, unsigned int *pDutyCycle)
/**
 * (I2C: 0x41)
 * (USB: CMD2: 0x1A, CMD3: 0x11)
 * This API reads back the clock period and duty cycle of the specified PWM channel. The PWM
 * frequency and duty cycle is derived from an internal 18.67MHz clock. To calculate the desired PWM
 * period, divide the desired clock frequency from the internal 18.67Mhz clock. For example, a PWM
 * frequency of 2kHz, requires pulse period to be set to 18666667 / 2000 = 9333.
 *
 * @param   channel - I - PWM Channel Select
 *                      0 - PWM channel 0 (GPIO_0)
 *                      1 - Reserved
 *                      2 - PWM channel 2 (GPIO_2)
 *
 * @param   *pPulsePeriod - O - Clock Period in increments of 53.57ns. Clock Period = (value + 1) * 53.5ns
 *
 * @param   *pDutyCycle - O - Duty Cycle = (value + 1)% Value range is 1%-99%
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmdWithParam(PWM_SETUP, (unsigned char)channel);
    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pPulsePeriod = msg.text.data[1] | msg.text.data[2] << 8 | msg.text.data[3] << 16 | msg.text.data[4] << 24;
        *pDutyCycle = msg.text.data[5];
        return 0;
    }
    return -1;
}


int LCR_GetBusyGPIOConfig(unsigned char *value)
/**
 * (I2C: 0x5E)
 * (USB: CMD2: 0x1A, CMD3: 0x5E)
 * This API reads back the busy GPIO configuration.
 *
 * @param   *value - O - Bit 7 is Busy GPIO Enable
 *                       Bit 6: 0 - Busy GPIO Index
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(BUSY_GPIO_CONFIG);
    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *value = msg.text.data[0] ;
        return 0;
    }
    return -1;
}

int LCR_SetBusyGPIOConfig(unsigned char value)
/**
 * (I2C: 0x5E)
 * (USB: CMD2: 0x1A, CMD3: 0x5E)
 * This API writes the busy GPIO configuration.
 *
 * @param   value - O - Bit 7 is Busy GPIO Enable
 *                      Bit 6: 0 - Busy GPIO Index
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;
    msg.text.data[2] = value;
    LCR_PrepWriteCmd(&msg, BUSY_GPIO_CONFIG);

    return LCR_SendMsg(&msg);
}

int LCR_GetStandbyDelaySec(unsigned char *value)
/**
 * (I2C: 0x66)
 * (USB: CMD2: 0x1A, CMD3: 0x25)
 * This API reads back the Standby Delay in Seconds.
 *
 * @param   *value - O - Standby Delay in Seconds
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(STANDBY_DELAY);
    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *value = msg.text.data[0] ;
        return 0;
    }
    return -1;
}


int LCR_SetFlashTest(unsigned int offset, unsigned int size)
/**
 * (I2C: 0x5F)
 * (USB: CMD2: 0x1A, CMD3: 0x3A)
 * This API runs the flash test.
 *
 * @param   offset - O - offset of Flash to start the test
 * @param   size - O - size of Flash the test needs to be run
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;
    msg.text.data[2] = offset;
    msg.text.data[3] = offset >> 8;
    msg.text.data[4] = offset >> 16;
    msg.text.data[5] = offset >> 24;
    msg.text.data[6] = size;
    msg.text.data[7] = size >> 8;
    msg.text.data[8] = size >> 16;
    msg.text.data[9] = size >> 24;
    LCR_PrepWriteCmd(&msg, FLASH_TEST);

    return LCR_SendMsg(&msg);
}

int LCR_SetPWMEnable(unsigned int channel, BOOL Enable)
/**
 * (I2C: 0x40)
 * (USB: CMD2: 0x1A, CMD3: 0x10)
 * After the PWM Setup command configures the clock period and duty cycle, the PWM Enable command
 * activates the PWM signals.
 *
 * @param   channel - I - PWM Channel Select
 *                      0 - PWM channel 0 (GPIO_0)
 *                      1 - PWM channel 1 (GPIO_1)
 *                      2 - PWM channel 2 (GPIO_2)
 *                      3 - PWM channel 3 (GPIO_3)
 *
 * @param   Enable - I - PWM Channel enable 0=disable; 1=enable
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;
    unsigned char value = 0;

    if (channel > 3)
        return -1;
    else
        value = channel;

    if(Enable)
        value |= BIT7;

    msg.text.data[2] = value;
    LCR_PrepWriteCmd(&msg, PWM_ENABLE);

    return LCR_SendMsg(&msg);
}

int LCR_GetPWMEnable(unsigned int channel, BOOL *pEnable)
/**
 * (I2C: 0x40)
 * (USB: CMD2: 0x1A, CMD3: 0x10)
 * Reads back the enabled/disabled status of the given PWM channel.
 *
 * @param   channel - I - PWM Channel Select
 *                      0 - PWM channel 0 (GPIO_0)
 *                      1 - PWM channel 1 (GPIO_1)
 *                      2 - PWM channel 2 (GPIO_2)
 *                      3 - PWM channel 3 (GPIO_3)
 *
 * @param   *pEnable - O - PWM Channel enable 0=disable; 1=enable
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmdWithParam(PWM_ENABLE, (unsigned char)channel);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        if(msg.text.data[0] & BIT7)
            *pEnable =  TRUE;
        else
            *pEnable = FALSE;

        return 0;
    }
    return -1;
}

int LCR_SetPWMCaptureConfig(unsigned int channel, BOOL enable, unsigned int sampleRate)
/**
 * (I2C: 0x43)
 * (USB: CMD2: 0x1A, CMD3: 0x12)
 * This API samples the specified PWM input signals and returns the PWM clock period.
 *
 * @param   channel - I - PWM Capture Port
 *                      0 - PWM input channel 0 (GPIO_5)
 *                      1 - PWM input channel 1 (GPIO_6)
 *
 * @param   enable - I - PWM Channel enable 0=disable; 1=enable
 *
 * @param   sampleRate - I - PWM Sample Rate (285 Hz to 18,666,667 Hz) - Sample Rate = Pulse Frequency / Duty Cycle
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;
    unsigned char value = 0;

    value = channel & 1;

    if(enable)
        value |= BIT7;

    msg.text.data[2] = value;
    msg.text.data[3] = sampleRate;
    msg.text.data[4] = sampleRate >> 8;
    msg.text.data[5] = sampleRate >> 16;
    msg.text.data[6] = sampleRate >> 24;
    LCR_PrepWriteCmd(&msg, PWM_CAPTURE_CONFIG);

    return LCR_SendMsg(&msg);
}

int LCR_GetPWMCaptureConfig(unsigned int channel, BOOL *pEnabled, unsigned int *pSampleRate)
/**
 * (I2C: 0x43)
 * (USB: CMD2: 0x1A, CMD3: 0x12)
 * This API reads back the configuration of the specified PWM capture channel.
 *
 * @param   channel - I - PWM Capture Port
 *                      0 - PWM input channel 0 (GPIO_5)
 *                      1 - PWM input channel 1 (GPIO_6)
 *
 * @param   *pEnabled - O - PWM Channel enable 0=disable; 1=enable
 *
 * @param   *pSampleRate - O - PWM Sample Rate (285 Hz to 18,666,667 Hz) - Sample Rate = Pulse Frequency / Duty Cycle
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmdWithParam(PWM_CAPTURE_CONFIG, (unsigned char)channel);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        if(msg.text.data[0] & BIT7)
            *pEnabled =  TRUE;
        else
            *pEnabled = FALSE;

        *pSampleRate = msg.text.data[1] | msg.text.data[2] << 8 | msg.text.data[3] << 16 | msg.text.data[4] << 24;

        return 0;
    }
    return -1;
}

int LCR_PWMCaptureRead(unsigned int channel, unsigned int *pLowPeriod, unsigned int *pHighPeriod)
/**
 * (I2C: 0x4E)
 * (USB: CMD2: 0x1A, CMD3: 0x13)
 * This API returns both the number of clock cycles the signal was low and high.
 *
 * @param   channel - I - PWM Capture Port
 *                      0 - PWM input channel 0 (GPIO_5)
 *                      1 - PWM input channel 1 (GPIO_6)
 *
 * @param   *pLowPeriod - O - indicates how many samples were taken during a low signal
 *
 * @param   *pHighPeriod - O - indicates how many samples were taken during a high signal
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmdWithParam(PWM_CAPTURE_READ, (unsigned char)channel);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pLowPeriod = msg.text.data[1] | msg.text.data[2] << 8;
        *pHighPeriod = msg.text.data[3] | msg.text.data[4] << 8;
        return 0;
    }
    return -1;
}

int LCR_SetGPIOConfig(unsigned int pinNum, BOOL dirOutput, BOOL outTypeOpenDrain, BOOL pinState)
/**
 * (I2C: 0x44)
 * (USB: CMD2: 0x1A, CMD3: 0x38)
 *
 * This API enables GPIO functionality on a specific set of DLPC900 pins. The
 * command sets their direction, output buffer type, and output state.
 *
 * @param   pinNum - I - GPIO selection. See Table 2-38 in the programmer's guide for description of available pins
 *
 * @param   dirOutput - I - 0=input; 1=output
 *
 * @param   outTypeOpenDrain - I - 0=Standard buffer (drives high or low); 1=open drain buffer (drives low only)
 *
 * @param   pinState - I - 0=LOW; 1=HIGH
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;
    unsigned char value = 0;

    if(dirOutput)
        value |= BIT1;
    if(outTypeOpenDrain)
        value |= BIT2;
    if(pinState)
        value |= BIT0;

    msg.text.data[2] = pinNum;
    msg.text.data[3] = value;
    LCR_PrepWriteCmd(&msg, GPIO_CONFIG);

    return LCR_SendMsg(&msg);
}

int LCR_GetGPIOConfig(unsigned int pinNum, BOOL *pDirOutput, BOOL *pOutTypeOpenDrain, BOOL *pState)
/**
 * (I2C: 0x44)
 * (USB: CMD2: 0x1A, CMD3: 0x38)
 *
 * This API reads back the GPIO configuration on a specific set of DLPC900 pins. The
 * command reads back their direction, output buffer type, and  state.
 *
 * @param   pinNum - I - GPIO selection. See Table 2-38 in the programmer's guide for description of available pins
 *
 * @param   *pDirOutput - O - 0=input; 1=output
 *
 * @param   *pOutTypeOpenDrain - O - 0=Standard buffer (drives high or low); 1=open drain buffer (drives low only)
 *
 * @param   *pState - O - 0=LOW; 1=HIGH
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmdWithParam(GPIO_CONFIG, (unsigned char)pinNum);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pDirOutput = ((msg.text.data[0] & BIT1) == BIT1);
        *pOutTypeOpenDrain = ((msg.text.data[0] & BIT2) == BIT2);
        *pState = ((msg.text.data[0] & BIT0) == BIT0);

        return 0;
    }
    return -1;
}

int LCR_SetGeneralPurposeClockOutFreq(unsigned int clkId, BOOL enable, unsigned int clkDivider)
/**
 * (I2C: 0x48)
 * (USB: CMD2: 0x08, CMD3: 0x07)
 *
 * DLPC900 supports two pins with clock output capabilities: GPIO_11 and GPIO_12.
 * This API enables the clock output functionality and sets the clock frequency.
 *
 * @param   clkId - I - Clock selection. 1=GPIO_11; 2=GPIO_12
 *
 * @param   enable - I - 0=disable clock functionality on selected pin; 1=enable clock functionality on selected pin
 *
 * @param   clkDivider - I - Allowed values in the range of 2 to 127. Output frequency = 96MHz / (Clock Divider)
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = clkId;
    msg.text.data[3] = enable;
    msg.text.data[4] = clkDivider;
    LCR_PrepWriteCmd(&msg, GPCLK_CONFIG);

    return LCR_SendMsg(&msg);
}

int LCR_GetGeneralPurposeClockOutFreq(unsigned int clkId, BOOL *pEnabled, unsigned int *pClkDivider)
/**
 * (I2C: 0x48)
 * (USB: CMD2: 0x08, CMD3: 0x07)
 *
 * DLPC900 supports two pins with clock output capabilities: GPIO_11 and GPIO_12.
 * This API reads back the clock output enabled status and the clock frequency.
 *
 * @param   clkId - I - Clock selection. 1=GPIO_11; 2=GPIO_12
 *
 * @param   *pEnabled - O - 0=disable clock functionality on selected pin; 1=enable clock functionality on selected pin
 *
 * @param   *pClkDivider - O - Allowed values in the range of 2 to 127. Output frequency = 96MHz / (Clock Divider)
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmdWithParam(GPCLK_CONFIG, (unsigned char)clkId);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pEnabled = msg.text.data[1];
        *pClkDivider = msg.text.data[2];
        return 0;
    }
    return -1;
}

int LCR_SetLEDPWMInvert(BOOL invert)
/**
 * (I2C: 0x0B)
 * (USB: CMD2: 0x1A, CMD3: 0x05)
 *
 * This API sets the polarity of all LED PWM signals. This API must be called before powering up the LED drivers.
 *
 * @param   invert - I - 0 = Normal polarity, PWM 0 value corresponds to no current while PWM 255 value corresponds to maximum current
 *                       1 = Inverted polarity. PWM 0 value corresponds to maximum current while PWM 255 value corresponds to no current.
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = invert;
    LCR_PrepWriteCmd(&msg, PWM_INVERT);

    return LCR_SendMsg(&msg);
}

int LCR_GetLEDPWMInvert(BOOL *inverted)
/**
 * (I2C: 0x0B)
 * (USB: CMD2: 0x1A, CMD3: 0x05)
 *
 * This API reads the polarity of all LED PWM signals.
 *
 * @param   invert - O - 0 = Normal polarity, PWM 0 value corresponds to no current while PWM 255 value corresponds to maximum current
 *                       1 = Inverted polarity. PWM 0 value corresponds to maximum current while PWM 255 value corresponds to no current.
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(PWM_INVERT);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *inverted = (msg.text.data[0] != 0);
        return 0;
    }
    return -1;
}

int LCR_I2CConfigure(unsigned int port, unsigned int addm, unsigned int clk)
/**
 * (I2C: 0x4E)
 * (USB: CMD2: 0x1A, CMD3: 0x4E)
 *
 * @param
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.head.flags.rw = 0; //Write
    msg.head.flags.reply = 1; //Host wants a reply from device
    msg.head.flags.dest = 0; //Projector Control Endpoint
    msg.head.flags.reserved = 0;
    msg.head.flags.nack = 0;
    msg.head.seq = 0;
    msg.text.cmd = CmdList[I2C_CONFIG].USBCMD;
    msg.head.length = CmdList[I2C_CONFIG].len + 2;

    msg.text.data[2] = (((addm & 0x01) << 4) | (port & 0x03));
    msg.text.data[3] = (unsigned char)(clk);
    msg.text.data[4] = (unsigned char)(clk >> 8);
    msg.text.data[5] = (unsigned char)(clk >> 16);
    msg.text.data[6] = (unsigned char)(clk >> 24);

    return LCR_SendMsg(&msg);
}

int LCR_WriteI2CPassThrough(unsigned int port, unsigned int devadd, unsigned char* wdata, unsigned int nwbytes)
/**
 * (I2C: 0x4F)
 * (USB: CMD2: 0x1A, CMD3: 0x4F)
 *
 * @param
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    uint32 i;
    int x=0;
    hidMessageStruct msg;

    msg.head.flags.rw = 0; //Write
    msg.head.flags.reply = 1; //Host wants a reply from device
    msg.head.flags.dest = 0; //Projector Control Endpoint
    msg.head.flags.reserved = 0;
    msg.head.flags.nack = 0;
    msg.head.seq = seqNum++;
    msg.text.cmd = CmdList[I2C_PASSTHRU].USBCMD;
    msg.head.length = CmdList[I2C_PASSTHRU].len + nwbytes + 2;

    msg.text.data[2] = (unsigned char)(nwbytes);
    msg.text.data[3] = (unsigned char)(nwbytes >> 8);
    msg.text.data[4] = (port & 0x03);
    msg.text.data[5] = (unsigned char)(devadd);
    msg.text.data[6] = (unsigned char)(devadd >> 8);

    for ( i = 7; i < (7 + nwbytes); i++)
    {
        msg.text.data[i] = wdata[x++];
    }

    return LCR_SendMsg(&msg);
}

int LCR_ReadI2CPassThrough(unsigned int port, unsigned int devadd, unsigned char* wdata, unsigned int nwbytes, unsigned int nrbytes, unsigned char* rdata)
/**
 * (I2C: 0x4F)
 * (USB: CMD2: 0x1A, CMD3: 0x4F)
 *
 * @param
 *
 * @return  >=0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    uint32 i;
    int x=0;
    hidMessageStruct msg;

    msg.head.flags.rw = 1; //Read
    msg.head.flags.reply = 1; //Host wants a reply from device
    msg.head.flags.dest = 0; //Projector Control Endpoint
    msg.head.flags.reserved = 0;
    msg.head.flags.nack = 0;
    msg.head.seq = 0;

    msg.text.cmd = CmdList[I2C_PASSTHRU].USBCMD;
    msg.head.length = 7 + nwbytes + 2;

    msg.text.data[2]  = (unsigned char)(nwbytes);
    msg.text.data[3]  = (unsigned char)(nwbytes >> 8);
    msg.text.data[4]  = (unsigned char)(nrbytes);
    msg.text.data[5]  = (unsigned char)(nrbytes >> 8);
    msg.text.data[6]  = (port & 0x03);
    msg.text.data[7]  = (unsigned char)(devadd);
    msg.text.data[8]  = (unsigned char)(devadd >> 8);


    if ( nwbytes > 0 )
    {
        for ( i = 9; i < (9 + nwbytes); i++)
        {
            msg.text.data[i] = wdata[x++];
        }
    }

    OutputBuffer[0]=0; // First byte is the report number
    memcpy(&OutputBuffer[bufferIndex], &msg, (sizeof(msg.head)+ msg.head.length));

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        nrbytes &= 0x3F;
        for( i = 0; i < nrbytes; i++ )
            rdata[i] = msg.text.data[i];

        return 0;
    }

    return -1;
}

int LCR_InitPatternMemLoad(BOOL master, unsigned short imageNum, unsigned int size)
{
    hidMessageStruct msg;

    msg.text.data[2] = imageNum & 0xFF;
    msg.text.data[3] = (imageNum >> 8) & 0xFF;
    msg.text.data[4] = size & 0xFF;
    msg.text.data[5] = (size >> 8) & 0xFF;
    msg.text.data[6] = (size >> 16) & 0xFF;
    msg.text.data[7] = (size >> 24) & 0xFF;

    if (master)
        LCR_PrepWriteCmd(&msg, PATMEM_LOAD_INIT_MASTER);
    else
        LCR_PrepWriteCmd(&msg, PATMEM_LOAD_INIT_SLAVE);

    return LCR_SendMsg(&msg);
}

int LCR_pattenMemLoad(BOOL master, unsigned char *pByteArray, int size)
{
    hidMessageStruct msg;
    int retval;
    int dataLen;

    dataLen = 512 - sizeof(msg.head)- sizeof(msg.text.cmd) - 2;//The last -2 is to workaround a bug in bootloader.
    
    //    dataLen = 186;

    /*    if (size < dataLen)
    {
      memcpy(&msg.text.data[3], pByteArray, size);
    }
    else
    {
      memcpy(&msg.text.data[3], pByteArray, dataLen);
    }*/

    if (size < dataLen)
        dataLen = size;


    memcpy(&msg.text.data[4], pByteArray, dataLen);
    msg.text.data[2] = dataLen & 0xFF;
    msg.text.data[3] = (dataLen >> 8) & 0xFF;

    if (master)
    {
        CmdList[PATMEM_LOAD_DATA_MASTER].len = dataLen + 2;
        LCR_PrepWriteCmd(&msg, PATMEM_LOAD_DATA_MASTER);
    }
    else
    {
        CmdList[PATMEM_LOAD_DATA_SLAVE].len = dataLen + 2;
        LCR_PrepWriteCmd(&msg, PATMEM_LOAD_DATA_SLAVE);
    }

    retval = LCR_SendMsg(&msg);
    if(retval > 0)
        return dataLen;

    return -1;
}

int LCR_getBatchFileName(unsigned char id, char *batchFileName, unsigned char* length)
{
    hidMessageStruct msg;

    LCR_PrepReadCmdWithParam(BATCHFILE_NAME, id);
    *length = 0;

    if(LCR_Read() > 0)
    {
		int i;
        memcpy(&msg, InputBuffer, 65);
        for (i = 0; i < 16; i++)
        {
            if (msg.text.data[i] == 0)
            {
                batchFileName[i] = '\0';
                break;
            }
            batchFileName[i] = msg.text.data[i];
        }
        *length = i;
        return 0;
    }
    return -1;
}

int LCR_executeBatchFile(unsigned char id)
{
    hidMessageStruct msg;

    msg.text.data[2] = id;
    LCR_PrepWriteCmd(&msg, BATCHFILE_EXECUTE);

    return LCR_SendMsg(&msg);
}

int LCR_enableDebug()
{
    hidMessageStruct msg;

    msg.text.data[2] = 0x0f;
    msg.text.data[3] = 0x00;
    msg.text.data[4] = 0x00;
    msg.text.data[5] = 0x40;
    msg.text.data[6] = 0x01;
    LCR_PrepWriteCmd(&msg, DEBUG);

    return LCR_SendMsg(&msg);

}

int LCR_ExecuteRawCommand(uint16 USBCmd, uint08 *Data, int Length)
{
	int i;
    hidMessageStruct msg;

    msg.head.flags.rw = 0; //Write
    msg.head.flags.reply = bufferIndex; //Host wants a reply from device. Set for HID.
    msg.head.flags.dest = 0; //Projector Control Endpoint
    msg.head.flags.reserved = 0;
    msg.head.flags.nack = 0;
    msg.head.seq = seqNum++;
    
    msg.text.cmd = USBCmd;
    for (i = 0; i < Length; i++)
        msg.text.data[i+2] = Data[i];

    msg.head.length = Length + 2;

    return LCR_SendMsg(&msg);
}

void API_SetDataCallback(API_DataCallback_t *Callback, void *Param)
{
    LCR_DataCallback = Callback;
	LCR_CallbackParam = Param;
}

int API_GetUSBCommand(char const *command, uint16 *usbCommand)
{
    uint16 i;
    for(i = 0; i < ARRAY_SIZE(CmdList); i++)
    {
        if(!strcmp(command, CmdList[i].name))
        {
            *usbCommand = CmdList[i].USBCMD;
            return 0;
        }
    }
    return -1;
}

int API_GetI2CCommand(char *command, unsigned char *i2cCommand)
{
    uint16 i;
    for(i = 0; i < ARRAY_SIZE(CmdList); i++)
    {
        if(!strcmp(command, CmdList[i].name))
        {
            *i2cCommand = CmdList[i].I2CCMD;
            return 0;
        }
    }
    return -1;
}

int API_GetCommandLength(unsigned char *cmd, int *len)
{
    uint16 i;
	int length = 0;
    for(i = 0; i < ARRAY_SIZE(CmdList); i++)
    {
        if(CmdList[i].I2CCMD == ((*cmd)&0x7F))
        {
			if(CmdList[i].varLen == 1)
				length = cmd[1];
			else if(CmdList[i].varLen == 2)
				length = MAKE_WORD16(cmd[2], cmd[1]);
			length += CmdList[i].len;

			*len = length;
            return 0;
        }
    }
    return -1;
}

int API_GetCommandName(unsigned char i2cCommand, char const **command)
{
    uint16 i;
    for(i = 0; i < ARRAY_SIZE(CmdList); i++)
    {
        if ((i2cCommand&0x7F) == CmdList[i].I2CCMD)
        {
            *command = (char *)CmdList[i].name;
            return 0;
        }
    }
    return -1;
}


int LCR_ReadErrorCode(unsigned int *pCode)
/**
 * (I2C: 0x32)
 * (USB: CMD2: 0x01, CMD3: 0x00)
 * Reads the error code for the last executed command.
 *
 * @param *pCode - O - Error code for last executed command.
 *
 * @return  =0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    LCR_PrepReadCmd(READ_ERROR_CODE);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);
        *pCode = msg.text.data[0];
        return 0;
    }
    return -1;
}

int LCR_ReadErrorString(char *errStr)
/**
 * (I2C: 0x33)
 * (USB: CMD2: 0x01, CMD3: 0x01)
 * This API Reads the descriptive error string for the last executed command.
 * This is in parallel with the error code but contains more details.
 *
 * @param - *errStr - O - Error description for the last executed command.
 *
 * @return  =0 = PASS    <BR>
 *          <0 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;
    unsigned int i;

    LCR_PrepReadCmd(READ_ERROR_MSG);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 128);

        for(i=0;i<128;i++)
        {
          *errStr = msg.text.data[i];
            errStr++;
        }

        return 0;
    }
    return -1;
}

int LCR_GetFrmwVersion(unsigned int *pFrmwType, char *pFrmwTag)
/**
 * (I2C: 0x12)
 * (USB: CMD2: 0x02, CMD3: 0x06)
 * This API reads the firmware type and the firmware tag stored in the image.
 *
 * @param   *pFrmwType  - O - Firmware Type:
 *                        0 = Unknown
 *                        1 = 1080p Firmware
 *                        2 = WQXGA Firmware
 * @param   *pSwap - O - Firmware tag stored in the image.
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;
    unsigned int i;

    LCR_PrepReadCmd(READ_FRMW_VERSION);

    if(LCR_Read() > 0)
    {
        memcpy(&msg, InputBuffer, 65);

        *pFrmwType = msg.text.data[0];

        for(i=1;i<=32;i++)
        {
          *pFrmwTag = msg.text.data[i];
            pFrmwTag++;
        }

        return 0;
    }
    return -1;
}

int LCR_GetDMDBlocks(int *startBlock, int *numBlocks)
/**
 * (I2C: 0x60)
 * (USB: CMD2: 0x1A, CMD3: 0x40)
 *
 * This API reads the active DMD blocks
 *
 * @param   *startBlock
 * @param   *numBlocks
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    //hidMessageStruct msg;

    LCR_PrepReadCmd(DMD_BLOCKS);

    if(LCR_Read() > 0)
    {
        *startBlock = InputBuffer[4];
        *numBlocks = InputBuffer[5];
        return 0;
    }

    *startBlock = 0;
    *numBlocks = 0;

    return -1;
}

int LCR_SetDMDBlocks(int startBlock, int numBlocks)
/**
 * (I2C: 0x60)
 * (USB: CMD2: 0x1A, CMD3: 0x40)
 *
 * This API reads the active DMD blocks
 *
 * @param   startBlock
 * @param   numBlocks
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = startBlock;
    msg.text.data[3] = numBlocks;

    LCR_PrepWriteCmd(&msg, DMD_BLOCKS);

    return LCR_SendMsg(&msg);
}

int LCR_SetMinLEDPulseWidth(uint32 pulseWidthUS)
/**
 * (I2C: 0x64)
 * (USB: CMD2: 0x1A, CMD3: 0x43)
 *
 * This API sets the minimum LED pulse width restriction for the
 * pattern seqeunce generation
 *
 * @param   pulseWidthUS - Minimum pulse width in 0.01 microseconds
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    hidMessageStruct msg;

    msg.text.data[2] = pulseWidthUS;
    msg.text.data[3] = pulseWidthUS >> 8;
    msg.text.data[4] = pulseWidthUS >> 16;
    msg.text.data[5] = pulseWidthUS >> 24;

    LCR_PrepWriteCmd(&msg, LED_PULSE_WIDTH);

    return LCR_SendMsg(&msg);
}

int LCR_GetMinLEDPulseWidth(uint32 *pulseWidthUS)
/**
 * (I2C: 0x64)
 * (USB: CMD2: 0x1A, CMD3: 0x43)
 *
 * This API reads the active DMD blocks
 *
 * @param   [out] pulseWidthUS - Minimum pulse width in 0.01 microseconds
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    LCR_PrepReadCmd(LED_PULSE_WIDTH);

    if(LCR_Read() > 0)
    {
        *pulseWidthUS = MAKE_WORD32(InputBuffer[7], InputBuffer[6], InputBuffer[5], InputBuffer[4]);
        return 0;
    }

    *pulseWidthUS = 0;

    return -1;
}

int LCR_GetMinPatExposure(int minExposureUS[16])
/**
 * (I2C: 0x63)
 * (USB: CMD2: 0x1A, CMD3: 0x42)
 *
 * This API reads the minimum pattern exposure for all the bitdepths
 * The value is based on DMD active block selection and LED pulse width
 * settings
 *
 * @param   [out] minExposureUS - Minimum pattern exposure for each bitdepth
 *                                in microseconds
 *
 * @return  0 = PASS    <BR>
 *          -1 = FAIL  <BR>
 *
 */
{
    LCR_PrepReadCmd(MIN_EXPOSURE);

    if(LCR_Read() > 0)
    {
        int index = 4;
		int i;
        for(i=0; i<16; i++) minExposureUS[i]=0;

		for(i = 0; i < 8; i++)
        {
            minExposureUS[i] = MAKE_WORD16(InputBuffer[index+1], InputBuffer[index]);
            index += 2;
        }
		
		for(; i < 16; i++)
        {
			i++;
            minExposureUS[i] = MAKE_WORD32(InputBuffer[index+3], InputBuffer[index+2], InputBuffer[index+1], InputBuffer[index]);
            index += 4;
        }
		
		
        return 0;
    }

    return -1;
}

int LCR_ReadSram(uint32 address, uint32 *data, uint08 type)
{
    LCR_PrepReadCmdWithWordParam(READ_WRITE_SRAM, address, type);

    if(LCR_Read() > 0)
    {
        *data = MAKE_WORD32(InputBuffer[7], InputBuffer[6], InputBuffer[5], InputBuffer[4]);
        return 0;
    }

    return -1;
}

int LCR_WriteSram(uint32 address, uint32 data, uint08 type)
{
    hidMessageStruct msg;
    uint32 *pA = (uint32*)&msg.text.data[2];
    uint32 *pD = (uint32*)&msg.text.data[6];
    msg.text.data[10] = type;

    *pA = address;
    *pD = data;
    LCR_PrepWriteCmd(&msg, READ_WRITE_SRAM);

    return LCR_SendMsg(&msg);

    return -1;
}

int LCR_LoadNGo_Init(uint32 size)
{
    hidMessageStruct msg;

    msg.text.data[2] = (uint08)(size      );
    msg.text.data[3] = (uint08)(size >>  8);
    msg.text.data[4] = (uint08)(size >> 16);
    msg.text.data[5] = (uint08)(size >> 24);

    LCR_PrepWriteCmd(&msg, LOADNGO_INIT_MASTER);

    return LCR_SendMsg(&msg);
}

int LCR_LoadNGo_Data(unsigned char *pByteArray, unsigned int dataLen)
{
    hidMessageStruct msg;
    int retval;

    uint08 *ptr = pByteArray;

    if(dataLen > 504)
        dataLen = 504;

    CmdList[LOADNGO_DATA_MASTER].len = dataLen + 2;
    msg.text.data[2] = (uint08)dataLen;
    msg.text.data[3] = (uint08)(dataLen >> 8);
    memcpy((uint08*)&msg.text.data[4], (uint08*)ptr, dataLen);

    LCR_PrepWriteCmd(&msg, LOADNGO_DATA_MASTER);

    retval = LCR_SendMsg(&msg);
    if(retval > 0)
        return dataLen;

    return -1;
}

int LCR_LoadNGo_SRAM(BOOL run)
{
    hidMessageStruct msg;

    msg.text.data[2] = run;

    LCR_PrepWriteCmd(&msg, LOADNGO_SRAM);

    return LCR_SendMsg(&msg);
}
