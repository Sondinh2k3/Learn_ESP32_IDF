
/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_eth.h"
// #include "esp_tls_crypto.h"
#include <esp_http_server.h>

#include "http_server_app.h"

/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 */

static const char *TAG = "HTTP_SERVER";
static httpd_handle_t server = NULL;
static httpd_req_t *REG;

// Cau lenh nhung file HTML vao esp32
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

static http_post_callback_t http_post_switch_callback = NULL;
static http_post_callback_t http_post_slider_callback = NULL;
static http_get_callback_t http_get_dht11_callback = NULL;

#if CONFIG_EXAMPLE_BASIC_AUTH

typedef struct
{
    char *username;
    char *password;
} basic_auth_info_t;

#define HTTPD_401 "401 UNAUTHORIZED" /*!< HTTP Response 401 */

static char *http_auth_basic(const char *username, const char *password)
{
    int out;
    char *user_info = NULL;
    char *digest = NULL;
    size_t n = 0;
    asprintf(&user_info, "%s:%s", username, password);
    if (!user_info)
    {
        ESP_LOGE(TAG, "No enough memory for user information");
        return NULL;
    }
    esp_crypto_base64_encode(NULL, 0, &n, (const unsigned char *)user_info, strlen(user_info));

    /* 6: The length of the "Basic " string
     * n: Number of bytes for a base64 encode format
     * 1: Number of bytes for a reserved which be used to fill zero
     */
    digest = calloc(1, 6 + n + 1);
    if (digest)
    {
        strcpy(digest, "Basic ");
        esp_crypto_base64_encode((unsigned char *)digest + 6, n, (size_t *)&out, (const unsigned char *)user_info, strlen(user_info));
    }
    free(user_info);
    return digest;
}

