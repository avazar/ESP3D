/*
  ACClient.h - Aura Connect Client class

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

#pragma once
#include "mqtt_client.h"
#include "config.h"
#include "espcom.h"
#include "SerialUploader/SerialUploader.h"

class ACClient {
private:
    static char _clientId[MAX_ID_LENGTH + 1];
    static esp_mqtt_client_handle_t _client;

    static int _fileTransfered;
    
    static uint16_t generateErrorMessage(char* dest_buffer, const int buffer_length, const char *message);

    static esp_err_t mqttEventHandler(esp_mqtt_event_handle_t event);
    static inline esp_err_t mqttDataHandler(esp_mqtt_event_handle_t event);
    static void receiveFileData(const char *data, int data_len, int current_data_offset, int total_data_len);

    static bool initClient(const char *uri, uint32_t port, const char *cert_pem, const char *client_id, const char *username, const char *password);
public:
    static bool begin();
};
