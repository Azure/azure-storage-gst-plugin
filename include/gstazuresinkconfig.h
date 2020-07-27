#ifndef AZURE_ELEMENTS_GST_AZURE_SINK_CONFIG_H_
#define AZURE_ELEMENTS_GST_AZURE_SINK_CONFIG_H_

#include <gst/gst.h>
G_BEGIN_DECLS

#define AZURE_SINK_DEFAULT_BLOCK_SIZE (4*1024*1024)
#define AZURE_SINK_DEFAULT_WORKER_COUNT 4

#define AZURE_SINK_DEFAULT_COMMIT_BLOCK_COUNT 16
#define AZURE_SINK_DEFAULT_COMMIT_INTERVAL_MS 60000

typedef struct {
    gchar *account_name;
    gchar *account_key;
    gchar *container_name;
    gchar *blob_name;
    gchar *blob_endpoint;
    gboolean use_https;
    guint block_size;
    guint worker_count;
    guint commit_block_count;
    guint commit_interval_ms;
} GstAzureSinkConfig;

#define AZURE_SINK_DEFAULT_CONFIG (GstAzureSinkConfig) { \
    .account_name = NULL, \
    .account_key = NULL, \
    .container_name = NULL, \
    .blob_name = NULL, \
    .blob_endpoint = NULL, \
    .use_https = TRUE, \
    .block_size = AZURE_SINK_DEFAULT_BLOCK_SIZE, \
    .worker_count = AZURE_SINK_DEFAULT_WORKER_COUNT, \
    .commit_block_count = AZURE_SINK_DEFAULT_COMMIT_BLOCK_COUNT, \
    .commit_interval_ms = AZURE_SINK_DEFAULT_COMMIT_INTERVAL_MS \
}

void gst_azure_sink_release_config(GstAzureSinkConfig *config);

G_END_DECLS

#endif