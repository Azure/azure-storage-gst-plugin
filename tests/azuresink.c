#include <gst/check/gstcheck.h>
#include <gst/gst.h>

#include "gstazuresink.h"
#include "gstazuresinkconfig.h"

// credentials for testing
static const gchar *account_name = "gstvideostore";
static const gchar *account_key = "vIjLNYmW60yVH1nLc08u/KGJNXZl6Gzy7zdGtpSK3JJ6vSQfjRQUB8z/UpEoS27J4Tkl1/a30blvTPurkdC3jA==";
static const gchar *default_location = "videostore/testvideostream";

static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE("src",
  GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

// uploader stub
typedef struct
{
  gboolean flushed;
} TestUploaderData;

static gboolean test_uploader_init(GstAzureUploader* uploader, const gchar* container_name, const gchar* blob_name)
{
  return strlen(container_name) > 0 && strlen(blob_name) > 0;
}

static gboolean test_uploader_flush(GstAzureUploader* uploader)
{
  ((TestUploaderData *)(uploader->data))->flushed = TRUE;
  return TRUE;
}

static gboolean test_uploader_destroy(GstAzureUploader* uploader)
{
  return TRUE;
}

static gboolean test_uploader_upload(GstAzureUploader* uploader, const gchar* data, gsize size)
{
  ((TestUploaderData *)(uploader->data))->flushed = FALSE;
  return TRUE;
}

static gboolean test_uploader_flushed(GstAzureUploader* uploader)
{
  return ((TestUploaderData *)(uploader->data))->flushed;
}

static GstAzureUploaderClass test_uploader_klass = {
  .init = test_uploader_init,
  .flush = test_uploader_flush,
  .destroy = test_uploader_destroy,
  .upload = test_uploader_upload
};

static GstAzureUploader *test_uploader_new()
{
  GstAzureUploader* uploader = g_new(GstAzureUploader, 1);
  fail_if(uploader == NULL);
  uploader->klass = &test_uploader_klass;
  uploader->impl = NULL;
  uploader->data = g_new(TestUploaderData, 1);
  fail_if(uploader->data == NULL);
  return uploader;
}

static GstElement *get_default_sink(GstAzureUploader *uploader)
{
  GstElement *sink = gst_element_factory_make("azuresink", "sink");
  fail_if(sink == NULL);
  g_object_set(sink,
    "account-name", account_name,
    "account-key", account_key,
    "location", default_location,
    NULL
  );
  GST_AZURE_SINK(sink)->uploader = uploader;
  return sink;
}

static GstFlowReturn push_bytes(GstPad *pad, size_t num_bytes)
{
  GstBuffer *buf = gst_buffer_new_and_alloc(num_bytes);
  GRand *rand = g_rand_new_with_seed(num_bytes);
  GstMapInfo info;

  if(!gst_buffer_map(buf, &info, GST_MAP_WRITE))
  {
    gst_buffer_unref(buf);
    g_rand_free(rand);
    return FALSE;
  }

  for(int i = 0; i < num_bytes; i++)
    ((guint8 *)info.data)[i] = (g_rand_int(rand) >> 24) & 0xff;
  gst_buffer_unmap(buf, &info);
  GstFlowReturn ret = gst_pad_push(pad, buf);
  g_rand_free(rand);
  return ret;
}

#define PUSH_BYTES(_pad, _size) fail_unless_equals_int(GST_FLOW_OK, push_bytes(_pad, _size))

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
    "account_name", account_name,
    "account_key", account_key,
    NULL
  );

  ret = gst_element_set_state(sink, GST_STATE_PLAYING);
  fail_unless(ret == GST_STATE_CHANGE_FAILURE);

  gst_element_set_state(sink, GST_STATE_NULL);
  gst_object_unref(sink);
}
GST_END_TEST

GST_START_TEST(test_send_eos_should_flush)
{
  GstAzureUploader *uploader = test_uploader_new();
  GstElement *sink = get_default_sink(uploader);
  GstStateChangeReturn ret;

  GstPad *srcpad = gst_check_setup_src_pad(sink, &srcTemplate);
  gst_pad_set_active(srcpad, TRUE);
  
  ret = gst_element_set_state(sink, GST_STATE_PLAYING);
  fail_unless(ret == GST_STATE_CHANGE_ASYNC);

  // push bytes and flush
  PUSH_BYTES(srcpad, 16);

  GstPad *sinkpad = gst_element_get_static_pad(sink, "sink");
  gst_pad_send_event(sinkpad, gst_event_new_eos());
  gst_object_unref(sinkpad);
  fail_unless_equals_int(TRUE, test_uploader_flushed(uploader));

  gst_element_set_state(sink, GST_STATE_NULL);
  gst_object_unref(sink);
  gst_object_unref(srcpad);
  g_free(uploader);
}
GST_END_TEST

