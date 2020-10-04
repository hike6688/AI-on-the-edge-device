#include "interface_mqtt.h"


#include "esp_log.h"
#include "mqtt_client.h"
#include "ClassLogFile.h"

static const char *TAG = "interface_mqtt";

bool debugdetail = true;

// #define CONFIG_BROKER_URL "mqtt://192.168.178.43:1883"

esp_mqtt_event_id_t esp_mmqtt_ID = MQTT_EVENT_ANY;

bool mqtt_connected = false;
esp_mqtt_client_handle_t client = NULL;

void MQTTPublish(std::string _key, std::string _content){
    if (client && mqtt_connected) {
        int msg_id;
        std::string zw;
        msg_id = esp_mqtt_client_publish(client, _key.c_str(), _content.c_str(), 0, 1, 0);
        zw = "sent publish successful in MQTTPublish, msg_id=" + std::to_string(msg_id) + ", " + _key + ", " + _content;
        if (debugdetail) LogFile.WriteToFile(zw);
        ESP_LOGI(TAG, "sent publish successful in MQTTPublish, msg_id=%d, %s, %s", msg_id, _key.c_str(), _content.c_str());
    }
    else {
        ESP_LOGI(TAG, "Problem with Publish, client=%d, mqtt_connected %d", (int) client, (int) mqtt_connected);
    }
}


static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            mqtt_connected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb((esp_mqtt_event_handle_t) event_data);
}

void MQTTInit(std::string _mqttURI, std::string _clientid){
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = _mqttURI.c_str(),
        .client_id = _clientid.c_str(),
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, esp_mmqtt_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}