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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

//#include "fastboot.h"
#include "libusb.h"

char ERRORX[128];

int usb_read(usb_dev_handle* handle, struct usb_device* dev, void* data, int len);
int usb_write(usb_dev_handle* handle, struct usb_device* dev, const void* data, int len);
//int usb_close2(usb_dev_handle* handle);

char *fb_get_error(void)
{
    return ERRORX;
}

static int check_response(usb_dev_handle *usb, struct usb_device *dev, unsigned size, 
                          unsigned data_okay, char *response)
{
    unsigned char status[65];
    int r;
    int retry = 0;

    for(;;) {
        //printf("Dev: %p\n", dev);
        r = usb_read(usb, dev, status, 64);
        if (r == -116)
        {
            sleep(1); //we need to wait flash complete
            if (retry++ > 30) break;
            continue;
        }
        if(r < 0) {
            sprintf(ERRORX, "status read failed (%s)", strerror(errno));
            //usb_close2(usb);
            return -1;
        }
        status[r] = 0;

        if(r < 4) {
            sprintf(ERRORX, "status malformed (%d bytes)", r);
            //usb_close2(usb);
            return -1;
        }

        if(!memcmp(status, "INFO", 4)) {
            fprintf(stderr,"(bootloader) %s\n", status + 4);
            continue;
        }

        if(!memcmp(status, "OKAY", 4)) {
            if(response) {
                strcpy(response, (char*) status + 4);
            }
            return 0;
        }

        if(!memcmp(status, "FAIL", 4)) {
            if(r > 4) {
                sprintf(ERRORX, "remote: %s", status + 4);
            } else {
                strcpy(ERRORX, "remote failure");
            }
            return -1;
        }

        if(!memcmp(status, "DATA", 4) && data_okay){
            unsigned dsize = strtoul((char*) status + 4, 0, 16);
            if(dsize > size) {
                strcpy(ERRORX, "data size too large");
                //usb_close2(usb);
                return -1;
            }
            return dsize;
        }

        strcpy(ERRORX,"unknown status code");
        //usb_close2(usb);
        break;
    }

    return -1;
}

static int _command_send(usb_dev_handle *usb, struct usb_device *dev, const char *cmd,
                         const void *data, unsigned size,
                         char *response)
{
    int cmdsize = (int)strlen(cmd);
    int r;
    
    if(response) {
        response[0] = 0;
    }

    if(cmdsize > 64) {
        sprintf(ERRORX,"command too large");
        return -1;
    }

    if(usb_write(usb, dev, cmd, cmdsize) != cmdsize) {
        sprintf(ERRORX,"command write failed (%s)", strerror(errno));
        //usb_close2(usb);
        return -1;
    }

    if(data == 0) {
        return check_response(usb, dev, size, 0, response);
    }

    r = check_response(usb, dev, size, 1, 0);
    if(r < 0) {
        return -1;
    }
    size = r;

    if(size) {
        r = usb_write(usb, dev, data, size);
        if(r < 0) {
            sprintf(ERRORX, "data transfer failure (%s)", strerror(errno));
            //usb_close2(usb);
            return -1;
        }
        if(r != ((int) size)) {
            sprintf(ERRORX, "data transfer failure (short transfer)");
            //usb_close2(usb);
            return -1;
        }
    }
    
    r = check_response(usb, dev, 0, 0, 0);
    if(r < 0) {
        return -1;
    } else {
        return size;
    }
}

int fb_command(usb_dev_handle *usb, struct usb_device *dev, const char *cmd)
{
    return _command_send(usb, dev, cmd, 0, 0, 0);
}

int fb_command_response(usb_dev_handle *usb, struct usb_device *dev, const char *cmd, char *response)
{
    return _command_send(usb, dev, cmd, 0, 0, response);
}

int fb_download_data(usb_dev_handle *usb, struct usb_device *dev, const void *data, unsigned size)
{
    char cmd[64];
    int r;

    sprintf(cmd, "download:%08x", size);
    r = _command_send(usb, dev, cmd, data, size, 0);
    
    if(r < 0) {
        return -1;
    } else {
        return 0;
    }
}

