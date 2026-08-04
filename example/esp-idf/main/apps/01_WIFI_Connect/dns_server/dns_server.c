#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <inttypes.h>
#include <errno.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "dns_server.h"

#define DNS_SERVER_PORT        (53)
#define DNS_SERVER_PACKET_MAX  (256)
#define DNS_SERVER_OPCODE_MASK (0x7800)
#define DNS_SERVER_QR_FLAG     (1 << 7)
#define DNS_SERVER_QD_TYPE_A   (0x0001)
#define DNS_SERVER_ANSWER_TTL  (300)

static const char *TAG = "dns_server";

typedef struct __attribute__((__packed__)) {
    uint16_t id;
    uint16_t flags;
    uint16_t qd_count;
    uint16_t an_count;
    uint16_t ns_count;
    uint16_t ar_count;
} dns_header_t;

typedef struct {
    uint16_t type;
    uint16_t class;
} dns_question_t;

typedef struct __attribute__((__packed__)) {
    uint16_t ptr_offset;
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t addr_len;
    uint32_t ip_addr;
} dns_answer_t;

struct dns_server_handle {
    bool started;
    TaskHandle_t task;
    int num_of_entries;
    dns_entry_pair_t entry[];
};

static char *dns_server_parse_name(char *raw_name, char *parsed_name, size_t parsed_name_size)
{
    char *label = raw_name;
    char *parsed = parsed_name;
    int parsed_len = 0;

    while (*label != 0) {
        int part_len = *label;

        parsed_len += (part_len + 1);
        if ((parsed_len <= 0) || ((size_t)parsed_len > parsed_name_size)) {
            return NULL;
        }

        memcpy(parsed, label + 1, part_len);
        parsed[part_len] = '.';
        parsed += (part_len + 1);
        label += (part_len + 1);
    }

    if (parsed_len == 0) {
        return NULL;
    }

    parsed_name[parsed_len - 1] = '\0';
    return label + 1;
}

static esp_ip4_addr_t dns_server_find_answer_ip(dns_server_handle_t handle, const char *name)
{
    esp_ip4_addr_t ip = { .addr = IPADDR_ANY };
    int index = 0;

    while (index < handle->num_of_entries) {
        const dns_entry_pair_t *entry = &handle->entry[index];

        if ((strcmp(entry->name, "*") == 0) || (strcmp(entry->name, name) == 0)) {
            if (entry->if_key != NULL) {
                esp_netif_t *netif = esp_netif_get_handle_from_ifkey(entry->if_key);

                if (netif != NULL) {
                    esp_netif_ip_info_t ip_info;

                    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
                        ip.addr = ip_info.ip.addr;
                    }
                }
                return ip;
            }

            if (entry->ip.addr != IPADDR_ANY) {
                ip.addr = entry->ip.addr;
                return ip;
            }
        }

        index++;
    }

    return ip;
}

static int dns_server_build_reply(char *request, size_t request_len, char *reply, size_t reply_size,
                                  dns_server_handle_t handle)
{
    char *question_ptr = NULL;
    char *answer_ptr = NULL;
    dns_header_t *header = NULL;
    uint16_t question_count = 0;
    int reply_len = 0;
    int index = 0;

    if (request_len > reply_size) {
        return -1;
    }

    memset(reply, 0, reply_size);
    memcpy(reply, request, request_len);

    header = (dns_header_t *)reply;
    if ((header->flags & DNS_SERVER_OPCODE_MASK) != 0) {
        return 0;
    }

    question_count = ntohs(header->qd_count);
    header->flags |= DNS_SERVER_QR_FLAG;
    header->an_count = htons(question_count);

    reply_len = (int)request_len + (int)(question_count * sizeof(dns_answer_t));
    if ((reply_len <= 0) || ((size_t)reply_len > reply_size)) {
        return -1;
    }

    question_ptr = reply + sizeof(dns_header_t);
    answer_ptr = reply + request_len;

    while (index < question_count) {
        char name[128];
        char *name_end = dns_server_parse_name(question_ptr, name, sizeof(name));
        dns_question_t *question = NULL;
        esp_ip4_addr_t ip = { .addr = IPADDR_ANY };

        if (name_end == NULL) {
            ESP_LOGE(TAG, "Failed to parse DNS question");
            return -1;
        }

        question = (dns_question_t *)name_end;
        if (ntohs(question->type) == DNS_SERVER_QD_TYPE_A) {
            ip = dns_server_find_answer_ip(handle, name);
        }

        if (ip.addr != IPADDR_ANY) {
            dns_answer_t *answer = (dns_answer_t *)answer_ptr;

            answer->ptr_offset = htons(0xC000 | (question_ptr - reply));
            answer->type = question->type;
            answer->class = question->class;
            answer->ttl = htonl(DNS_SERVER_ANSWER_TTL);
            answer->addr_len = htons(sizeof(ip.addr));
            answer->ip_addr = ip.addr;
            answer_ptr += sizeof(dns_answer_t);
        }

        question_ptr = (char *)(question + 1);
        index++;
    }

    return reply_len;
}

