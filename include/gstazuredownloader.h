#ifndef _AZURE_ELEMENTS_GST_AZURE_DOWNLOADER_H_
#define _AZURE_ELEMENTS_GST_AZURE_DOWNLOADER_H_

#include <gst/gst.h>
#include "gstazuresinkconfig.h"

G_BEGIN_DECLS

typedef struct _GstAzureDownloader GstAzureDownloader;

// azure downloader class, containing function pointers for member functions
typedef struct {
  gboolean (*init) (GstAzureDownloader *, gchar *, const gchar *);
  gboolean (*read) (GstAzureDownloader *, gchar *, gsize);
  gboolean (*destroy) (GstAzureDownloader *);
} GstAzureDownloaderClass;

// and their default implementations
gboolean gst_azure_downloader_init(GstAzureDownloader *downloader, const gchar *container_name, const gchar *blob_name);
gboolean gst_azure_downloader_read(GstAzureDownloader *downloader, gchar *data, const gsize size);
gboolean gst_azure_downloader_destroy(GstAzureDownloader *downloader);

struct _GstAzureDownloader {
  GstAzureDownloaderClass *klass;
  void *impl;
  void *data;
};

G_END_DECLS
#endif