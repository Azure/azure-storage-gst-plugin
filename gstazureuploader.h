#ifndef _GST_AZURE_UPLOADER_H
#define _GST_AZURE_UPLOADER_H

#include <gst/gst.h>
#include "gstazuresinkconfig.h"
G_BEGIN_DECLS

typedef struct _GstAzureUploader GstAzureUploader;

typedef struct {
    void (*destroy) (GstAzureUploader *);
    gboolean (*upload) (GstAzureUploader *, const gchar *, gsize);
    gboolean (*flush) (GstAzureUploader *);
} GstAzureUploaderClass;

struct _GstAzureUploader {
    GstAzureUploaderClass *klass;
};

GstAzureUploader *gst_azure_sink_uploader_new(const GstAzureSinkConfig *config) {
    g_return_val_if_fail(config, NULL);
    
}

G_END_DECLS
#endif