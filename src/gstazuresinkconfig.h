#ifndef AZURE_SINK_GST_AZURE_SINK_CONFIG_H_
#define AZURE_SINK_GST_AZURE_SINK_CONFIG_H_

#include <gst/gst.h>
G_BEGIN_DECLS

#define AZURE_SINK_DEFAULT_BUFFER_SIZE (5*1024*1024)
#define AZURE_SINK_DEFAULT_BUFFER_COUNT 4

typedef struct {
    gchar *account_name;
    gchar *account_key;
    gchar *container_name;
    gchar *blob_name;
    gchar *blob_endpoint;
    gboolean use_https;

    // TODO following two are now useless :)
    gsize buffer_size;
    gsize buffer_count;
} GstAzureSinkConfig;

#define AZURE_SINK_DEFAULT_CONFIG (GstAzureSinkConfig) { \
    .account_name = NULL, \
    .account_key = NULL, \
    .container_name = NULL, \
    .blob_name = NULL, \
    .blob_endpoint = NULL, \
    .use_https = TRUE, \
    .buffer_size = AZURE_SINK_DEFAULT_BUFFER_SIZE, \
    .buffer_count = AZURE_SINK_DEFAULT_BUFFER_COUNT, \
}

void gst_azure_sink_release_config(GstAzureSinkConfig *config);

G_END_DECLS

#endif