/*
 *
 * This module has the wrapper functions to access USB driver functions.
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#include <QLibrary>
#include <QUuid>
#include <windows.h>

#include "usb.h"
#include <stdio.h>
#include <stdlib.h>
#include "hidapi.h"
#include "API.h"

/***************************************************
*                  GLOBAL VARIABLES
****************************************************/
/** USB HID Device handler */
static hid_device *HidDeviceHandle;

static BOOL FakeConnection = FALSE; /**< Simulated connection */
static BOOL USBConnected = FALSE; /**< Device connected status */

static hidMessageStruct dummyMsg;
static unsigned char powermode;
static unsigned char dispmode;

static int USB_ConnectionType = USBHID;

#define INTZERO ((int *)0)
#define UINT32ZERO ((uint32 *)0)
#define PIPE_TRANSFER_TIMEOUT 0x03

#pragma pack(push,1)
struct SP_DEVICE_INTERFACE_DETAIL_DATA
{
    uint32 cbSize;
    QChar   DeviceName[256];
};

struct SP_DEVICE_INTERFACE_DATA
{
    uint32 cbSize;
    GUID   ClassGuid;
    int    FLAGS;
    int    Reserved;
};

struct SP_DEVINFO_DATA
{
    uint32 cbSize;
    GUID   ClassGuid;
    uint32 DevInst;
    int*   Reserved;
};
#pragma pack(pop)

typedef PVOID WINUSB_INTERFACE_HANDLE, *PWINUSB_INTERFACE_HANDLE;

HANDLE USBBulkhandle = NULL;
WINUSB_INTERFACE_HANDLE USBWinDrvHandle = NULL;

const int DIGCF_PRESENT = 0x00000002;
const int DIGCF_DEVICEINTERFACE = 0x00000010;
const int CR_SUCCESS = 0x00000000;
const int READWRITE = 0xC0000000;

typedef __stdcall int (*call_getLastError)(void);

typedef __stdcall int* (*call_SetupDiGetClassDevs)(GUID &ClassGuid, int Enumerator, int hwndParent, uint32 Flags);


typedef __stdcall BOOL (*call_SetupDiEnumDeviceInfo)(int* hDevInfo,
                                           uint32 MemberIndex,
                                           SP_DEVINFO_DATA &DeviceInfoData);

typedef __stdcall BOOL (*call_SetupDiEnumDeviceInterfaces)(int* hDevInfo, SP_DEVINFO_DATA &deviceInfoData,
                                                 GUID &ClassGuid, uint32 MemberIndex,
                                                 SP_DEVICE_INTERFACE_DATA &DeviceInterfaceData);

typedef __stdcall BOOL (*call_SetupDiGetDeviceInterfaceDetail)( int* hDevInfo,
                                                      SP_DEVICE_INTERFACE_DATA &deviceInterfaceData,
                                                      SP_DEVICE_INTERFACE_DETAIL_DATA *deviceInterfaceDetailData,
                                                      uint32 deviceInterfaceDetailDataSize,
                                                      uint32 *requiredSize,
                                                      SP_DEVINFO_DATA &deviceInfoData
                                                      );

typedef __stdcall HANDLE (*call_CreateFile)(wchar_t * lpFileName,						// file name
                                            uint dwDesiredAccess,					// access mode
                                            uint dwShareMode,						// share mode
                                            uint lpSecurityAttributes,				// SD
                                            uint dwCreationDisposition,				// how to create
                                            uint dwFlagsAndAttributes,				// file attributes
                                            uint hTemplateFile					    // handle to template file
                                            );

typedef __stdcall BOOL (*call_CloseHandle)(HANDLE hObject );
typedef __stdcall BOOL (*call_WinUsb_Initialize)( HANDLE DeviceHandle, PWINUSB_INTERFACE_HANDLE InterfaceHandle);
typedef __stdcall BOOL (*call_WinUsb_Free)(WINUSB_INTERFACE_HANDLE InterfaceHandle);
typedef __stdcall BOOL (*call_WinUsb_WritePipe)(WINUSB_INTERFACE_HANDLE InterfaceHandle,
                                                UCHAR pID,
                                                PUCHAR buf,
                                                ULONG buflen,
                                                PULONG sent,
                                                LPOVERLAPPED Olap);

