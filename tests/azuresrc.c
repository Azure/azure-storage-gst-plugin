#include <gst/check/gstcheck.h>
#include <gst/gst.h>

#include "gstazuresrc.h"
#include "gstazuresrcconfig.h"
#include "gstazuredownloader.h"

// credentials for testing
static const gchar *account_name = "gstvideostore";
static const gchar *account_key = "vIjLNYmW60yVH1nLc08u/KGJNXZl6Gzy7zdGtpSK3JJ6vSQfjRQUB8z/UpEoS27J4Tkl1/a30blvTPurkdC3jA==";
static const gchar *default_location = "videostore/testvideostream";

static GstStaticPadTemplate sinkTemplate = GST_STATIC_PAD_TEMPLATE("sink",
  GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

// downloader stub
typedef struct
{
  size_t offset;
  gboolean destroyed;
} TestDownloaderData;

static inline TestDownloaderData *test_data(GstAzureDownloader *downloader)
{
  return (TestDownloaderData *)(downloader->data);
}
#define TEST_BLOB_SIZE 1048576

gboolean test_downloader_init(GstAzureDownloader* downloader, const gchar* container_name, const gchar* blob_name)
{
  test_data(downloader)->offset = 0;
  test_data(downloader)->destroyed = FALSE;
  return TRUE;
}
gsize test_downloader_read(GstAzureDownloader* downloader, gchar* buffer, gsize size)
{
  if(test_data(downloader)->destroyed) return FALSE;
  size_t start = test_data(downloader)->offset;
  size_t end = test_data(downloader)->offset + size;
  end = end > TEST_BLOB_SIZE ? TEST_BLOB_SIZE : end;
  for(size_t i = test_data(downloader)->offset; i < end; i++)
    buffer[i] = i & 0xff;
  test_data(downloader)->offset = end;
  return end - start;
}

gboolean test_downloader_seek(GstAzureDownloader* downloader, goffset offset)
{
  if(test_data(downloader)->destroyed) return FALSE;
  test_data(downloader)->offset = offset;
  return TRUE;
}

gsize test_downloader_get_size(GstAzureDownloader* downloader)
{
  return TEST_BLOB_SIZE;
}

gboolean test_downloader_destroy(GstAzureDownloader* downloader)
{
  test_data(downloader)->destroyed = TRUE;
  return TRUE;
}

static const GstAzureDownloaderClass test_downloader_class = {
  .init = test_downloader_init,
  .read = test_downloader_read,
  .seek = test_downloader_seek,
  .get_size = test_downloader_get_size,
  .destroy = test_downloader_destroy
};

GstAzureDownloader* test_downloader_new()
{
  GstAzureDownloader *downloader = g_new(GstAzureDownloader, 1);
  fail_if(downloader == NULL);
  downloader->klass = &test_downloader_class;
  downloader->impl = NULL;
  TestDownloaderData *dat = g_new(TestDownloaderData, 1);
  dat->destroyed = FALSE;
  downloader->data = dat;
  return downloader;
}

static GstElement *get_default_src(GstAzureDownloader *downloader)
{
  GstElement *src = gst_element_factory_make("azuresrc", "src");
  fail_if(src == NULL);
  g_object_set(src,
    "account-name", account_name,
    "account-key", account_key,
    "location", default_location,
    NULL
  );
  GST_AZURE_SRC(src)->downloader = downloader;
  return src;
}

static GstFlowReturn pull_bytes(GstPad *pad, size_t offset, size_t num_bytes)
{
  GstBuffer *buf = gst_buffer_new_and_alloc(num_bytes);
  GstMapInfo info;

  if(!gst_buffer_map(buf, &info, GST_MAP_WRITE))
  {
    gst_buffer_unref(buf);
    return FALSE;
  }
  GstFlowReturn ret = gst_pad_pull_range(pad, offset, num_bytes, &buf);
  gst_buffer_unmap(buf, &info);
  return ret;
}

GST_START_TEST(test_no_credential_should_fail)
{
  GstElement* src = gst_element_factory_make("azuresrc", "src");
  GstStateChangeReturn ret;
  fail_if(src == NULL);

  ret = gst_element_set_state(src, GST_STATE_PLAYING);
  fail_unless(ret == GST_STATE_CHANGE_FAILURE);

  gst_element_set_state(src, GST_STATE_NULL);
  gst_object_unref(src);
}
GST_END_TEST

GST_START_TEST(test_no_location_should_fail)
{
  GstElement* src = gst_element_factory_make("azuresrc", "src");
  GstStateChangeReturn ret;
  fail_if(src == NULL);
  
  g_object_set(src,
    "account_name", account_name,
    "account_key", account_key,
    NULL
  );

  ret = gst_element_set_state(src, GST_STATE_PLAYING);
  fail_unless(ret == GST_STATE_CHANGE_FAILURE);

  gst_element_set_state(src, GST_STATE_NULL);
  gst_object_unref(src);
}
GST_END_TEST

GST_START_TEST(test_invalid_location_should_fail)
{
  GstElement* src = gst_element_factory_make("azuresrc", "src");
  GstStateChangeReturn ret;
  fail_if(src == NULL);
  
  g_object_set(src,
    "account_name", account_name,
    "account_key", account_key,
    "location", "invalid-location-blablabla",
    NULL
  );

  ret = gst_element_set_state(src, GST_STATE_PLAYING);
  fail_unless(ret == GST_STATE_CHANGE_FAILURE);

  gst_element_set_state(src, GST_STATE_NULL);
  gst_object_unref(src);
}
GST_END_TEST

GST_START_TEST(test_read_content_sequential)
{
  GstAzureDownloader *downloader = test_downloader_new();
  GstElement *src = get_default_src(downloader);
  
  GstPad *sinkpad = gst_check_setup_sink_pad(src, &sinkTemplate);
  fail_if(sinkpad == NULL);
  gst_pad_set_active(sinkpad, TRUE);
  GstPad *srcpad = gst_element_get_static_pad(src, "src");
  fail_if(srcpad == NULL);

  GstStateChangeReturn ret = gst_element_set_state(src, GST_STATE_PLAYING);
  fail_unless_equals_int(GST_STATE_CHANGE_SUCCESS, ret);
  
  g_mutex_lock(&check_mutex);
  while(g_list_length(buffers) == 0)
  {
    g_cond_wait(&check_cond, &check_mutex);
    GST_INFO("%u buffers", g_list_length(buffers));
  }
  g_mutex_unlock(&check_mutex);
  gst_element_set_state(src, GST_STATE_NULL);
  gst_object_unref(src);
  gst_object_unref(sinkpad);
  g_free(downloader);
}
GST_END_TEST

GST_PLUGIN_STATIC_DECLARE(azureelements);

static Suite *azuresrc_suite(void)
{
  Suite *s = suite_create("azuresrc");
  TCase *tc_chain = tcase_create("general");

  suite_add_tcase(s, tc_chain);
  tcase_set_timeout(tc_chain, 30);
  tcase_add_test(tc_chain, test_no_credential_should_fail);
  tcase_add_test(tc_chain, test_no_location_should_fail);
  tcase_add_test(tc_chain, test_invalid_location_should_fail);
  // tcase_add_test(tc_chain, test_read_content_sequential);
  return s;
}

GST_CHECK_MAIN(azuresrc)