#include "WiFi_Connect.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "lwip/inet.h"
#include "nvs_flash.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "dns_server.h"
#include "src/libs/qrcode/lv_qrcode.h"

#define WIFI_CONNECT_IDLE_MS          (1000)
#define WIFI_CONNECT_UI_REFRESH_MS    (300)
#define WIFI_CONNECT_AP_CHANNEL       (1)
#define WIFI_CONNECT_AP_MAX_CONN      (4)
#define WIFI_CONNECT_AP_PASSWORD      "12345678"
#define WIFI_CONNECT_AP_URL           "http://192.168.4.1"
#define WIFI_CONNECT_CAPTIVE_URI_MAX  (32)
#define WIFI_CONNECT_HTTP_MAX_SOCKETS (4)
#define WIFI_CONNECT_HTTP_BACKLOG     (2)
#define WIFI_CONNECT_HTTP_RECV_TIMEOUT_SEC (2)
#define WIFI_CONNECT_HTTP_SEND_TIMEOUT_SEC (2)
#define WIFI_CONNECT_STATUS_BAR_WIDTH (220)
#define WIFI_CONNECT_STATUS_BAR_HEIGHT_MARQUEE (40)
#define WIFI_CONNECT_STATUS_BAR_HEIGHT_DETAIL  (72)
#define WIFI_CONNECT_STATUS_BAR_BOTTOM_PERCENT (8)
#define WIFI_CONNECT_QR_PAYLOAD_FORMAT \
    "WIFI:T:WPA;S:%s;P:%s;;"

#define BSP_DISPLAY_PARTIAL_MODE_HEIGHT 10


typedef enum {
    WIFI_CONNECT_STATE_INIT = 0,
    WIFI_CONNECT_STATE_AP_READY,
    WIFI_CONNECT_STATE_WAIT_CONFIG,
    WIFI_CONNECT_STATE_CONNECTING,
    WIFI_CONNECT_STATE_CONNECTED,
    WIFI_CONNECT_STATE_FAILED,
} wifi_connect_state_t;

static const char *TAG = "wifi_connect";

static httpd_handle_t wifi_connect_httpd;
static dns_server_handle_t wifi_connect_dns_server;
static wifi_connect_state_t wifi_connect_state;
static lv_obj_t *wifi_connect_status_bar;
static lv_obj_t *wifi_connect_status_label;
static char wifi_connect_ap_ssid[32];
static char wifi_connect_sta_ssid[33];
static char wifi_connect_sta_ip[16];
static char wifi_connect_captive_uri[WIFI_CONNECT_CAPTIVE_URI_MAX];
static char wifi_connect_last_status_text[192];
static bool wifi_connect_last_use_marquee;

static void wifi_connect_url_decode(char *text)
{
    char *src = text;
    char *dst = text;

    while (*src != '\0') {
        if ((src[0] == '%') && (src[1] != '\0') && (src[2] != '\0')) {
            int value = 0;

            if (sscanf(src + 1, "%2x", &value) == 1) {
                *dst++ = (char)value;
                src += 3;
                continue;
            }
        }

        if (*src == '+') {
            *dst++ = ' ';
            src++;
            continue;
        }

        *dst++ = *src++;
    }

    *dst = '\0';
}

static void wifi_connect_set_state(wifi_connect_state_t new_state)
{
    wifi_connect_state = new_state;
}

static void wifi_connect_set_close_header(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Connection", "close");
}

static void wifi_connect_status_label_x_cb(void *var, int32_t value)
{
    lv_obj_set_x((lv_obj_t *)var, (int32_t)value);
}

static void wifi_connect_stop_status_animation(void)
{
    if (wifi_connect_status_label == NULL) {
        return;
    }

    lv_anim_delete(wifi_connect_status_label, wifi_connect_status_label_x_cb);
    lv_obj_set_x(wifi_connect_status_label, 0);
}

