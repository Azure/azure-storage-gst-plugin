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

#define SIMPLE_UPLOADER(_u) ((gst::azure::storage::SimpleAzureUploader *)((_u)->impl))

gboolean gst_simple_azure_uploader_init(GstAzureUploader *uploader, const gchar *container_name, const gchar *blob_name)
{
  uploader->loc = SIMPLE_UPLOADER(uploader)->init(container_name, blob_name);
  return TRUE;
}

gboolean gst_simple_azure_uploader_flush(GstAzureUploader *uploader)
{
  g_return_val_if_fail(uploader->loc, FALSE);
  return SIMPLE_UPLOADER(uploader)->flush(uploader->loc);
}

gboolean gst_simple_azure_uploader_destroy(GstAzureUploader *uploader)
{
  g_return_val_if_fail(uploader->loc, FALSE);
  g_return_val_if_fail(SIMPLE_UPLOADER(uploader)->destroy(uploader->loc), FALSE);
  uploader->loc = nullptr;
  return TRUE;
}

gboolean gst_simple_azure_uploader_upload(GstAzureUploader *uploader, const gchar *data, const gsize size)
{
  g_return_val_if_fail(uploader->loc, FALSE);
  return SIMPLE_UPLOADER(uploader)->upload(uploader->loc, (const char *)data, (size_t)size);
}

// get default class for azureuploader.cpp(single-threaded version)
static GstAzureUploaderClass *getDefaultClass()
{
  GstAzureUploaderClass *ret = new GstAzureUploaderClass();
  ret->init = gst_simple_azure_uploader_init;
  ret->flush = gst_simple_azure_uploader_flush;
  ret->destroy = gst_simple_azure_uploader_destroy;
  ret->upload = gst_simple_azure_uploader_upload;
  return ret;
}

// FIXME implement this
GstAzureUploader *gst_azure_sink_uploader_new(const GstAzureSinkConfig *config) {
  static GstAzureUploaderClass *defaultClass = getDefaultClass();
  g_return_val_if_fail(config, NULL);
  GstAzureUploader *uploader = new GstAzureUploader();
  g_return_val_if_fail(uploader, NULL);
  uploader->klass = defaultClass;
  uploader->impl = (void *)(new gst::azure::storage::SimpleAzureUploader(
    config->account_name, config->account_key, (bool)config->use_https));
  return uploader;
}

G_END_DECLS