static void dns_server_task(void *argument)
{
    dns_server_handle_t handle = (dns_server_handle_t)argument;

    while (handle->started) {
        struct sockaddr_in listen_addr = {0};
        int socket_fd = -1;

        listen_addr.sin_family = AF_INET;
        listen_addr.sin_port = htons(DNS_SERVER_PORT);
        listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (socket_fd < 0) {
            ESP_LOGE(TAG, "Failed to create socket: errno=%d", errno);
            break;
        }

        if (bind(socket_fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
            ESP_LOGE(TAG, "Failed to bind socket: errno=%d", errno);
            shutdown(socket_fd, 0);
            close(socket_fd);
            break;
        }

        while (handle->started) {
            char request[128];
            char reply[DNS_SERVER_PACKET_MAX];
            struct sockaddr_in6 source_addr;
            socklen_t source_len = sizeof(source_addr);
            int recv_len = recvfrom(socket_fd, request, sizeof(request), 0, (struct sockaddr *)&source_addr, &source_len);

            if (recv_len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno=%d", errno);
                break;
            }

            {
                int reply_len = dns_server_build_reply(request, (size_t)recv_len, reply, sizeof(reply), handle);

                if (reply_len <= 0) {
                    continue;
                }

                if (sendto(socket_fd, reply, reply_len, 0, (struct sockaddr *)&source_addr, source_len) < 0) {
                    ESP_LOGE(TAG, "sendto failed: errno=%d", errno);
                    break;
                }
            }
        }

        shutdown(socket_fd, 0);
        close(socket_fd);
    }

    vTaskDelete(NULL);
}

dns_server_handle_t start_dns_server(dns_server_config_t *config)
{
    dns_server_handle_t handle = NULL;

    ESP_RETURN_ON_FALSE(config != NULL, NULL, TAG, "config is null");
    ESP_RETURN_ON_FALSE(config->num_of_entries > 0, NULL, TAG, "config has no entries");

    handle = calloc(1, sizeof(struct dns_server_handle) + (size_t)config->num_of_entries * sizeof(dns_entry_pair_t));
    ESP_RETURN_ON_FALSE(handle != NULL, NULL, TAG, "failed to allocate dns server handle");

    handle->started = true;
    handle->num_of_entries = config->num_of_entries;
    memcpy(handle->entry, config->item, (size_t)config->num_of_entries * sizeof(dns_entry_pair_t));

    if (xTaskCreate(dns_server_task, "dns_server", 4096, handle, 5, &handle->task) != pdPASS) {
        free(handle);
        ESP_LOGE(TAG, "failed to create dns server task");
        return NULL;
    }

    return handle;
}

void stop_dns_server(dns_server_handle_t handle)
{
    if (handle == NULL) {
        return;
    }

    handle->started = false;
    if (handle->task != NULL) {
        vTaskDelete(handle->task);
    }
    free(handle);
}
