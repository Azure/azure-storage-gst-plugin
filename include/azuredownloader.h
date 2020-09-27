#ifndef _AZURE_ELEMENTS_AZURE_DOWNLOADER_H_
#define _AZURE_ELEMENTS_AZURE_DOWNLOADER_H_
#include <gst/gst.h>
#include "gstazuredownloader.h"
#include "gstazuresrcconfig.h"

G_BEGIN_DECLS

GstAzureDownloaderClass* getDownloaderClass();
// default simple uploader constructor
GstAzureDownloader* gst_azure_src_downloader_new(const GstAzureSrcConfig* config);

G_END_DECLS

#endif