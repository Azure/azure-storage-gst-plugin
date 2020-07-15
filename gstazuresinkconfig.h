#ifndef AZURE_SINK_CONFIG_H_
#define AZURE_SINK_CONFIG_H_

#include <gst/gst.h>

#define AZURE_SINK_DEFAULT_BUFFER_SIZE (5*1024*1024)
#define AZURE_SINK_DEFAULT_BUFFER_COUNT 4

typedef struct {
    gchar *account_name;
    gchar *account_key;
    gchar *container_name;
    gchar *blob_name;
    gchar *blob_endpoint;

    gsize buffer_size;
    gsize buffer_count;
} GstAzureSinkConfig;

#define AZURE_SINK_DEFAULT_CONFIG (GstAzureSinkConfig) { \
    .account_name = NULL, \
    .account_key = NULL, \
    .container_name = NULL, \
    .blob_name = NULL, \
    .blob_endpoint = NULL, \
    .buffer_size = AZURE_SINK_DEFAULT_BUFFER_SIZE, \
    .buffer_count = AZURE_SINK_DEFAULT_BUFFER_COUNT, \
}

void gst_azure_sink_release_config(GstAzureSinkConfig *config) {
    g_free(config->account_name);
    g_free(config->account_key);
    g_free(config->container_name);
    g_free(config->blob_endpoint);
    g_free(config->blob_name);
    
    *config = AZURE_SINK_DEFAULT_CONFIG;
}

#endif