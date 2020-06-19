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

#include "SerialUploader.h"


SerialTransfer SerialUploader::_fileTransfer;

byte SerialUploader::_sdSlot = 0;
char SerialUploader::_basepath[] = {0};
uint32_t SerialUploader::_transfered = 0; 
uint32_t SerialUploader::_size = 0; 

void SerialUploader::initialize(Stream &port, byte sdSlot, const char * const basePath)
{
    _sdSlot = sdSlot;
    strcpy(_basepath, basePath);  
    _fileTransfer.begin(port, false);
}

bool SerialUploader::beginFileTransfer(const char * const filename, uint32_t size)
{
    _transfered = 0;
    _size = size;
    
    
}