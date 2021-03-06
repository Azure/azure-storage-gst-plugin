#ifndef _AZURE_ELEMENTS_SIMPLE_AZURE_UPLOADER_H_
#define _AZURE_ELEMENTS_SIMPLE_AZURE_UPLOADER_H_
#include <gst/gst.h>
#include "gstazureuploader.h"

G_BEGIN_DECLS

GstAzureUploaderClass *getSimpleUploaderClass();
// default simple uploader constructor
GstAzureUploader *gst_azure_sink_simple_uploader_new(const GstAzureSinkConfig *config);

G_END_DECLS

#endif