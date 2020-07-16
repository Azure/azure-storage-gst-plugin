#ifndef _AZURE_SINK_SIMPLE_UPLOADER_H_
#define _AZURE_SINK_SIMPLE_UPLOADER_H_
#include <gst/gst.h>
#include "gstazureuploader.h"

G_BEGIN_DECLS

struct GstAzureUploaderClass *getSimpleUploaderClass();
// default simple uploader constructor
struct GstAzureUploader *gst_azure_sink_uploader_new(const GstAzureSinkConfig *config);

G_END_DECLS

#endif