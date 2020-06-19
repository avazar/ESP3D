/*
  SerialUploader.h - Fast upload to printer SD card using 
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

#include "SerialTransfer.h"

#define MAX_BASEPATH_LENGTH 32

class SerialUploader
{
private:
	static SerialTransfer _fileTransfer;

    static byte _sdSlot;
    static char _basepath[MAX_BASEPATH_LENGTH+1];
    static uint32_t _transfered; 
    static uint32_t _size; 
public:
    static void initialize(Stream &port, byte sdSlot, const char * const basePath);
    static bool beginFileTransfer(const char * const filename, uint32_t size);

};
