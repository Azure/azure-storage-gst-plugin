/* GStreamer
 * Copyright (C) 2020 Eugene Chen <t-yijunc@microsoft.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstazuresrc
 *
 * Read blob from azure storage.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v azuresrc \
 *    account-name="..." account-key="..." block-size=1048576 \
 *    location = "container_name/blob_name" ! fakesink
 * ]|
 * Read from azure blob storage 1MiB by 1MiB and feed the content to fakesink.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include "gstazuresrc.h"
#include "gstazuresrcconfig.h"
#include "gstazuredownloader.h"
#include "azuredownloader.h"
#include "utils/gstutils.h"

GST_DEBUG_CATEGORY_STATIC(gst_azuresrc_debug_category);
#define GST_CAT_DEFAULT gst_azuresrc_debug_category

/* prototypes */

static void gst_azure_src_set_property(GObject *object,
                                       guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_azure_src_get_property(GObject *object,
                                       guint property_id, GValue *value, GParamSpec *pspec);
static void gst_azure_src_finalize(GObject *object);

static gboolean gst_azure_src_start(GstBaseSrc *basesrc);
static gboolean gst_azure_src_stop(GstBaseSrc *basesrc);
static gboolean gst_azure_src_is_seekable(GstBaseSrc *basesrc);
static gboolean gst_azure_src_get_size(GstBaseSrc *basesrc, gsize *size);
static GstFlowReturn gst_azure_src_fill(GstBaseSrc *src, guint64 offset,
                                        guint length, GstBuffer *buf);

enum
{
  PROP_0,
  PROP_BLOB_ENDPOINT,
  PROP_ACCOUNT_NAME,
  PROP_ACCOUNT_KEY,
  PROP_LOCATION,
  PROP_USE_HTTPS,
  PROP_WORKER_COUNT,
  PROP_BLOCK_SIZE,
  PROP_PREFETCH_BLOCK_COUNT,
};

/* pad templates */
static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE("src",
                                                                  GST_PAD_SRC,
                                                                  GST_PAD_ALWAYS,
                                                                  GST_STATIC_CAPS_ANY);

/* class initialization */

G_DEFINE_TYPE_WITH_CODE(GstAzureSrc, gst_azure_src, GST_TYPE_BASE_SRC,
                        GST_DEBUG_CATEGORY_INIT(gst_azuresrc_debug_category, "azuresrc", 0,
                                                "debug category for azuresrc element"));

static void
gst_azure_src_class_init(GstAzureSrcClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
  GstBaseSrcClass *basesrc_class = GST_BASE_SRC_CLASS(klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */

  gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
                                        "Azure storage source",
                                        "Generic",
                                        "Read blob from azure blob storage.",
                                        "Eugene Chen<t-yijunc@microsoft.com>");

  gst_element_class_add_static_pad_template(element_class, &srctemplate);

  gobject_class->set_property = gst_azure_src_set_property;
  gobject_class->get_property = gst_azure_src_get_property;
  gobject_class->finalize = gst_azure_src_finalize;

  g_object_class_install_property(gobject_class, PROP_ACCOUNT_NAME,
                                  g_param_spec_string("account-name", "azure account name",
                                                      "Your azure storage account.", NULL,
                                                      (G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property(gobject_class, PROP_ACCOUNT_KEY,
                                  g_param_spec_string("account-key", "azure account key",
                                                      "Your azure storage account key.", NULL,
                                                      (G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property(gobject_class, PROP_LOCATION,
                                  g_param_spec_string("location", "azure blob storage blob location",
                                                      "The blob you want to read from in container-name/blob-name format.", NULL,
                                                      (G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property(gobject_class, PROP_BLOB_ENDPOINT,
                                  g_param_spec_string("blob-endpoint", "azure blob storage endpoint",
                                                      "Azure blob storage service endpoint. Set it to your blob service's url "
                                                      "if you're using an emulator, leave it blank otherwise.",
                                                      NULL,
                                                      (G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property(gobject_class, PROP_USE_HTTPS,
                                  g_param_spec_boolean("use-https", "azure storage use https",
                                                       "Whether to use https or not in azure storage REST API. This is highly recommended "
                                                       "and enabled by default, by you can turn it off for debug purporses.",
                                                       TRUE, G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class, PROP_WORKER_COUNT,
                                  g_param_spec_uint("worker-count", "azure storage worker count",
                                                    "The number of concurrent block downloaders. 4 by default.",
                                                    1, 64, AZURE_SRC_DEFAULT_WORKER_COUNT, G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class, PROP_BLOCK_SIZE,
                                  g_param_spec_uint64("block-size", "azure storage block size",
                                                      "The size of one block, which is the mininal download unit. 1MiB by default.",
                                                      1, 64 * 1024 * 1024, AZURE_SRC_DEFAULT_BLOCK_SIZE, G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class, PROP_PREFETCH_BLOCK_COUNT,
                                  g_param_spec_uint("prefetch-block-count", "azure storage downloader prefetch block count",
                                                    "The amount of data prefetched by downloader ahead of time. 4 blocks(the default worker number) by default.",
                                                    1, 64, AZURE_SRC_DEFAULT_PREFETCH_BLOCK_COUNT, G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS));

  basesrc_class->start = gst_azure_src_start;
  basesrc_class->stop = gst_azure_src_stop;
  basesrc_class->is_seekable = gst_azure_src_is_seekable;
  basesrc_class->get_size = gst_azure_src_get_size;
  basesrc_class->fill = gst_azure_src_fill;
}

static void
gst_azure_src_init(GstAzureSrc *azuresrc)
{
  azuresrc->config = AZURE_SRC_DEFAULT_CONFIG;
  azuresrc->downloader = NULL;
  azuresrc->read_position = 0;
  // GST_DEBUG_OBJECT(azuresrc, "init");
  /* NOTE pads are configured here with gst_pad_set_*_function () */
}

void gst_azure_src_set_property(GObject *object, guint property_id,
                                const GValue *value, GParamSpec *pspec)
{
  GstAzureSrc *azuresrc = GST_AZURE_SRC(object);

  GST_DEBUG_OBJECT(azuresrc, "set_property");

  switch (property_id)
  {
  case PROP_ACCOUNT_NAME:
    gst_azure_elements_set_string_property(azuresrc, value,
                                           &azuresrc->config.account_name, "account-name");
    break;
  case PROP_ACCOUNT_KEY:
    gst_azure_elements_set_string_property(azuresrc, value,
                                           &azuresrc->config.account_key, "account_key");
    break;
  case PROP_LOCATION:
  {
    const gchar *v = g_value_get_string(value);
    if (v != NULL)
    {
      gchar **tokens = g_strsplit(v, "/", 2);
      if (tokens[0] == NULL || tokens[1] == NULL)
      {
        GST_ELEMENT_ERROR(azuresrc, RESOURCE, FAILED,
                          ("Location is invalid, expecting container/blob, found %s\n", v), (NULL));
        break;
      }
      azuresrc->config.container_name = tokens[0];
      azuresrc->config.blob_name = tokens[1];
      g_info("Setting container name to %s, blob name to %s.\n", tokens[0], tokens[1]);
    }
    break;
  }
  case PROP_BLOB_ENDPOINT:
    gst_azure_elements_set_string_property(azuresrc, value,
                                           &azuresrc->config.blob_endpoint, "blob-endpoint");
    break;
  case PROP_USE_HTTPS:
    gst_azure_elements_set_boolean_property(azuresrc, value,
                                            &azuresrc->config.use_https, "use-https");
    break;
  case PROP_WORKER_COUNT:
    gst_azure_elements_set_uint_property(azuresrc, value,
                                         &azuresrc->config.worker_count, "worker-count");
    break;
  case PROP_BLOCK_SIZE:
    gst_azure_elements_set_uint64_property(azuresrc, value,
                                           &azuresrc->config.block_size, "block-size");
    break;
  case PROP_PREFETCH_BLOCK_COUNT:
    gst_azure_elements_set_uint_property(azuresrc, value,
                                         &azuresrc->config.prefetch_block_count, "prefetch-block-count");
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

void gst_azure_src_get_property(GObject *object, guint property_id,
                                GValue *value, GParamSpec *pspec)
{
  GstAzureSrc *azuresrc = GST_AZURE_SRC(object);

  GST_DEBUG_OBJECT(azuresrc, "get_property");

  switch (property_id)
  {
  case PROP_BLOB_ENDPOINT:
    g_value_set_string(value, azuresrc->config.blob_endpoint);
    break;
  case PROP_ACCOUNT_NAME:
    g_value_set_string(value, azuresrc->config.account_name);
    break;
  case PROP_ACCOUNT_KEY:
    g_value_set_string(value, azuresrc->config.account_key);
    break;
  case PROP_LOCATION:
  {
    const gchar *loc = g_strconcat(azuresrc->config.container_name, "/", azuresrc->config.blob_name, NULL);
    g_value_set_string(value, loc);
    break;
  }
  case PROP_USE_HTTPS:
    g_value_set_boolean(value, azuresrc->config.use_https);
    break;
  case PROP_WORKER_COUNT:
    g_value_set_uint(value, azuresrc->config.worker_count);
    break;
  case PROP_BLOCK_SIZE:
    g_value_set_uint64(value, azuresrc->config.block_size);
    break;
  case PROP_PREFETCH_BLOCK_COUNT:
    g_value_set_uint(value, azuresrc->config.prefetch_block_count);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
  }
}

void gst_azure_src_finalize(GObject *object)
{
  GstAzureSrc *azuresrc = GST_AZURE_SRC(object);

  GST_DEBUG_OBJECT(azuresrc, "finalize");
  // free up configuration and downloader
  gst_azure_src_release_config(&azuresrc->config);
  if (azuresrc->downloader != NULL)
    gst_azure_downloader_destroy(azuresrc->downloader);
  azuresrc->downloader = NULL;

  G_OBJECT_CLASS(gst_azure_src_parent_class)->finalize(object);
}

gboolean
gst_azure_src_start(GstBaseSrc *basesrc)
{
  GstAzureSrc *azuresrc = GST_AZURE_SRC(basesrc);

  if (GSTR_IS_EMPTY(azuresrc->config.account_key) ||
      GSTR_IS_EMPTY(azuresrc->config.account_name))
  {
    GST_ELEMENT_ERROR(azuresrc, RESOURCE, NOT_AUTHORIZED,
                      ("Missing account name or account key."), (NULL));
    return FALSE;
  }
  if (GSTR_IS_EMPTY(azuresrc->config.container_name) ||
      GSTR_IS_EMPTY(azuresrc->config.blob_name))
  {
    GST_ELEMENT_ERROR(azuresrc, RESOURCE, NOT_FOUND,
                      ("Missing container name or blob name, cannot determine source."), (NULL));
    return FALSE;
  }
  if (azuresrc->downloader == NULL)
  {
    azuresrc->downloader = gst_azure_src_downloader_new(&azuresrc->config);
    if (azuresrc->downloader == NULL)
    {
      GST_ELEMENT_ERROR(azuresrc, RESOURCE, FAILED,
                        ("Failed to create new downloader."), (NULL));
      return FALSE;
    }
  }
  if (gst_azure_downloader_init(azuresrc->downloader,
                                azuresrc->config.container_name, azuresrc->config.blob_name) == FALSE)
  {
    GST_ELEMENT_ERROR(azuresrc, RESOURCE, NOT_FOUND,
                      ("Failed to initialize downloader, maybe resource does not exist."), (NULL));
    return FALSE;
  }
  return TRUE;
}

static gboolean
gst_azure_src_stop(GstBaseSrc *basesrc)
{
  GstAzureSrc *src = GST_AZURE_SRC(basesrc);
  return gst_azure_downloader_destroy(src->downloader);
}

static gboolean
gst_azure_src_is_seekable(GstBaseSrc *basesrc)
{
  // azure src is always seekable
  return TRUE;
}

static gboolean
gst_azure_src_get_size(GstBaseSrc *basesrc, gsize *size)
{
  GstAzureSrc *src = GST_AZURE_SRC(basesrc);
  if (src->downloader == NULL)
    return FALSE;
  // FIXME retun FALSE on failure(not initialized)
  *size = gst_azure_downloader_get_size(src->downloader);
  return TRUE;
}

static GstFlowReturn
gst_azure_src_fill(GstBaseSrc *src, guint64 offset,
                   guint length, GstBuffer *buf)
{
  GstAzureSrc *azure_src = GST_AZURE_SRC(src);
  if (azure_src->downloader == NULL)
    return FALSE;
  if (G_UNLIKELY(offset != -1 && offset != azure_src->read_position))
  {
    // seek
    if (!gst_azure_downloader_seek(azure_src->downloader, offset))
    {
      GST_ELEMENT_ERROR(azure_src, RESOURCE, SEEK, ("Cannot seek blob file."), (NULL));
      return GST_FLOW_ERROR;
    }
  }
  GstMapInfo info;
  // map the memory for writting
  if (!gst_buffer_map(buf, &info, GST_MAP_WRITE))
  {
    GST_ELEMENT_ERROR(azure_src, RESOURCE, WRITE, (NULL), ("Cannot write to buffer."));
    return GST_FLOW_ERROR;
  }
  gsize bytes_read = 0;
  while (bytes_read < length)
  {
    gsize ret = gst_azure_downloader_read(azure_src->downloader, (char *)info.data + bytes_read, length - bytes_read);
    bytes_read += ret;
    azure_src->read_position += ret;
    // gst_println("Read position = %ld\n", azure_src->read_position);
    if (G_UNLIKELY(ret == 0))
    {
      // eos
      if (bytes_read == 0)
      {
        gst_buffer_unmap(buf, &info);
        gst_buffer_resize(buf, 0, 0);
        return GST_FLOW_EOS;
      }
      else
        break;
    }
  }
  gst_buffer_unmap(buf, &info);
  gst_buffer_resize(buf, 0, bytes_read);
  GST_BUFFER_OFFSET(buf) = offset;
  GST_BUFFER_OFFSET_END(buf) = offset + bytes_read;
  return GST_FLOW_OK;
}