/*
  SerialUploader.cpp - Fast upload to printer SD card using 
  SerialTransfer library

  Copyright (c) 2020 Andrey Azarov. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "espcom.h"
#include "SerialUploader.h"
#include "webinterface.h"

#define ESP_SU_LOGI(...)              ESP_LOGI("serupload", __VA_ARGS__)
#define ESP_SU_LOGW(...)              ESP_LOGW("serupload", __VA_ARGS__)
#define ESP_SU_LOGE(...)              ESP_LOGE("serupload", __VA_ARGS__)



#define LINE_MAX_SIZE 256

SerialTransfer SerialUploader::_fileTransfer;

byte SerialUploader::_sdSlot = 0;
char SerialUploader::_basepath[] = {0};
int32_t SerialUploader::_transfered = 0; 

void SerialUploader::initialize(Stream &port, byte sdSlot, const char * const basePath)
{
    _sdSlot = sdSlot;
    strcpy(_basepath, basePath);  
    _fileTransfer.begin(port, false);
}

bool SerialUploader::waitResponse(uint32_t timeout, char * responce_buffer, uint16_t buffer_length ,const char * const prefix)
{
    uint32_t start = millis();
    uint16_t responce_length = 0;

    while (true)
    {
        if (ESPCOM::available(DEFAULT_PRINTER_PIPE))
        {
            char c = ESPCOM::readByte(DEFAULT_PRINTER_PIPE);
            if (c == '\n' || c == '\r' || responce_length == buffer_length-1)
            {
                responce_buffer[responce_length] = 0;
                char* prefix_pos = strstr(responce_buffer, prefix);
                if (prefix_pos!=nullptr)
                {
                    return true;
                }
                else
                {
                    responce_length = 0;
                }
            }
            else
            {
                responce_buffer[responce_length] = c;
                responce_length++;
            }
            if ((millis() - start) > timeout) return false;
        }
    }
}

bool SerialUploader::beginFileTransfer(const char * const filename, uint32_t size)
{
    ESPCOM::printerSerialLock(SERIAL_LOCK_ACCLIENT);
    if (purge_serial()) return false;
    //send file transfer command
    char buf[LINE_MAX_SIZE] = {0};
    snprintf(buf, LINE_MAX_SIZE, "M28.1 P%d S%d %s%s", _sdSlot, size, _basepath, filename);
    ESPCOM::println(buf, DEFAULT_PRINTER_PIPE);
    ESPCOM::flush(DEFAULT_PRINTER_PIPE);
    memset(buf, 0, LINE_MAX_SIZE);
    if (waitResponse(30000, buf, LINE_MAX_SIZE, "RESULT:"))
    {
        if (strstr(buf, "READY"))
        {
            _transfered = 0;
            return true;
        }
    }
    ESP_SU_LOGW("File transfer can't be started");
    return false;
}

int32_t SerialUploader::processFileTransfer(const char * block, uint32_t block_size)
{
    uint32_t local_transfered = 0;
    while (local_transfered < block_size)
    {
        //Transfer
        uint16_t packet_size = MAX_PACKET_SIZE;
        if ((block_size - local_transfered) < packet_size) packet_size = block_size - local_transfered;
        for (uint16_t i = 0; i < packet_size; i++) _fileTransfer.txBuff[i] = block[local_transfered+i];
        _fileTransfer.sendData(packet_size);
        //Responce
        char buf[LINE_MAX_SIZE] = {0};
        if (waitResponse(30000, buf, LINE_MAX_SIZE, "RESULT:"))
        {
            if (strstr(buf, "ok"))
            {
                local_transfered += packet_size;
            }
            else if (strstr(buf, "RESEND"))
            {
                ESP_SU_LOGI("Resending file block");
            }
            else if (strstr(buf, "ERROR"))
            {
                ESP_SU_LOGW("File transfer error");
                return -1;
            }
        }
        else
        {
            ESP_SU_LOGW("File transfer error");
            return -1;
        }
    }
    _transfered += block_size;
    return _transfered;
}

