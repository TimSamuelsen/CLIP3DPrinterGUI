/**
 * This module has the wrapper functions to access USB driver functions.
 *
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#ifndef USB_H
#define USB_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif


#define USB_MIN_PACKET_SIZE 64
#define USB_MAX_PACKET_SIZE 64

#define MY_VID					0x0451
#define MY_PID					0xC900
#define HID_MESSAGE_MAX_SIZE    2048

#define USBHID  0
#define USBBULK 1

enum SetupDiGetDeviceRegistryPropertyEnum
{
    SPDRP_DEVICEDESC = 0x00000000, // DeviceDesc (R/W)
    SPDRP_HARDWAREID = 0x00000001, // HardwareID (R/W)
    SPDRP_COMPATIBLEIDS = 0x00000002, // CompatibleIDs (R/W)
    SPDRP_UNUSED0 = 0x00000003, // unused
    SPDRP_SERVICE = 0x00000004, // Service (R/W)
    SPDRP_UNUSED1 = 0x00000005, // unused
    SPDRP_UNUSED2 = 0x00000006, // unused
    SPDRP_CLASS = 0x00000007, // Class (R--tied to ClassGUID)
    SPDRP_CLASSGUID = 0x00000008, // ClassGUID (R/W)
    SPDRP_DRIVER = 0x00000009, // Driver (R/W)
    SPDRP_CONFIGFLAGS = 0x0000000A, // ConfigFlags (R/W)
    SPDRP_MFG = 0x0000000B, // Mfg (R/W)
    SPDRP_FRIENDLYNAME = 0x0000000C, // FriendlyName (R/W)
    SPDRP_LOCATION_INFORMATION = 0x0000000D, // LocationInformation (R/W)
    SPDRP_PHYSICAL_DEVICE_OBJECT_NAME = 0x0000000E, // PhysicalDeviceObjectName (R)
    SPDRP_CAPABILITIES = 0x0000000F, // Capabilities (R)
    SPDRP_UI_NUMBER = 0x00000010, // UiNumber (R)
    SPDRP_UPPERFILTERS = 0x00000011, // UpperFilters (R/W)
    SPDRP_LOWERFILTERS = 0x00000012, // LowerFilters (R/W)
    SPDRP_BUSTYPEGUID = 0x00000013, // BusTypeGUID (R)
    SPDRP_LEGACYBUSTYPE = 0x00000014, // LegacyBusType (R)
    SPDRP_BUSNUMBER = 0x00000015, // BusNumber (R)
    SPDRP_ENUMERATOR_NAME = 0x00000016, // Enumerator Name (R)
    SPDRP_SECURITY = 0x00000017, // Security (R/W, binary form)
    SPDRP_SECURITY_SDS = 0x00000018, // Security (W, SDS form)
    SPDRP_DEVTYPE = 0x00000019, // Device Type (R/W)
    SPDRP_EXCLUSIVE = 0x0000001A, // Device is exclusive-access (R/W)
    SPDRP_CHARACTERISTICS = 0x0000001B, // Device Characteristics (R/W)
    SPDRP_ADDRESS = 0x0000001C, // Device Address (R)
    SPDRP_UI_NUMBER_DESC_FORMAT = 0X0000001D, // UiNumberDescFormat (R/W)
    SPDRP_DEVICE_POWER_DATA = 0x0000001E, // Device Power Data (R)
    SPDRP_REMOVAL_POLICY = 0x0000001F, // Removal Policy (R)
    SPDRP_REMOVAL_POLICY_HW_DEFAULT = 0x00000020, // Hardware Removal Policy (R)
    SPDRP_REMOVAL_POLICY_OVERRIDE = 0x00000021, // Removal Policy Override (RW)
    SPDRP_INSTALL_STATE = 0x00000022, // Device Install State (R)
    SPDRP_LOCATION_PATHS = 0x00000023, // Device Location Paths (R)
    SPDRP_BASE_CONTAINERID = 0x00000024  // Base ContainerID (R)
};

typedef struct
{
    struct
    {
        struct
        {
            unsigned char dest		:3; /* 0 - ProjCtrl; 1 - RFC; 7 - Debugmsg */
            unsigned char reserved	:2;
            unsigned char nack		:1; /* Command Handler Error */
            unsigned char reply	    :1; /* Host wants a reply from device */
            unsigned char rw		:1; /* Write = 0; Read = 1 */
        }flags;
        unsigned char seq;
        unsigned short length;
    }head;
    union
    {
        unsigned short cmd;
        unsigned char data[HID_MESSAGE_MAX_SIZE];
    }text;
}hidMessageStruct;
void USB_SetFakeConnection(BOOL enable);
int USB_Open(void);
BOOL USB_IsConnected();
int USB_Write(uint08 *Data);
int USB_Read(uint08 *Data);
int USB_Close();
int USB_Init();
int USB_Exit();
int USB_SetUSBType(int type);
int USB_GetUSBType();

#ifdef __cplusplus
}
#endif


#endif //USB_H