typedef __stdcall BOOL (*call_WinUsb_SetPipePolicy)(WINUSB_INTERFACE_HANDLE InterfaceHandle,
                                                    UCHAR pID,
                                                    ULONG policytype,
                                                    ULONG valuelen,
                                                    PVOID value);

typedef __stdcall BOOL (*call_WinUsb_ReadPipe)(WINUSB_INTERFACE_HANDLE InterfaceHandle,
                                                         UCHAR PipeID,
                                                         PUCHAR Buffer,
                                                         ULONG BufferLength,
                                                         PULONG LengthTransferred,
                                                         LPOVERLAPPED Overlapped);

call_WinUsb_WritePipe fcall_WinUsb_WritePipe = NULL;
call_WinUsb_Free fcall_WinUsb_Free = NULL;
call_WinUsb_ReadPipe fcall_WinUsb_ReadPipe = NULL;
call_CloseHandle fcall_CloseHandle = NULL;

int USB_InitHid();
int USB_InitBulk();

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
 * Initialize USB driver type
 *
 * @return 0 on success, -ve on failure
 */
int USB_Init(int type)
{
    int results;
    if( type == USBHID )
    {
        results = USB_InitHid();
    }
    else
    {
        results = 0;
    }

    USB_ConnectionType = type;

    return results;
}

/**
 * Close the USB driver
 *
 * @return  0 on success, -ve on failure
 */
int USB_Exit(void)
{
    if( USB_ConnectionType == USBHID )
    {
        return hid_exit();
    }
    else
        return 0;
}

/**
 * Open a new USB connection with the EVM system
 *
 * @return 0 on success, -1 on failure
 */
int USB_Open()
{
    int results;

    if(FakeConnection == FALSE)
    {
        // Open the device using the VID, PID,
        // and optionally the Serial number.
        //HidDeviceHandle = (MY_VID, MY_PID, NULL);VID_0x451 PID_C900 MI_00
        //USBWinDrvHandle = (MY_VID, MY_PID, NULL);VID_0x451 PID_C900 MI_01

        if( USB_ConnectionType == USBHID )
        {
            HidDeviceHandle = hid_open(MY_VID, MY_PID, NULL);
            if(HidDeviceHandle == NULL)
            {
                USBConnected = FALSE;
                return -1;
            }
        }
        else
        {
            results = USB_InitBulk();
            if((USBWinDrvHandle == NULL) || (results == -1) )
            {
                USBConnected = FALSE;
                return -1;
            }
        }
    }
    USBConnected = TRUE;
    return 0;
}


/**
 * Writes given bytes to the USB device
 *
 * @return 0 on success, -1 on failure
 */
int USB_Write(uint08 *Data)
{
    ULONG cbSent = 0;
    UCHAR pID = 2;   //The C900 bulk write end point is on address 2
    BOOL res = 0;

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

    if( USB_ConnectionType == USBHID )
    {
        if(HidDeviceHandle == NULL)
            return -1;

        return hid_write(HidDeviceHandle, Data, USB_MIN_PACKET_SIZE+1);
    }
    else
    {
        if(USBWinDrvHandle == NULL)
            return -1;

        res = fcall_WinUsb_WritePipe(USBWinDrvHandle, pID, (UCHAR*)Data, USB_MIN_PACKET_SIZE, &cbSent, 0);
        if(res == true)
            return cbSent;
        else
            return -1;
    }
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
    BOOL res = 0;
    UCHAR pID = 0x82;          //The C900 bulk read end point is on address 82
    ULONG LengthTransferred;
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

    if( USB_ConnectionType == USBHID )
    {
        if(HidDeviceHandle == NULL)
            return -1;

        return hid_read_timeout(HidDeviceHandle, Data, USB_MIN_PACKET_SIZE, 10000);
    }
    else
    {
        if(USBWinDrvHandle == NULL)
            return -1;

        res = fcall_WinUsb_ReadPipe(USBWinDrvHandle, pID, Data, USB_MIN_PACKET_SIZE, &LengthTransferred, 0);
        if(res == true)
            return LengthTransferred;
        else
            return -1;
    }
    return -1;
}

/**
 * Close the USB connection (if any) to the device
 * @return
 */
