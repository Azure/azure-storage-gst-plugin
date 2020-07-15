// azure uploader
// this is a gstreamer-style wrapper of the C++ azure append blob uploader
#include "gstazureuploader.h"

G_BEGIN_DECLS

#define GET_CLASS(obj) (((GstAzureUploader *)(obj))->klass)

static GstAzureUploaderClass *getDefaultClass()
{
  GstAzureUploaderClass *ret = new GstAzureUploaderClass();
  ret->init = gst_azure_uploader_init;
  ret->flush = gst_azure_uploader_flush;
  ret->destroy = gst_azure_uploader_destroy;
  ret->upload = gst_azure_uploader_upload;
  return ret;
}

// FIXME implement this
GstAzureUploader *gst_azure_sink_uploader_new(const GstAzureSinkConfig *config) {
  static GstAzureUploaderClass *defaultClass = getDefaultClass();
  g_return_val_if_fail(config, NULL);
}

gboolean gst_azure_uploader_destroy(GstAzureUploader *uploader)
{
  if(uploader->loc == nullptr)
  {
    return FALSE;
  }
  return (gboolean)uploader->uploader->destroy(uploader->loc);
}

gboolean gst_azure_uploader_upload(GstAzureUploader *uploader, const gchar *data, const gsize size)
{
  if(uploader->loc == nullptr)
  {
    return FALSE;
  }
  return (gboolean)uploader->uploader->upload(uploader->loc, data, size);
}

gboolean gst_azure_uploader_flush(GstAzureUploader *uploader)
{
  if(uploader->loc == nullptr)
  {
    return FALSE;
  }
  return (gboolean)uploader->uploader->flush(uploader->loc);
}

gboolean gst_azure_uploader_init(GstAzureUploader *uploader, const gchar *container_name, const gchar *blob_name)
{
  uploader->loc = uploader->uploader->init(container_name, blob_name);
  return true;
}

G_END_DECLS