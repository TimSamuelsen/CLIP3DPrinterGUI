/*
 *
 * This module has the wrapper functions to access USB driver functions.
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/


#include "usb.h"
#include <stdio.h>
#include <stdlib.h>
#include "hidapi.h"
#include "API.h"

/***************************************************
*                  GLOBAL VARIABLES
****************************************************/
/** USB HID Device handler */
static hid_device *DeviceHandle;

static BOOL FakeConnection = FALSE; /**< Simulated connection */
static BOOL USBConnected = FALSE; /**< Device connected status */

/**
 * Enable/disable simulated connection without HW
 *
 * @param enable TRUE = Enable simulated connection
 *               FALSE = Disable simulated connection
 */
void USB_SetFakeConnection(BOOL enable)
{
    FakeConnection = enable;
}

/**
 * Check if USB device is conencted.
 *
 * @return TRUE = USB device is connected, FALSE = USB device is not connected
 */
BOOL USB_IsConnected()
{
    return USBConnected;
}

/**
 * Initialize USB driver
 *
 * @return 0 on success, -ve on failure
 */
int USB_Init(void)
{
    return hid_init();
}

/**
 * Close the USB driver
 *
 * @return  0 on success, -ve on failure
 */
int USB_Exit(void)
{
    return hid_exit();
}

/**
 * Open a new USB connection with the EVM system
 *
 * @return 0 on success, -1 on failure
 */
int USB_Open()
{
    if(FakeConnection == FALSE)
    {
        struct hid_device_info *hid_info;
        hid_info = hid_enumerate(MY_VID, MY_PID);
        if(hid_info == NULL)
        {
            USBConnected = FALSE;
            return -1;
        }

        DeviceHandle = NULL;

        if(0 == hid_info->interface_number)
        {
            DeviceHandle = hid_open_path(hid_info->path);
        }
        else
        {
            struct hid_device_info *hid_next_info = hid_info->next;
            if(hid_next_info != NULL)
            {
                DeviceHandle = hid_open_path(hid_next_info->path);
            }
        }

        if(DeviceHandle == NULL)
        {
            USBConnected = FALSE;
            return -1;
        }
    }
    USBConnected = TRUE;
    return 0;
}

static hidMessageStruct dummyMsg;
static unsigned char powermode;
static unsigned char dispmode;

/**
 * Writes given bytes to the USB device
 *
 * @return 0 on success, -1 on failure
 */
int USB_Write(uint08 *Data)
{
    if(FakeConnection == TRUE)
    {
        memcpy(&dummyMsg, Data + 1, 16);

		if(dummyMsg.head.flags.rw == 0)
		{
			switch(dummyMsg.text.cmd)
			{
				case  0x200: // power mode
					powermode = dummyMsg.text.data[2];
					break;

				case 0x1A1B:
					dispmode = dummyMsg.text.data[2];
					break;
			}
		}
        return 1;
	}


    if(DeviceHandle == NULL)
        return -1;

    /*    for (int i = 0; i < USB_MIN_PACKET_SIZE; i++)
        printf("0x%x ", Data[i]);
    printf("\n\n");*/
    return hid_write(DeviceHandle, Data, USB_MIN_PACKET_SIZE+1);
}

/**
 * Read one HID packet of data from the USB connection
 *
 * @param Data Pointer to store the read data
 *
 * @return 0 on success -1 on failure
 */
int USB_Read(uint08 *Data)
{
    if(FakeConnection == TRUE)
    {
        switch(dummyMsg.text.cmd)
        {
        case  0x200: // power mode
			dummyMsg.text.data[0] = powermode;
			memcpy(Data, &dummyMsg, 16);
            break;

        case 0x1A1B:
			dummyMsg.text.data[0] = dispmode;
			memcpy(Data, &dummyMsg, 16);
            break;

        default:
            memset(Data, 0, 16);
        }
        return 1;
    }

    if(DeviceHandle == NULL)
        return -1;

    return hid_read_timeout(DeviceHandle, Data, USB_MIN_PACKET_SIZE, 10000);
}

/**
 * Close the USB connection (if any) to the device
 * @return
 */
int USB_Close()
{
    if(FakeConnection == FALSE)
    {
        hid_close(DeviceHandle);
        USBConnected = FALSE;
    }
    return 0;
}