int USB_Close()
{
    if(FakeConnection == FALSE)
    {
        if(USB_ConnectionType == USBHID)
        {
            hid_close(HidDeviceHandle);
            HidDeviceHandle = NULL;
        }
        else
        {
            fcall_WinUsb_Free(USBWinDrvHandle);
            fcall_CloseHandle(USBBulkhandle);
            USBWinDrvHandle = NULL;
            USBBulkhandle = NULL;
        }
        USBConnected = FALSE;
    }
    return 0;
}

/**
 * Return the USB connection type
 * @return
 */
int USB_GetUSBType()
{
    return USB_ConnectionType;
}

/**
 * Set the USB connection type
 * @return
 */
int USB_SetUSBType(int type)
{
    USB_ConnectionType = type;

    return 0;
}

/**
 * Init HID Interface
 * @return
 */
int USB_InitHid()
{
    return hid_init();
}

/**
 * Init BULK Interface
 * The C900 implements BULK on interface 1 and uses endpoint 2.
 * On Windows, the BULK interface is enumerated on MI_01.
 * @return
 */
int USB_InitBulk()
{
    QString str;
    int lastError = -1;
    uint32 ZERO = 0;
    SP_DEVINFO_DATA *dan = 0;
    SP_DEVICE_INTERFACE_DETAIL_DATA *di = 0;
    uint32 RequiredSize = 0;
    uint32 MemberIndex = 0;
    QUuid GUID_DEVINTERFACE_DEV("FCD4A8F6-1C8D-4076-8562-0407CB7566A0");

    // contains interface GUID
    SP_DEVICE_INTERFACE_DATA devInterData;
    memset(&devInterData, 0x00, sizeof(devInterData));
    devInterData.cbSize = (uint)sizeof(typeof(SP_DEVICE_INTERFACE_DATA));

    // build a DevInfo Data structure
    SP_DEVINFO_DATA devInfoData;
    memset(&devInfoData, 0x00, sizeof(devInfoData));
    devInfoData.cbSize = (uint32)sizeof(typeof(SP_DEVINFO_DATA));

    // build a dummey DevInfo Data structure
    SP_DEVINFO_DATA da;
    memset(&da, 0x00, sizeof(da));
    da.cbSize = 0;

    QLibrary library( "setupapi" );
    QLibrary kernelLib( "Kernel32" );
    QLibrary winusbLib( "winusb" );

    fcall_WinUsb_WritePipe = (call_WinUsb_WritePipe)winusbLib.resolve( "WinUsb_WritePipe" );
    fcall_WinUsb_Free = (call_WinUsb_Free)winusbLib.resolve( "WinUsb_Free" );
    fcall_WinUsb_ReadPipe = (call_WinUsb_ReadPipe)winusbLib.resolve( "WinUsb_ReadPipe" );
    call_WinUsb_SetPipePolicy fcall_WinUsb_SetPipePolicy = (call_WinUsb_SetPipePolicy)winusbLib.resolve( "WinUsb_SetPipePolicy" );
    call_WinUsb_Initialize fcall_WinUsb_Initialize = (call_WinUsb_Initialize)winusbLib.resolve( "WinUsb_Initialize" );
    fcall_CloseHandle = (call_CloseHandle)kernelLib.resolve( "CloseHandle" );
    call_CreateFile fcall_CreateFile = (call_CreateFile)kernelLib.resolve( "CreateFileW" );
    call_getLastError fcall_getLastError = (call_getLastError)kernelLib.resolve( "GetLastError" );
    call_SetupDiGetClassDevs fcall_SetupDiGetClassDevs = (call_SetupDiGetClassDevs)library.resolve( "SetupDiGetClassDevsW" );
    call_SetupDiEnumDeviceInfo fcall_SetupDiEnumDeviceInfo = (call_SetupDiEnumDeviceInfo)library.resolve( "SetupDiEnumDeviceInfo" );
    call_SetupDiEnumDeviceInterfaces fcall_SetupDiEnumDeviceInterfaces = (call_SetupDiEnumDeviceInterfaces)library.resolve( "SetupDiEnumDeviceInterfaces" );
    call_SetupDiGetDeviceInterfaceDetail fcall_SetupDiGetDeviceInterfaceDetail = (call_SetupDiGetDeviceInterfaceDetail)library.resolve( "SetupDiGetDeviceInterfaceDetailW" );

    if( (fcall_WinUsb_Initialize) && (fcall_WinUsb_Free)  && (fcall_WinUsb_WritePipe)  && (fcall_WinUsb_SetPipePolicy)  && (fcall_WinUsb_ReadPipe))
    {
        if( (fcall_CreateFile) && ( fcall_CloseHandle)  )
        {
            if( (fcall_SetupDiGetDeviceInterfaceDetail) && (fcall_SetupDiEnumDeviceInterfaces) && (fcall_SetupDiEnumDeviceInfo) && (fcall_SetupDiGetClassDevs) )
            {
                int* hdeviceinfo = fcall_SetupDiGetClassDevs((GUID&)GUID_DEVINTERFACE_DEV, 0, 0, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
                if (hdeviceinfo != INVALID_HANDLE_VALUE)
                {   //Interate through the 2 interfaces to locate MI_01
                    //for(int mi = 0; mi < 2; mi++)
                    {
                        if( true == fcall_SetupDiEnumDeviceInterfaces(hdeviceinfo, *dan, (GUID&)GUID_DEVINTERFACE_DEV, MemberIndex, (SP_DEVICE_INTERFACE_DATA&)devInterData))
                        {
                            MemberIndex++;

                            //probing to get allocation size
                            if (false == fcall_SetupDiGetDeviceInterfaceDetail(hdeviceinfo, (SP_DEVICE_INTERFACE_DATA &)devInterData, di, ZERO, &RequiredSize, (SP_DEVINFO_DATA &)devInfoData))
                            {
                                lastError = fcall_getLastError();
                                if( ERROR_INSUFFICIENT_BUFFER == lastError)
                                {
                                    // we got the size, allocate buffer
                                    SP_DEVICE_INTERFACE_DETAIL_DATA *pInterfaceDetailData;
                                    pInterfaceDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA*)malloc(RequiredSize);
                                    pInterfaceDetailData->cbSize = 6;//sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

                                    //getting DevicePath in interface detail data struct
                                    if (false == fcall_SetupDiGetDeviceInterfaceDetail(hdeviceinfo, (SP_DEVICE_INTERFACE_DATA &)devInterData, pInterfaceDetailData, RequiredSize, UINT32ZERO, (SP_DEVINFO_DATA &)devInfoData))
                                    {
                                        lastError = fcall_getLastError();
                                    }
                                    else
                                    {
                                        lastError = fcall_getLastError();
                                        if( CR_SUCCESS == lastError)
                                        {
                                            str = QString::fromRawData((QChar*)pInterfaceDetailData->DeviceName, ((RequiredSize >> 1)-3));
                                            str = str.toUpper();
                                            if(str.contains("VID_0451") && str.contains("PID_C900") && str.contains("MI_01"))
                                            {
                                                lastError = CR_SUCCESS;
                                                //Open file USBBulkhandle to the BULK Devices
                                                USBBulkhandle = fcall_CreateFile((wchar_t *)pInterfaceDetailData->DeviceName, READWRITE, 0, 0, 3, FILE_FLAG_OVERLAPPED, 0);
                                                lastError = fcall_getLastError();
                                                if( USBBulkhandle != INVALID_HANDLE_VALUE)
                                                {
                                                    if( false == fcall_WinUsb_Initialize(USBBulkhandle, &USBWinDrvHandle))
                                                    {
                                                        lastError = fcall_getLastError();
                                                        fcall_CloseHandle(USBBulkhandle);
                                                        USBBulkhandle = NULL;
                                                        USBWinDrvHandle = NULL;
                                                    }
                                                    else
                                                    {
                                                        UCHAR pID = 0x82;
                                                        ULONG timeout = 5000;//ms
                                                        if(false == fcall_WinUsb_SetPipePolicy( USBWinDrvHandle, pID, PIPE_TRANSFER_TIMEOUT, sizeof(ULONG), &timeout))
                                                        {
                                                            fcall_WinUsb_Free(USBWinDrvHandle);
                                                            fcall_CloseHandle(USBBulkhandle);
                                                            USBBulkhandle = NULL;
                                                            USBWinDrvHandle = NULL;
                                                        }
                                                        lastError = fcall_getLastError();
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                lastError = -1;
                                            }
                                        }
                                    }
                                    free(pInterfaceDetailData);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return lastError;
}
