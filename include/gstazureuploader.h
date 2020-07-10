#ifndef _AZURE_ELEMENTS_GST_AZURE_UPLOADER_H
#define _AZURE_ELEMENTS_GST_AZURE_UPLOADER_H

#include <gst/gst.h>
#include "gstazuresinkconfig.h"

G_BEGIN_DECLS

typedef struct _GstAzureUploader GstAzureUploader;

// azure uploader class, containing function pointers for member functions
typedef struct {
  gboolean (*init) (GstAzureUploader *, const gchar *, const gchar *);
  gboolean (*upload) (GstAzureUploader *, const gchar *, gsize);
  gboolean (*flush) (GstAzureUploader *);
  gboolean (*destroy) (GstAzureUploader *);
} GstAzureUploaderClass;

struct _GstAzureUploader {
  const GstAzureUploaderClass* klass;
  // maybe a more elegant solution?
  void* impl;
  void* data;
};

// and their default implementations
static inline gboolean gst_azure_uploader_init(GstAzureUploader* uploader, const gchar* container_name, const gchar* blob_name)
{
  return uploader->klass->init(uploader, container_name, blob_name);
}

static inline gboolean gst_azure_uploader_upload(GstAzureUploader* uploader, const gchar* data, const gsize size)
{
  return uploader->klass->upload(uploader, data, size);
}

static inline gboolean gst_azure_uploader_flush(GstAzureUploader* uploader)
{
  return uploader->klass->flush(uploader);
}

static inline gboolean gst_azure_uploader_destroy(GstAzureUploader* uploader)
{
  return uploader->klass->destroy(uploader);
}

G_END_DECLS
#endif