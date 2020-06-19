/*
  espcom.h - esp3d communication serial/tcp/etc... class

  Copyright (c) 2014 Luc Lebosse. All rights reserved.

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

#ifndef ESPCOM_H
#define ESPCOM_H
#include <WiFiServer.h>
#include "config.h"
#ifdef TCP_IP_DATA_FEATURE
extern WiFiServer * data_server;
#endif

typedef uint8_t flag_seriallock_t;
#define SERIAL_NOT_LOCKED            ((flag_seriallock_t)0x00u) /** Not locked */
#define SERIAL_LOCK_WEBUI            ((flag_seriallock_t)0x01u) /** Serial locked by web ui */
#define SERIAL_LOCK_ACCLIENT         ((flag_seriallock_t)0x02u) /** Serial locked by Aura Connect client */

class ESPCOM
{
private:
    static flag_seriallock_t lock_flag;
public:
    static bool printerSerialLocked(flag_seriallock_t exclude = SERIAL_NOT_LOCKED);
    static void printerSerialLock(flag_seriallock_t lockReason);
    static void printerSerialUnlock(flag_seriallock_t lockReason);
    static void printerSerialUnlockAll();
    static size_t  write(tpipe output, uint8_t d);
    static long readBytes (tpipe output, uint8_t * sbuf, size_t len);
    static long baudRate(tpipe output);
    static size_t available(tpipe output);
    static void flush(tpipe output, ESPResponseStream  *espresponse = NULL);
    static void bridge(bool async = false);
    static bool processFromSerial (bool async = false);
    static void print (const __FlashStringHelper *data, tpipe output, ESPResponseStream  *espresponse = NULL);
    static void print (String & data, tpipe output, ESPResponseStream  *espresponse = NULL);
    static void print (const char * data, tpipe output, ESPResponseStream  *espresponse = NULL);
    static void println (const __FlashStringHelper *data, tpipe output, ESPResponseStream  *espresponse = NULL);
    static void println (String & data, tpipe output, ESPResponseStream  *espresponse = NULL);
    static void println (const char * data, tpipe output, ESPResponseStream  *espresponse = NULL);
    static uint8_t current_socket_id;
#ifdef TCP_IP_DATA_FEATURE
    static void processFromTCP2Serial();
    static void send2TCP (const __FlashStringHelper *data, bool async = false);
    static void send2TCP (String data, bool async = false);
    static void send2TCP (const char * data, bool async = false);
#endif
#ifdef ESP_OLED_FEATURE
    static bool block_2_oled;
#endif
};
#endif
