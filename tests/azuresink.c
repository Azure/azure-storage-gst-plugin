#include <gst/check/gstcheck.h>
#include <gst/gst.h>

#include "gstazuresink.h"
#include "gstazuresinkconfig.h"

// uploader stub
static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE("src",
  GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static gboolean test_uploader_init(GstAzureUploader* uploader, const gchar* container_name, const gchar* blob_name)
{
  return strlen(container_name) > 0 && strlen(blob_name) > 0;
}

static gboolean test_uploader_flush(GstAzureUploader* uploader)
{
  return TRUE;
}

static gboolean test_uploader_destroy(GstAzureUploader* uploader)
{
  return TRUE;
}

static gboolean test_uploader_upload(GstAzureUploader* uploader, const gchar* data, gsize size)
{
  return TRUE;
}

static GstAzureUploaderClass test_uploader_klass = {
  .init = test_uploader_init,
  .flush = test_uploader_flush,
  .destroy = test_uploader_destroy,
  .upload = test_uploader_upload
};

static GstAzureUploader *test_uploader()
{
  GstAzureUploader* uploader = g_new(GstAzureUploader, 1);
  fail_on(uploader == NULL);
  uploader->klass = &test_uploader_klass;
  uploader->impl = NULL;
  uploader->data = NULL;
  return uploader;
}

GST_START_TEST(test_no_credential_should_fail)
{
  GstElement* sink = gst_element_factory_make("azuresink", "sink");
  GstStateChangeReturn ret;
  fail_if(sink == NULL);

  ret = gst_element_set_state(sink, GST_STATE_PLAYING);
  fail_unless(ret == GST_STATE_CHANGE_FAILURE);

  gst_element_set_state(sink, GST_STATE_NULL);
  gst_object_unref(sink);
}
GST_END_TEST

GST_START_TEST(test_no_location_should_fail)
{
  GstElement* sink = gst_element_factory_make("azuresink", "sink");
  GstStateChangeReturn ret;
  fail_if(sink == NULL);
  
  g_object_set(sink,
    "account-name", "gstvideostore",
    "account-key", "vIjLNYmW60yVH1nLc08u/KGJNXZl6Gzy7zdGtpSK3JJ6vSQfjRQUB8z/UpEoS27J4Tkl1/a30blvTPurkdC3jA=="
  );

  ret = gst_element_set_state(sink, GST_STATE_PLAYING);
  fail_unless(ret == GST_STATE_CHANGE_FAILURE);

  gst_element_set_state(sink, GST_STATE_NULL);
  gst_object_unref(sink);
}
GST_END_TEST
