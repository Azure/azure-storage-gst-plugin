#ifndef AZURE_ELEMENTS_GST_AZURE_SRC_CONFIG_H_
#define AZURE_ELEMENTS_GST_AZURE_SRC_CONFIG_H_

#include <gst/gst.h>
G_BEGIN_DECLS

#define AZURE_SRC_DEFAULT_WORKER_COUNT 4
#define AZURE_SRC_DEFAULT_BLOCK_SIZE (1 * 1024 * 1024)
#define AZURE_SRC_DEFAULT_PREFETCH_BLOCK_COUNT 4

typedef struct {
    gchar *account_name;
    gchar *account_key;
    gchar *container_name;
    gchar *blob_name;
    gchar *blob_endpoint;
    gboolean use_https;
    guint worker_count;
    gsize block_size;
    guint prefetch_block_count;
} GstAzureSrcConfig;

// read_ahead_size have -1 as default value, which means 1 * block_size
#define AZURE_SRC_DEFAULT_CONFIG ((GstAzureSrcConfig) {\
  .account_name = NULL,\
  .account_key = NULL,\
  .container_name = NULL,\
  .blob_name = NULL,\
  .blob_endpoint = NULL,\
  .use_https = TRUE,\
  .worker_count = AZURE_SRC_DEFAULT_WORKER_COUNT,\
  .block_size = AZURE_SRC_DEFAULT_BLOCK_SIZE,\
  .prefetch_block_count = AZURE_SRC_DEFAULT_PREFETCH_BLOCK_COUNT\
})

static inline void gst_azure_src_release_config(GstAzureSrcConfig* config)
{
  g_free(config->account_name);
  g_free(config->account_key);
  g_free(config->container_name);
  g_free(config->blob_name);
  *config = AZURE_SRC_DEFAULT_CONFIG;
}


G_END_DECLS

#endif