static void wifi_connect_start_status_animation(void)
{
    lv_anim_t anim;
    int32_t bar_width = 0;
    int32_t label_width = 0;

    if ((wifi_connect_status_bar == NULL) || (wifi_connect_status_label == NULL)) {
        return;
    }

    lv_obj_update_layout(wifi_connect_status_bar);
    lv_obj_update_layout(wifi_connect_status_label);
    bar_width = lv_obj_get_content_width(wifi_connect_status_bar);
    label_width = lv_obj_get_width(wifi_connect_status_label);
    wifi_connect_stop_status_animation();

    if (label_width <= bar_width) {
        return;
    }

    lv_anim_init(&anim);
    lv_anim_set_var(&anim, wifi_connect_status_label);
    lv_anim_set_exec_cb(&anim, wifi_connect_status_label_x_cb);
    lv_anim_set_values(&anim, bar_width, -label_width);
    lv_anim_set_duration(&anim, (uint32_t)((bar_width + label_width) * 15));
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&anim, lv_anim_path_linear);
    lv_anim_start(&anim);
}

static bool wifi_connect_is_same_ssid_connected(const char *ssid)
{
    wifi_ap_record_t ap_info = {0};

    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
        return false;
    }

    return strncmp((const char *)ap_info.ssid, ssid, sizeof(ap_info.ssid)) == 0;
}

static esp_err_t wifi_connect_apply_sta_config(const char *ssid, const char *password)
{
    wifi_config_t config = {0};

    if (wifi_connect_is_same_ssid_connected(ssid)) {
        strlcpy(wifi_connect_sta_ssid, ssid, sizeof(wifi_connect_sta_ssid));
        wifi_connect_set_state(WIFI_CONNECT_STATE_CONNECTED);
        return ESP_OK;
    }

    if (wifi_connect_state == WIFI_CONNECT_STATE_CONNECTED) {
        ESP_ERROR_CHECK(esp_wifi_disconnect());
    }

    strlcpy((char *)config.sta.ssid, ssid, sizeof(config.sta.ssid));
    strlcpy((char *)config.sta.password, password, sizeof(config.sta.password));
    config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    config.sta.pmf_cfg.capable = true;
    config.sta.pmf_cfg.required = false;

    strlcpy(wifi_connect_sta_ssid, ssid, sizeof(wifi_connect_sta_ssid));
    wifi_connect_sta_ip[0] = '\0';
    wifi_connect_set_state(WIFI_CONNECT_STATE_CONNECTING);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
    return esp_wifi_connect();
}

static esp_err_t wifi_connect_index_get_handler(httpd_req_t *req)
{
    static const char html[] =
        "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>WiFi Setup</title></head><body>"
        "<h2>WiFi Setup</h2>"
        "<form method=\"post\" action=\"/configure\">"
        "<label>SSID</label><br><input name=\"ssid\" maxlength=\"32\"><br><br>"
        "<label>Password</label><br><input name=\"password\" type=\"password\" maxlength=\"64\"><br><br>"
        "<button type=\"submit\">Connect</button></form>"
        "<p>Submit home router credentials and wait for the watch screen status update.</p>"
        "</body></html>";

    wifi_connect_set_close_header(req);
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t wifi_connect_redirect_to_root(httpd_req_t *req, const char *body)
{
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    wifi_connect_set_close_header(req);
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, body, HTTPD_RESP_USE_STRLEN);
}

static const char *wifi_connect_state_to_text(wifi_connect_state_t current_state)
{
    switch (current_state) {
    case WIFI_CONNECT_STATE_INIT:
        return "starting";
    case WIFI_CONNECT_STATE_AP_READY:
    case WIFI_CONNECT_STATE_WAIT_CONFIG:
        return "waiting_config";
    case WIFI_CONNECT_STATE_CONNECTING:
        return "connecting";
    case WIFI_CONNECT_STATE_CONNECTED:
        return "connected";
    default:
        return "failed";
    }
}

