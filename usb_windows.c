/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the 
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <windows.h>
#include <winerror.h>
#include <errno.h>
#include <usb100.h>
#include <stdio.h>

#include "libusb.h"

//#define TRACE_USB 1
#if TRACE_USB
#define DBG(...) printf(__VA_ARGS__)
#else
#define DBG(...)
#endif

#define JUST_BREAK_INSIDE 1

static int USB_ENDPOINT_IN = 0x83;
static int USB_ENDPOINT_OUT = 0x02;
static int MAX_USBFS_BULK_SIZE = 0x200;

/// Checks if interface (device) matches certain criteria
int recognized_device(usb_dev_handle* handle, ifc_match_func callback);

/// Writes data to the opened usb handle
int usb_write(usb_dev_handle* handle, struct usb_device* dev, const void* data, int len);

/// Reads data using the opened usb handle
int usb_read(usb_dev_handle *handle, struct usb_device* dev, void* data, int len);

/// Cleans up opened usb handle
//void usb_cleanup_handle(usb_dev_handle* handle);

/// Cleans up (but don't close) opened usb handle
void usb_kick(usb_dev_handle* handle);

/// Closes opened usb handle
//int usb_close2(usb_dev_handle* handle);
void get_endpoint(struct usb_device* dev);

int usb_write(usb_dev_handle* handle, struct usb_device* dev, const char* data, int len) {
    unsigned long written = 0;
    unsigned count = 0;
    int ret;

    DBG("usb_write %d\n", len);
    
    if (handle != NULL && dev != NULL)
    {
      while(len > 0) {
        int xfer;
        xfer = (len > MAX_USBFS_BULK_SIZE) ? MAX_USBFS_BULK_SIZE : len;
        
        ret = usb_bulk_write(handle, USB_ENDPOINT_OUT, (char *)data, xfer, 0);
        
        if(ret != xfer) {
            DBG("ERROR: ret = %d, errno = %d (%s)\n",
                ret, errno, strerror(errno));
            return -1;
        }

        count += xfer;
        len -= xfer;
        data += xfer;
      }
    }
  return count;
}

int usb_read(usb_dev_handle* handle, struct usb_device* dev, char* data, int len) {
    unsigned long written = 0;
    unsigned count = 0;
    int n;
    int timeout = 500;

    DBG("usb_read %d\n", len);

    if (handle != NULL && dev != NULL)
    {
        while (len > 0) {
            int xfer;
            xfer = (len > MAX_USBFS_BULK_SIZE) ? MAX_USBFS_BULK_SIZE : len;

            n = usb_bulk_read(handle, USB_ENDPOINT_IN, (char*)data, xfer, timeout);

            count += n;
            len -= n;
            data += n;

            if (n < xfer) {
                break;
            }
        }
    }
    return count;
}

void usb_kick(usb_dev_handle* handle) {
    if (NULL != handle) {
        //usb_cleanup_handle(handle);
    } else {
        SetLastError(ERROR_INVALID_HANDLE);
        errno = ERROR_INVALID_HANDLE;
    }
}

extern usb_dev_handle* usb;
int usb_close2(usb_dev_handle* handle) {
    DBG("usb_close\n");

    if (NULL != handle) {
        // Cleanup handle
        usb_close(handle);
        //free(handle);
    }

    return 0;
}


usb_dev_handle* find_usb_device(ifc_match_func callback, struct usb_device** dev) {

    int printed = 0; //has been < waiting for device > printed?
    // libusb variable
    struct usb_bus* busses = NULL;
    // from usb.h, no need to some hard test over tons of functions
    struct usb_ifc_info info;

    // libusb inicialization
    usb_init();
    //usb_set_debug(4);

    // just following example - find busses and devices
    while (JUST_BREAK_INSIDE)
    { // make three whiles and loop until world's end or found device or user interrupt

        // get list of busses
        usb_find_busses();
        usb_find_devices();
        busses = usb_get_busses();

        // cycle the busses and devices			 
        while (busses != NULL)
        {
            //busses = busses->next;

            while (busses->devices != NULL)
            {
                // compatibility
                info.dev_vendor = busses->devices->descriptor.idVendor;
                //info.dev_product = busses->devices->descriptor.idProduct;
                info.ifc_class = busses->devices->config->interface->altsetting->bInterfaceClass;
                info.ifc_subclass = busses->devices->config->interface->altsetting->bInterfaceSubClass;
                info.ifc_protocol = busses->devices->config->interface->altsetting->bInterfaceProtocol;

                //printf("Vendor: %x\n", busses->devices->descriptor.idVendor);
                //printf("Class: %x\n", busses->devices->config->interface->altsetting->bInterfaceClass);
                //printf("Subclass: %x\n", busses->devices->config->interface->altsetting->bInterfaceSubClass);
                //printf("Protocol: %x\n", busses->devices->config->interface->altsetting->bInterfaceProtocol);


                // callback to match_fastboot
                if (callback(&info) == 0)
                {
                    // device passed the test to identity (Kindle), return handle
                    // first return dev descriptor
                    *dev = busses->devices;
                    if (dev == NULL)
                        printf("Dev error.\n");
                    // open usb through libusb
                    get_endpoint(busses->devices);
                    return usb_open(busses->devices);
                }
                else
                {
                    if (printed == 0)
                    {
                        fprintf(stderr, "< waiting for device >\n");
                        printed = 1;
                    }
                    busses->devices = busses->devices->next;
                }
            }
            if (printed == 0)
            {
                fprintf(stderr, "< waiting for device >\n");
                printed = 1;
            }
            busses = busses->next;
        }
    }
}

usb_dev_handle *usb_open2(ifc_match_func callback, struct usb_device **dev)
{
    return find_usb_device(callback, dev);
}

// called from fastboot.c
void sleep(int seconds)
{
    Sleep(seconds * 1000);
}

void get_endpoint(struct usb_device* dev)
{
    int bNumEndpoints = dev->config->interface->altsetting->bNumEndpoints;
    struct usb_endpoint_descriptor* endpoint = dev->config->interface->altsetting->endpoint;
    for (size_t i = 0; i < bNumEndpoints; i++)
    {
        if (endpoint->bEndpointAddress >= 0x80) USB_ENDPOINT_IN = endpoint->bEndpointAddress;
        else USB_ENDPOINT_OUT = endpoint->bEndpointAddress;
        MAX_USBFS_BULK_SIZE = endpoint->wMaxPacketSize;
        endpoint++;
    }
}