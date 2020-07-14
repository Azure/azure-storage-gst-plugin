// azure uploader
// this is a gstreamer-style wrapper of the C++ azure append blob uploader
#include "gstazureuploader.h"

#define GET_CLASS(obj) (((GstAzureUploader *)(obj))->klass)

void gst_azure_uploader_destroy(GstAzureUploader *uploader) {
    return GET_CLASS()
}