static esp_err_t wifi_connect_status_get_handler(httpd_req_t *req)
{
    char response[256];

    wifi_connect_set_close_header(req);
    httpd_resp_set_type(req, "application/json");
    snprintf(response, sizeof(response),
             "{\"state\":\"%s\",\"ssid\":\"%s\",\"ip\":\"%s\"}",
             wifi_connect_state_to_text(wifi_connect_state),
             wifi_connect_sta_ssid,
             wifi_connect_sta_ip);

    return httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t wifi_connect_result_get_handler(httpd_req_t *req)
{
    static const char html[] =
        "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>WiFi Status</title></head><body>"
        "<h2>WiFi Status</h2>"
        "<p id=\"state\">Connecting...</p>"
        "<p id=\"detail\"></p>"
        "<script>"
        "async function updateStatus(){"
        "const resp=await fetch('/status',{cache:'no-store'});"
        "const data=await resp.json();"
        "const state=document.getElementById('state');"
        "const detail=document.getElementById('detail');"
        "if(data.state==='connected'){"
        "state.textContent='Connected';"
        "detail.textContent='SSID: '+data.ssid+' IP: '+data.ip;"
        "return;"
        "}"
        "if(data.state==='failed'){"
        "state.textContent='Connect failed';"
        "detail.textContent='Please go back and retry.';"
        "return;"
        "}"
        "state.textContent='Connecting...';"
        "detail.textContent='Target SSID: '+(data.ssid || '');"
        "setTimeout(updateStatus,1000);"
        "}"
        "updateStatus();"
        "</script>"
        "</body></html>";

    wifi_connect_set_close_header(req);
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t wifi_connect_hotspot_detect_get_handler(httpd_req_t *req)
{
    return wifi_connect_redirect_to_root(req, "Redirect to captive portal");
}

static esp_err_t wifi_connect_generate_204_get_handler(httpd_req_t *req)
{
    return wifi_connect_redirect_to_root(req, "Redirect to captive portal");
}

static esp_err_t wifi_connect_connecttest_get_handler(httpd_req_t *req)
{
    return wifi_connect_redirect_to_root(req, "Redirect to captive portal");
}

static esp_err_t wifi_connect_ncsi_get_handler(httpd_req_t *req)
{
    return wifi_connect_redirect_to_root(req, "Redirect to captive portal");
}

static esp_err_t wifi_connect_http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    LV_UNUSED(err);
    return wifi_connect_redirect_to_root(req, "Redirect to captive portal");
}

static esp_err_t wifi_connect_configure_post_handler(httpd_req_t *req)
{
    char body[160] = {0};
    char ssid[33] = {0};
    char password[65] = {0};
    int received = 0;

    while (received < req->content_len) {
        int ret = httpd_req_recv(req, body + received, sizeof(body) - received - 1);

        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }

            return ESP_FAIL;
        }

        received += ret;
    }

    body[received] = '\0';
    httpd_query_key_value(body, "ssid", ssid, sizeof(ssid));
    httpd_query_key_value(body, "password", password, sizeof(password));
    wifi_connect_url_decode(ssid);
    wifi_connect_url_decode(password);

    if (ssid[0] == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "ssid required");
        return ESP_OK;
    }

    ESP_ERROR_CHECK(wifi_connect_apply_sta_config(ssid, password));
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/result");
    wifi_connect_set_close_header(req);
    return httpd_resp_send(req, NULL, 0);
}

