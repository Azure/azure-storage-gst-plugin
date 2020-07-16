// azure uploader
// This is a gstreamer-style wrapper of the C++ azure append blob uploader.
// Now it only contains the wrapper for single-threaded azure uploader(in azureuploader.cpp)

#include "gstazureuploader.h"

G_BEGIN_DECLS

#define GET_CLASS(obj) (((GstAzureUploader *)(obj))->klass)

// NOTE we need an extra layer if multiple implementations of GstAzureUploader is needed.
// wrapppers for uploader functions

gboolean gst_azure_uploader_destroy(GstAzureUploader *uploader)
{
  return uploader->klass->destroy(uploader);
}

gboolean gst_azure_uploader_upload(GstAzureUploader *uploader, const gchar *data, const gsize size)
{
  return uploader->klass->upload(uploader, data, size);
}

gboolean gst_azure_uploader_flush(GstAzureUploader *uploader)
{
  return uploader->klass->flush(uploader);
}

gboolean gst_azure_uploader_init(GstAzureUploader *uploader, const gchar *container_name, const gchar *blob_name)
{
  return uploader->klass->init(uploader, container_name, blob_name);
}

G_END_DECLS