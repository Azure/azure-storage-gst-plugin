#ifndef AZURE_ELEMENTS_GST_AZURE_SRC_CONFIG_H_
#define AZURE_ELEMENTS_GST_AZURE_SRC_CONFIG_H_

#include <gst/gst.h>
G_BEGIN_DECLS

#define AZURE_SRC_DEFAULT_WORKER_COUNT 4

typedef struct {
    gchar *account_name;
    gchar *account_key;
    gchar *container_name;
    gchar *blob_name;
    gchar *blob_endpoint;
    gboolean use_https;
    guint worker_count;
} GstAzureSrcConfig;

#define AZURE_SRC_DEFAULT_CONFIG (GstAzureSrcConfig) { \
    .account_name = NULL, \
    .account_key = NULL, \
    .container_name = NULL, \
    .blob_name = NULL, \
    .blob_endpoint = NULL, \
    .use_https = TRUE, \
    .worker_count = AZURE_SRC_DEFAULT_WORKER_COUNT, \
}

void gst_azure_src_release_config(GstAzureSrcConfig *config);

G_END_DECLS

#endif