static httpd_handle_t wifi_connect_start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = wifi_connect_index_get_handler,
        .user_ctx = NULL,
    };
    httpd_uri_t configure_uri = {
        .uri = "/configure",
        .method = HTTP_POST,
        .handler = wifi_connect_configure_post_handler,
        .user_ctx = NULL,
    };
    httpd_uri_t status_uri = {
        .uri = "/status",
        .method = HTTP_GET,
        .handler = wifi_connect_status_get_handler,
        .user_ctx = NULL,
    };
    httpd_uri_t result_uri = {
        .uri = "/result",
        .method = HTTP_GET,
        .handler = wifi_connect_result_get_handler,
        .user_ctx = NULL,
    };
    httpd_uri_t hotspot_detect_uri = {
        .uri = "/hotspot-detect.html",
        .method = HTTP_GET,
        .handler = wifi_connect_hotspot_detect_get_handler,
        .user_ctx = NULL,
    };
    httpd_uri_t generate_204_uri = {
        .uri = "/generate_204",
        .method = HTTP_GET,
        .handler = wifi_connect_generate_204_get_handler,
        .user_ctx = NULL,
    };
    httpd_uri_t connecttest_uri = {
        .uri = "/connecttest.txt",
        .method = HTTP_GET,
        .handler = wifi_connect_connecttest_get_handler,
        .user_ctx = NULL,
    };
    httpd_uri_t ncsi_uri = {
        .uri = "/ncsi.txt",
        .method = HTTP_GET,
        .handler = wifi_connect_ncsi_get_handler,
        .user_ctx = NULL,
    };
    httpd_handle_t server = NULL;

    config.max_open_sockets = WIFI_CONNECT_HTTP_MAX_SOCKETS;
    config.max_uri_handlers = 8;
    config.backlog_conn = WIFI_CONNECT_HTTP_BACKLOG;
    config.lru_purge_enable = true;
    config.recv_wait_timeout = WIFI_CONNECT_HTTP_RECV_TIMEOUT_SEC;
    config.send_wait_timeout = WIFI_CONNECT_HTTP_SEND_TIMEOUT_SEC;

    ESP_ERROR_CHECK(httpd_start(&server, &config));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &index_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &configure_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &status_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &result_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &hotspot_detect_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &generate_204_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &connecttest_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &ncsi_uri));
    ESP_ERROR_CHECK(httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, wifi_connect_http_404_error_handler));

    return server;
}

static void wifi_connect_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    LV_UNUSED(arg);

    if ((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_AP_START)) {
        wifi_connect_set_state(WIFI_CONNECT_STATE_AP_READY);
        return;
    }

    if ((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_DISCONNECTED)) {
        wifi_connect_set_state(WIFI_CONNECT_STATE_FAILED);
        return;
    }

    if ((event_base == IP_EVENT) && (event_id == IP_EVENT_STA_GOT_IP)) {
        ip_event_got_ip_t *got_ip = (ip_event_got_ip_t *)event_data;

        snprintf(wifi_connect_sta_ip, sizeof(wifi_connect_sta_ip), IPSTR, IP2STR(&got_ip->ip_info.ip));
        wifi_connect_set_state(WIFI_CONNECT_STATE_CONNECTED);
        return;
    }
}

static void wifi_connect_init_stack(void)
{
    esp_err_t err = ESP_OK;

    err = nvs_flash_init();
    if ((err == ESP_ERR_NVS_NO_FREE_PAGES) || (err == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    err = esp_netif_init();
    if (err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }

    err = esp_event_loop_create_default();
    if (err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }
}

static void wifi_connect_make_ap_ssid(char *ssid, size_t size)
{
    uint8_t mac[6] = {0};

    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP));
    snprintf(ssid, size, "ESP32C5_%02X%02X%02X", mac[3], mac[4], mac[5]);
}

static void wifi_connect_set_captive_portal_uri(void)
{
    esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    esp_netif_ip_info_t ip_info;

    ESP_ERROR_CHECK(ap_netif ? ESP_OK : ESP_FAIL);
    ESP_ERROR_CHECK(esp_netif_get_ip_info(ap_netif, &ip_info));
    snprintf(wifi_connect_captive_uri, sizeof(wifi_connect_captive_uri),
             "http://" IPSTR, IP2STR(&ip_info.ip));

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(ap_netif));
    ESP_ERROR_CHECK(esp_netif_dhcps_option(ap_netif, ESP_NETIF_OP_SET, ESP_NETIF_CAPTIVEPORTAL_URI,
                                           wifi_connect_captive_uri, strlen(wifi_connect_captive_uri)));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(ap_netif));
}

