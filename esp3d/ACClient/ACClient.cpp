/*
  ACClient.cpp - Aura Connect Client class

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

#include "ACClient.h"
#include "cJSON.h"

#define ESP_ACC_LOGI(...)              ESP_LOGI("acclient", __VA_ARGS__)
#define ESP_ACC_LOGW(...)              ESP_LOGW("acclient", __VA_ARGS__)
#define ESP_ACC_LOGE(...)              ESP_LOGE("acclient", __VA_ARGS__)

#define MQ_TOPIC_COMMAND "com"
#define MQ_TOPIC_FILE "file"
#define MQ_TOPIC_TELEMETRY "tel"

#define MAX_FILE_BLOCK_RESENDS 10

esp_mqtt_client_handle_t ACClient::_client;
char ACClient::_clientId[] = {0};

int ACClient::_fileTransfered = -1;
int ACClient::_fileSize = 0;
uint8_t ACClient::_fileResends = 0;

esp_err_t ACClient::mqttEventHandler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    String topic;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_ACC_LOGI("MQTT_EVENT_CONNECTED");

            topic = "/" + String(_clientId) + "/" + MQ_TOPIC_COMMAND;
            msg_id = esp_mqtt_client_subscribe(client, topic.c_str(), 2);
            ESP_ACC_LOGI("sent COM subscribe successful, msg_id=%d", msg_id);

            topic = "/" + String(_clientId) + "/" + MQ_TOPIC_FILE;
            msg_id = esp_mqtt_client_subscribe(client, topic.c_str(), 1);
            ESP_ACC_LOGI("sent FILE subscribe successful, msg_id=%d", msg_id);

            topic = "/" + String(_clientId) + "/" + MQ_TOPIC_TELEMETRY;
            msg_id = esp_mqtt_client_subscribe(client, topic.c_str(), 0);
            ESP_ACC_LOGI("sent TEL subscribe successful, msg_id=%d", msg_id);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_ACC_LOGI("MQTT_EVENT_DISCONNECTED");
            //TODO: Handle file transfer timeout
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_ACC_LOGI("MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_ACC_LOGI("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_ACC_LOGI("MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_ACC_LOGI("MQTT_EVENT_DATA");
            return mqttDataHandler(event);
            break;
        case MQTT_EVENT_ERROR:
            ESP_ACC_LOGI("MQTT_EVENT_ERROR");
            break;
        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_ACC_LOGI("MQTT_EVENT_BEFORE_CONNECT");
            break;
        default:
            ESP_ACC_LOGI("Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

esp_err_t ACClient::mqttDataHandler(esp_mqtt_event_handle_t event) {
    //topic matches id
    if (lwip_strnstr(event->topic, _clientId, event->topic_len)!=nullptr)
    {
        //skip id in topic
        char* subtopic = lwip_strnstr(event->topic, "/", event->topic_len);
        int subtopic_len = event->topic_len-(subtopic-event->topic);
        if (subtopic == nullptr) 
        {
            ESP_ACC_LOGW("No topic type");
            return ESP_OK;
        }
        if (lwip_strnstr(subtopic, MQ_TOPIC_COMMAND, subtopic_len))
        {
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
        }
        else if (lwip_strnstr(subtopic, MQ_TOPIC_FILE, subtopic_len))
        {
            ESP_ACC_LOGI("msg id:%d", event->msg_id);
            receiveFileData(event->data, event->data_len, event->current_data_offset, event->total_data_len);
        }
        else if (lwip_strnstr(subtopic, MQ_TOPIC_TELEMETRY, subtopic_len))
        {
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
        }
        else
        {
            ESP_ACC_LOGW("Wrong topic type");
        }
    }
    else
    {
        ESP_ACC_LOGW("Wrong id in topic");
    }
    
    return ESP_OK;

}

void ACClient::sendFileErrorMessage(const char *message)
{
    uint16_t len = 0;
    char buffer[256];
    cJSON *root;
    String topic = "/" + String(_clientId) + "/" + MQ_TOPIC_FILE;
    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "response", "error");
    cJSON_AddStringToObject(root, "msg", message);
    if (cJSON_PrintPreallocated(root, buffer, 256, false))
    {
        len = strlen(buffer);
    }
    cJSON_Delete(root);
    esp_mqtt_client_publish(_client, topic.c_str(), buffer, len, 1, 0);

}

void ACClient::sendFileContinueMessage()
{
    char buffer[] = "{response:continue}"; 
    cJSON *root;
    String topic = "/" + String(_clientId) + "/" + MQ_TOPIC_FILE;
    esp_mqtt_client_publish(_client, topic.c_str(), buffer, strlen(buffer), 1, 0);
}

void ACClient::sendFileFinishMessage()
{
    char buffer[] = "{response:finish}"; 
    cJSON *root;
    String topic = "/" + String(_clientId) + "/" + MQ_TOPIC_FILE;
    esp_mqtt_client_publish(_client, topic.c_str(), buffer, strlen(buffer), 1, 0);
}

void ACClient::sendFileResendBlockMessage(uint32_t index)
{
    uint16_t len = 0;
    char buffer[256];
    cJSON *root;
    String topic = "/" + String(_clientId) + "/" + MQ_TOPIC_FILE;
    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "response", "resend");
    cJSON_AddNumberToObject(root, "index", index);
    if (cJSON_PrintPreallocated(root, buffer, 256, false))
    {
        len = strlen(buffer);
    }
    cJSON_Delete(root);
    esp_mqtt_client_publish(_client, topic.c_str(), buffer, len, 1, 0);
}

void ACClient::receiveFileData(const char *data, int data_len, int current_data_offset, int total_data_len)
{
    if (ESPCOM::printerSerialLocked(SERIAL_LOCK_ACCLIENT))
    {
        sendFileErrorMessage("Serial connection to printer is busy");
    }

    //Start transfer
    if (_fileTransfered == -1)
    {
        cJSON *json = cJSON_ParseWithLength(data, data_len);
        if (json != nullptr)
        {
            char* filename = cJSON_GetObjectItem(json,"filename")->valuestring;
            uint32_t filesize = cJSON_GetObjectItem(json, "size")->valueint;
            if (SerialUploader::beginFileTransfer(filename, filesize))
            {
                ESP_ACC_LOGI("starting file transfer: %s, %d", filename, filesize);
                _fileSize = filesize;
                _fileTransfered = 0;  
                _fileResends = 0;              
                sendFileContinueMessage();               
            }
            else
            {
                sendFileErrorMessage("File transfer can't be started");
            }          
            cJSON_Delete(json);
        }
    }
    //Transfer in progress
    else if (_fileTransfered >= 0)
    {
        int block_start = int((byte)(*data) << 24 |
                          (byte)(*(data+1)) << 16 |
                          (byte)(*(data+2)) << 8 |
                          (byte)(*(data+3)));
        if (block_start != _fileTransfered)
        {
            _fileResends++;
            if (_fileResends < MAX_FILE_BLOCK_RESENDS)
            {
                sendFileResendBlockMessage(_fileTransfered);
            }
            else
            {
                _fileTransfered = -1;
                sendFileErrorMessage("Max block resend number reached");
                //TODO: cancel file transfer
            }
            return;
        }
        else
        {
            _fileResends = 0;
            int32_t result = SerialUploader::processFileTransfer(data+sizeof(int), data_len-sizeof(int));
            if (result!=-1) 
            {
                _fileTransfered = result;
                if (_fileTransfered  == _fileSize) 
                {
                    _fileTransfered = -1;
                    sendFileFinishMessage();
                }
                else if (_fileTransfered < _fileSize)
                {
                    sendFileContinueMessage();
                }
                else
                {
                    _fileTransfered = -1;
                    sendFileErrorMessage("Data size larger than file size");
                }
            }
            else
            {
            _fileTransfered = -1;
            sendFileErrorMessage("File transfer error");
            }
            

        }
    }
}



bool ACClient::initClient(const char *uri, uint32_t port, const char *cert_pem, const char *client_id, const char *username, const char *password) {
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.event_handle = ACClient::mqttEventHandler;
    mqtt_cfg.uri = "ws://192.168.1.50/";
    mqtt_cfg.port = 3000;
    //mqtt_cfg.transport = MQTT_TRANSPORT_OVER_WS;
    mqtt_cfg.client_id = "cli";
    //mqtt_cfg.buffer_size = 2048;
    //mqtt_cfg.username = username,
    //mqtt_cfg.password = password,
    //mqtt_cfg.cert_pem = cert_pem,

    strcpy(_clientId, client_id);
    _client = esp_mqtt_client_init(&mqtt_cfg);
    //esp_err_t res = esp_mqtt_client_start(_client);
    //if (res!=ESP_OK) 
    //{
    //    ESP_ACC_LOGE("Can't start MQTT client");
    //    return false;
    //}
    ESP_ACC_LOGI("[APP] Free memory: %d bytes", esp_get_free_heap_size());
    return true;
}

bool ACClient::begin()
{
    char server_uri[MAX_DATA_LENGTH + 1];
    int server_port;
    char client_id[MAX_ID_LENGTH + 1];
    char password[MAX_PASSWORD_LENGTH + 1];

    if (!CONFIG::read_string (AP_AC_SERVER, server_uri, MAX_DATA_LENGTH) ) {
        ESP_ACC_LOGE("Can't read MQTT server address from config");
        return false;
    }
    if (!CONFIG::read_buffer (AP_AC_PORT,  (byte *) &server_port, INTEGER_LENGTH) ) {
        ESP_ACC_LOGE("Can't read MQTT server port from config");
    } 
    if (!CONFIG::read_string (AP_AC_CLIENT_ID, client_id, MAX_ID_LENGTH) ) {
        ESP_ACC_LOGE("Can't read MQTT client id from config");
        return false;
    }
    if (!CONFIG::read_string (AP_AC_SERVER, password, MAX_PASSWORD_LENGTH) ) {
        ESP_ACC_LOGE("Can't read MQTT client password from config");
        return false;
    }

    //return initClient(server_uri, server_port, "", client_id, client_id, password);
    return initClient(server_uri, server_port, "", "cli", client_id, password);
}