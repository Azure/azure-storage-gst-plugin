#ifndef AZURE_ELEMENTS_GST_AZURE_SINK_CONFIG_H_
#define AZURE_ELEMENTS_GST_AZURE_SINK_CONFIG_H_

#include <gst/gst.h>
G_BEGIN_DECLS

#define AZURE_SINK_DEFAULT_BLOCK_SIZE (4*1024*1024)
#define AZURE_SINK_DEFAULT_WORKER_COUNT 4
#define AZURE_SINK_DEFAULT_COMMIT_BLOCK_COUNT 16
#define AZURE_SINK_DEFAULT_COMMIT_INTERVAL_MS 60000

#define AZURE_SINK_BLOB_TYPE_BLOCK "block"
#define AZURE_SINK_BLOB_TYPE_APPEND "append"

typedef struct {
    gchar *account_name;
    gchar *account_key;
    gchar *container_name;
    gchar *blob_name;
    gchar *blob_endpoint;
    gboolean use_https;
    gchar *blob_type;
    guint block_size;
    guint worker_count;
    guint commit_block_count;
    guint commit_interval_ms;
} GstAzureSinkConfig;

#define AZURE_SINK_DEFAULT_CONFIG ((GstAzureSinkConfig) {\
  .account_name = NULL,\
  .account_key = NULL,\
  .container_name = NULL,\
  .blob_name = NULL,\
  .blob_endpoint = NULL,\
  .use_https = TRUE,\
  .blob_type = NULL,\
  .block_size = AZURE_SINK_DEFAULT_BLOCK_SIZE,\
  .worker_count = AZURE_SINK_DEFAULT_WORKER_COUNT,\
  .commit_block_count = AZURE_SINK_DEFAULT_COMMIT_BLOCK_COUNT,\
  .commit_interval_ms = AZURE_SINK_DEFAULT_COMMIT_INTERVAL_MS\
})

static inline void gst_azure_sink_release_config(GstAzureSinkConfig *config)
{
  g_free(config->account_name);
  g_free(config->account_key);
  g_free(config->container_name);
  g_free(config->blob_endpoint);
  g_free(config->blob_name);

  *config = AZURE_SINK_DEFAULT_CONFIG;
}

G_END_DECLS

#endif