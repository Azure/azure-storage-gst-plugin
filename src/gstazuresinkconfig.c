#include "gstazuresinkconfig.h"

void gst_azure_sink_release_config(GstAzureSinkConfig *config) {
    g_free(config->account_name);
    g_free(config->account_key);
    g_free(config->container_name);
    g_free(config->blob_endpoint);
    g_free(config->blob_name);
    
    *config = AZURE_SINK_DEFAULT_CONFIG;
}