static void wifi_connect_start_network(void)
{
    dns_server_config_t dns_config = DNS_SERVER_CONFIG_SINGLE("*", "WIFI_AP_DEF");
    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t ap_config = {0};

    wifi_connect_make_ap_ssid(wifi_connect_ap_ssid, sizeof(wifi_connect_ap_ssid));
    wifi_connect_sta_ssid[0] = '\0';
    wifi_connect_sta_ip[0] = '\0';

    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_wifi_init(&init_config));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_connect_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_connect_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    strlcpy((char *)ap_config.ap.ssid, wifi_connect_ap_ssid, sizeof(ap_config.ap.ssid));
    strlcpy((char *)ap_config.ap.password, WIFI_CONNECT_AP_PASSWORD, sizeof(ap_config.ap.password));
    ap_config.ap.ssid_len = strlen(wifi_connect_ap_ssid);
    ap_config.ap.channel = WIFI_CONNECT_AP_CHANNEL;
    ap_config.ap.max_connection = WIFI_CONNECT_AP_MAX_CONN;
    ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    ap_config.ap.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_connect_set_captive_portal_uri();
    wifi_connect_httpd = wifi_connect_start_http_server();
    wifi_connect_dns_server = start_dns_server(&dns_config);
    ESP_ERROR_CHECK(wifi_connect_dns_server ? ESP_OK : ESP_FAIL);
    wifi_connect_set_state(WIFI_CONNECT_STATE_WAIT_CONFIG);
}