/* An HTTP GET handler */
static esp_err_t basic_auth_get_handler(httpd_req_t *req)
{
    char *buf = NULL;
    size_t buf_len = 0;
    basic_auth_info_t *basic_auth_info = req->user_ctx;

    buf_len = httpd_req_get_hdr_value_len(req, "Authorization") + 1;
    if (buf_len > 1)
    {
        buf = calloc(1, buf_len);
        if (!buf)
        {
            ESP_LOGE(TAG, "No enough memory for basic authorization");
            return ESP_ERR_NO_MEM;
        }

        if (httpd_req_get_hdr_value_str(req, "Authorization", buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found header => Authorization: %s", buf);
        }
        else
        {
            ESP_LOGE(TAG, "No auth value received");
        }

        char *auth_credentials = http_auth_basic(basic_auth_info->username, basic_auth_info->password);
        if (!auth_credentials)
        {
            ESP_LOGE(TAG, "No enough memory for basic authorization credentials");
            free(buf);
            return ESP_ERR_NO_MEM;
        }

        if (strncmp(auth_credentials, buf, buf_len))
        {
            ESP_LOGE(TAG, "Not authenticated");
            httpd_resp_set_status(req, HTTPD_401);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Connection", "keep-alive");
            httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");
            httpd_resp_send(req, NULL, 0);
        }
        else
        {
            ESP_LOGI(TAG, "Authenticated!");
            char *basic_auth_resp = NULL;
            httpd_resp_set_status(req, HTTPD_200);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Connection", "keep-alive");
            asprintf(&basic_auth_resp, "{\"authenticated\": true,\"user\": \"%s\"}", basic_auth_info->username);
            if (!basic_auth_resp)
            {
                ESP_LOGE(TAG, "No enough memory for basic authorization response");
                free(auth_credentials);
                free(buf);
                return ESP_ERR_NO_MEM;
            }
            httpd_resp_send(req, basic_auth_resp, strlen(basic_auth_resp));
            free(basic_auth_resp);
        }
        free(auth_credentials);
        free(buf);
    }
    else
    {
        ESP_LOGE(TAG, "No auth header received");
        httpd_resp_set_status(req, HTTPD_401);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Connection", "keep-alive");
        httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");
        httpd_resp_send(req, NULL, 0);
    }

    return ESP_OK;
}

static httpd_uri_t basic_auth = {
    .uri = "/basic_auth",
    .method = HTTP_GET,
    .handler = basic_auth_get_handler,
};

static void httpd_register_basic_auth(httpd_handle_t server)
{
    basic_auth_info_t *basic_auth_info = calloc(1, sizeof(basic_auth_info_t));
    if (basic_auth_info)
    {
        basic_auth_info->username = CONFIG_EXAMPLE_BASIC_AUTH_USERNAME;
        basic_auth_info->password = CONFIG_EXAMPLE_BASIC_AUTH_PASSWORD;

        basic_auth.user_ctx = basic_auth_info;
        httpd_register_uri_handler(server, &basic_auth);
    }
}
#endif

/* An HTTP GET handler */
static esp_err_t hello_get_handler(httpd_req_t *req)
{
    // const char *resp_str = (const char *)"Xin chao ban";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);

    return ESP_OK;
}

static const httpd_uri_t get_DHT11 = {
    .uri = "/dht11",
    .method = HTTP_GET,
    .handler = hello_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = NULL};

void dht11_response(char *data, int len)
{
    httpd_resp_send(REG, data, len);
}

/* An HTTP GET handler */
static esp_err_t get_handler_dht11(httpd_req_t *req)
{
    // const char *resp_str = (const char *)"{\"temperature\": \"25.4\", \"humidity\": \"65\"}";
    // httpd_resp_send(req, resp_str, strlen(resp_str));

    REG = req;
    http_get_dht11_callback();

    return ESP_OK;
}

static const httpd_uri_t get_data_DHT11 = {
    .uri = "/getdatadht11",
    .method = HTTP_GET,
    .handler = get_handler_dht11,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = NULL};

/* An HTTP POST handler */
static esp_err_t data_post_handler(httpd_req_t *req)
{
    char buf[100];
    int remaining = req->content_len;

    /* Read the data for the request */
    httpd_req_recv(req, buf, remaining);
    printf("DATA: %s\n", buf);

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t post_data = {
    .uri = "/data",
    .method = HTTP_POST,
    .handler = data_post_handler,
    .user_ctx = NULL};

/* An HTTP POST handler switch1 */
static esp_err_t sw1_post_handler(httpd_req_t *req)
{
    char buf[100];
    int remaining = req->content_len;

    /* Read the data for the request */
    httpd_req_recv(req, buf, remaining);
    http_post_switch_callback(buf, remaining);

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t sw1_post_data = {
    .uri = "/switch1",
    .method = HTTP_POST,
    .handler = sw1_post_handler,
    .user_ctx = NULL};

/* An HTTP POST handler slider */
static esp_err_t slider_post_handler(httpd_req_t *req)
{
    char buf[100];
    int remaining = req->content_len;

    /* Read the data for the request */
    httpd_req_recv(req, buf, remaining);
    http_post_slider_callback(buf, remaining);

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t slider_post_data = {
    .uri = "/slider",
    .method = HTTP_POST,
    .handler = slider_post_handler,
    .user_ctx = NULL};

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/dht11", req->uri) == 0)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    }

    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

void start_webserver(void)
{

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &get_DHT11);
        httpd_register_uri_handler(server, &sw1_post_data);
        httpd_register_uri_handler(server, &slider_post_data);
        httpd_register_uri_handler(server, &get_data_DHT11);
        httpd_register_uri_handler(server, &post_data);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    else
    {
        ESP_LOGI(TAG, "Error starting server!");
    }
}

void stop_webserver(void)
{
    // Stop the httpd server
    httpd_stop(server);
}

void http_set_callback_switch(void *cb)
{
    http_post_switch_callback = cb;
}

void http_set_callback_slider(void *cb)
{
    http_post_slider_callback = cb;
}

void http_set_callback_dht11(void *cb)
{
    http_get_dht11_callback = cb;
}
