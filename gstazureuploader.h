#ifndef _GST_AZURE_UPLOADER_H
#define _GST_AZURE_UPLOADER_H

#include <gst/gst.h>
#include "gstazuresinkconfig.h"
#include "simpleazureuploader.h"

G_BEGIN_DECLS

typedef struct _GstAzureUploader GstAzureUploader;

// azure uploader class, containing function pointers for member functions
typedef struct {
  gboolean (*init) (GstAzureUploader *, const gchar *, const gchar *);
  gboolean (*upload) (GstAzureUploader *, const gchar *, gsize);
  gboolean (*flush) (GstAzureUploader *);
  gboolean (*destroy) (GstAzureUploader *);
} GstAzureUploaderClass;

// and their default implementations
gboolean gst_azure_uploader_destroy(GstAzureUploader *uploader);
gboolean gst_azure_uploader_upload(GstAzureUploader *uploader, const gchar *data, const gsize size);
gboolean gst_azure_uploader_flush(GstAzureUploader *uploader);
gboolean gst_azure_uploader_init(GstAzureUploader *uploader, const gchar *container_name, const gchar *blob_name);

struct _GstAzureUploader {
  GstAzureUploaderClass *klass;
  // maybe a more elegant solution?
  void *impl;
  void *data;
};

G_END_DECLS
#endif