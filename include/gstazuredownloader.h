#ifndef _AZURE_ELEMENTS_GST_AZURE_DOWNLOADER_H_
#define _AZURE_ELEMENTS_GST_AZURE_DOWNLOADER_H_

#include <gst/gst.h>
#include "gstazuresinkconfig.h"

G_BEGIN_DECLS

typedef struct _GstAzureDownloader GstAzureDownloader;

// azure downloader class, containing function pointers for member functions
typedef struct {
  gboolean (*init) (GstAzureDownloader *, const gchar *, const gchar *);
  gsize (*read) (GstAzureDownloader *, gchar *, gsize);
  gboolean (*seek) (GstAzureDownloader*, goffset);
  gsize (*get_size) (GstAzureDownloader*);
  gboolean (*destroy) (GstAzureDownloader *);
} GstAzureDownloaderClass;

struct _GstAzureDownloader {
  const GstAzureDownloaderClass* klass;
  void* impl;
  void* data;
};

// and their default implementations
static inline gboolean gst_azure_downloader_init(GstAzureDownloader* downloader, const gchar* container_name, const gchar* blob_name)
{
  return downloader->klass->init(downloader, container_name, blob_name);
}

static inline gsize gst_azure_downloader_read(GstAzureDownloader* downloader, gchar* data, const gsize size)
{
  return downloader->klass->read(downloader, data, size);
}

static inline gboolean gst_azure_downloader_seek(GstAzureDownloader* downloader, goffset offset)
{
  return downloader->klass->seek(downloader, offset);
}

static inline gsize gst_azure_downloader_get_size(GstAzureDownloader* downloader)
{
  return downloader->klass->get_size(downloader);
}

static inline gboolean gst_azure_downloader_destroy(GstAzureDownloader* downloader)
{
  return downloader->klass->destroy(downloader);
}

G_END_DECLS
#endif