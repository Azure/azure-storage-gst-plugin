#ifndef _AZURE_SINK_BLOCK_AZURE_UPLOADER_H_
#define _AZURE_SINK_BLOCK_AZURE_UPLOADER_H_
#include <gst/gst.h>
#include "gstazureuploader.h"

G_BEGIN_DECLS

GstAzureUploaderClass *getBlockUploaderClass();
// default simple uploader constructor
GstAzureUploader *gst_azure_sink_block_ploader_new(const GstAzureSinkConfig *config);

G_END_DECLS

#endif