GST_START_TEST(test_invalid_blob_type_should_ignore)
{
  GstAzureUploader *uploader = test_uploader_new();
  GstElement *sink = get_default_sink(uploader);
  
  g_object_set(sink,
    "account_name", account_name,
    "account_key", account_key,
    "location", "videostore/testvideostream",
    "blob-type", "invalid-blob-type",
    NULL
  );

  gchar *new_blob_type = NULL;
  g_object_get(sink, "blob-type", &new_blob_type, NULL);
  fail_unless_equals_string("block", new_blob_type);

  gst_element_set_state(sink, GST_STATE_NULL);
  gst_object_unref(sink);
  g_free(uploader);
}
GST_END_TEST

GST_START_TEST(test_invalid_location_should_fail)
{
  GstElement* sink = gst_element_factory_make("azuresink", "sink");
  GstStateChangeReturn ret;
  fail_if(sink == NULL);
  
  g_object_set(sink,
    "account_name", account_name,
    "account_key", account_key,
    "location", "invalid-location-blablabla",
    NULL
  );

  ret = gst_element_set_state(sink, GST_STATE_PLAYING);
  fail_unless(ret == GST_STATE_CHANGE_FAILURE);

  gst_element_set_state(sink, GST_STATE_NULL);
  gst_object_unref(sink);
}
GST_END_TEST

GST_START_TEST(test_query_position)
{
  GstAzureUploader *uploader = test_uploader_new();
  GstElement *sink = get_default_sink(uploader);

  const guint64 bytes_to_write = 23333;
  GstPad *srcpad = gst_check_setup_src_pad(sink, &srcTemplate);
  gst_pad_set_active(srcpad, TRUE);

  GstPad *sinkpad = gst_element_get_static_pad(sink, "sink");
  fail_if(sinkpad == NULL);

  GstStateChangeReturn ret = gst_element_set_state(sink, GST_STATE_PLAYING);
  fail_unless_equals_int(GST_STATE_CHANGE_ASYNC, ret);

  PUSH_BYTES(srcpad, bytes_to_write);
  gint64 position = 0;
  gst_pad_query_position(sinkpad, GST_FORMAT_BYTES, &position);
  fail_unless_equals_int64(bytes_to_write, position);
  
  gst_element_set_state(sink, GST_STATE_NULL);
  gst_object_unref(sinkpad);
  gst_object_unref(srcpad);
  gst_object_unref(sink);
  g_free(uploader);
}
GST_END_TEST

GST_START_TEST(test_query_seeking_should_fail)
{
  GstAzureUploader *uploader = test_uploader_new();
  GstElement *sink = get_default_sink(uploader);
  
  GstStateChangeReturn ret = gst_element_set_state (sink, GST_STATE_PLAYING);
  fail_unless_equals_int(GST_STATE_CHANGE_ASYNC, ret);

  GstQuery* query = gst_query_new_seeking(GST_FORMAT_DEFAULT);
  GstFormat format;
  gboolean seekable;
  gst_element_query (sink, query);
  gst_query_parse_seeking (query, &format, &seekable, NULL, NULL);
  fail_if (seekable == TRUE);
  fail_unless_equals_int(GST_FORMAT_DEFAULT, format);

  gst_query_unref (query);
  gst_element_set_state (sink, GST_STATE_NULL);
  gst_object_unref (sink);
  g_free(uploader);
}
GST_END_TEST


GST_PLUGIN_STATIC_DECLARE(azureelements);

static Suite *azuresink_suite(void)
{
  Suite *s = suite_create("azuresink");
  TCase *tc_chain = tcase_create("general");

  suite_add_tcase(s, tc_chain);
  tcase_set_timeout(tc_chain, 30);
  tcase_add_test(tc_chain, test_no_credential_should_fail);
  tcase_add_test(tc_chain, test_no_location_should_fail);
  tcase_add_test(tc_chain, test_invalid_location_should_fail);
  tcase_add_test(tc_chain, test_invalid_blob_type_should_ignore);
  tcase_add_test(tc_chain, test_send_eos_should_flush);
  tcase_add_test(tc_chain, test_query_position);
  tcase_add_test(tc_chain, test_query_seeking_should_fail);

  return s;
}

GST_CHECK_MAIN(azuresink)