static void wifi_connect_refresh_status(lv_timer_t *timer)
{
    char text[192];
    int32_t status_bar_height = WIFI_CONNECT_STATUS_BAR_HEIGHT_MARQUEE;
    bool use_marquee = false;

    LV_UNUSED(timer);

    switch (wifi_connect_state) {
    case WIFI_CONNECT_STATE_INIT:
        snprintf(text, sizeof(text), "Starting AP...");
        break;
    case WIFI_CONNECT_STATE_AP_READY:
    case WIFI_CONNECT_STATE_WAIT_CONFIG:
        snprintf(text, sizeof(text),
                 "1.Scan QR to join AP   2.Portal opens automatically   3.If needed open %s   AP:%s   PWD:%s",
                 WIFI_CONNECT_AP_URL, wifi_connect_ap_ssid, WIFI_CONNECT_AP_PASSWORD);
        use_marquee = true;
        break;
    case WIFI_CONNECT_STATE_CONNECTING:
        snprintf(text, sizeof(text), "Connecting to %s ...", wifi_connect_sta_ssid);
        break;
    case WIFI_CONNECT_STATE_CONNECTED:
        snprintf(text, sizeof(text), "Connected\nSSID: %s\nIP: %s", wifi_connect_sta_ssid, wifi_connect_sta_ip);
        status_bar_height = WIFI_CONNECT_STATUS_BAR_HEIGHT_DETAIL;
        break;
    default:
        snprintf(text, sizeof(text), "Connect failed\nRe-open %s and retry", WIFI_CONNECT_AP_URL);
        status_bar_height = WIFI_CONNECT_STATUS_BAR_HEIGHT_DETAIL;
        break;
    }

    if ((strcmp(wifi_connect_last_status_text, text) == 0) && (wifi_connect_last_use_marquee == use_marquee)) {
        return;
    }

    strlcpy(wifi_connect_last_status_text, text, sizeof(wifi_connect_last_status_text));
    wifi_connect_last_use_marquee = use_marquee;
    lv_obj_set_height(wifi_connect_status_bar, status_bar_height);
    lv_label_set_long_mode(wifi_connect_status_label, use_marquee ? LV_LABEL_LONG_CLIP : LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(wifi_connect_status_label, use_marquee ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_CENTER,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(wifi_connect_status_label, text);
    lv_obj_set_width(wifi_connect_status_label, use_marquee ? LV_SIZE_CONTENT : lv_obj_get_content_width(wifi_connect_status_bar));

    if (use_marquee) {
        wifi_connect_start_status_animation();
        return;
    }

    wifi_connect_stop_status_animation();
}

static void wifi_connect_create_screen(void)
{
    char qr_payload[128];
    int32_t screen_height = 0;
    int32_t bottom_margin = 0;
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_t *title = lv_label_create(screen);
    lv_obj_t *qr = NULL;

    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_label_set_text(title, "WiFi Connect");
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(title, 12);
    lv_obj_set_style_text_color(title, lv_color_hex(0x101C52), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);

    qr = lv_qrcode_create(screen);
    lv_qrcode_set_size(qr, 140);
    lv_qrcode_set_dark_color(qr, lv_color_hex(0x000000));
    lv_qrcode_set_light_color(qr, lv_color_hex(0xFFFFFF));
    lv_obj_set_align(qr, LV_ALIGN_TOP_MID);
    lv_obj_set_y(qr, 52);

    snprintf(qr_payload, sizeof(qr_payload), WIFI_CONNECT_QR_PAYLOAD_FORMAT,
             wifi_connect_ap_ssid, WIFI_CONNECT_AP_PASSWORD);
    lv_qrcode_update(qr, qr_payload, strlen(qr_payload));

    screen_height = lv_obj_get_height(screen);
    bottom_margin = (screen_height * WIFI_CONNECT_STATUS_BAR_BOTTOM_PERCENT) / 100;
    wifi_connect_status_bar = lv_obj_create(screen);
    lv_obj_set_size(wifi_connect_status_bar, WIFI_CONNECT_STATUS_BAR_WIDTH, WIFI_CONNECT_STATUS_BAR_HEIGHT_MARQUEE);
    lv_obj_set_align(wifi_connect_status_bar, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(wifi_connect_status_bar, -bottom_margin);
    lv_obj_remove_flag(wifi_connect_status_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(wifi_connect_status_bar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(wifi_connect_status_bar, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(wifi_connect_status_bar, lv_color_hex(0xEEF2FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(wifi_connect_status_bar, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(wifi_connect_status_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(wifi_connect_status_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(wifi_connect_status_bar, true, LV_PART_MAIN | LV_STATE_DEFAULT);

    wifi_connect_status_label = lv_label_create(wifi_connect_status_bar);
    lv_obj_set_width(wifi_connect_status_label, lv_obj_get_content_width(wifi_connect_status_bar));
    lv_obj_set_align(wifi_connect_status_label, LV_ALIGN_LEFT_MID);
    lv_obj_set_style_text_color(wifi_connect_status_label, lv_color_hex(0x293062), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(wifi_connect_status_label, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(wifi_connect_status_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(wifi_connect_status_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    wifi_connect_last_status_text[0] = '\0';
    wifi_connect_last_use_marquee = false;

    lv_scr_load(screen);
    lv_timer_create(wifi_connect_refresh_status, WIFI_CONNECT_UI_REFRESH_MS, NULL);
    wifi_connect_refresh_status(NULL);
}

void wifi_connect_test_run(void)
{
    lv_display_t *display = NULL;

    esp_log_level_set("httpd_uri", ESP_LOG_ERROR);
    esp_log_level_set("httpd_txrx", ESP_LOG_ERROR);
    esp_log_level_set("httpd_parse", ESP_LOG_ERROR);
    wifi_connect_set_state(WIFI_CONNECT_STATE_INIT);
    wifi_connect_init_stack();
    wifi_connect_start_network();
    ESP_ERROR_CHECK(bsp_display_set_partial_mode(true, 40));

    display = bsp_display_start();
    ESP_ERROR_CHECK(display ? ESP_OK : ESP_FAIL);
    ESP_ERROR_CHECK(bsp_display_brightness_set(100));
    ESP_ERROR_CHECK(bsp_display_lock(0));
    wifi_connect_create_screen();
    bsp_display_unlock();

    ESP_LOGI(TAG, "wifi connect app started");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(WIFI_CONNECT_IDLE_MS));
    }
}
