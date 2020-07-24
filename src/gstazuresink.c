/* azure-storage-gst-plugin
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
 * SECTION:element-gstazuresink
 *
 * Upload the media content to azure blob storage.
 *
 * <refsect2>
 * <title>Example</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! azuresink
 * ]|
 * Upload fakesrc's content to azure blob storage.
 * </refsect2>
 */

// #ifdef HAVE_CONFIG_H
// #include "config.h"
// #endif
#include "gstazuresink.h"

#include <gst/gst.h>
#include <gst/base/gstbasesink.h>

#include "simpleazureuploader.h"
#include "blockazureuploader.h"
#include "utils/gstutils.h"

GST_DEBUG_CATEGORY_STATIC (gst_azure_sink_debug_category);
#define GST_CAT_DEFAULT gst_azure_sink_debug_category

/* prototypes */


static void gst_azure_sink_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_azure_sink_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
// static void gst_azure_sink_dispose (GObject * object);
// static void gst_azure_sink_finalize (GObject * object);

// static GstCaps *gst_azure_sink_get_caps (GstBaseSink * sink, GstCaps * filter);
// static gboolean gst_azure_sink_set_caps (GstBaseSink * sink, GstCaps * caps);
// static GstCaps *gst_azure_sink_fixate (GstBaseSink * sink, GstCaps * caps);
// static gboolean gst_azure_sink_activate_pull (GstBaseSink * sink, gboolean active);
// static void gst_azure_sink_get_times (GstBaseSink * sink, GstBuffer * buffer,
//     GstClockTime * start, GstClockTime * end);
// static gboolean gst_azure_sink_propose_allocation (GstBaseSink * sink,
//     GstQuery * query);
static gboolean gst_azure_sink_start (GstBaseSink * sink);
static gboolean gst_azure_sink_stop (GstBaseSink * sink);
// static gboolean gst_azure_sink_unlock (GstBaseSink * sink);
// static gboolean gst_azure_sink_unlock_stop (GstBaseSink * sink);
static gboolean gst_azure_sink_query (GstBaseSink * sink, GstQuery * query);
static gboolean gst_azure_sink_event (GstBaseSink * sink, GstEvent * event);
// static GstFlowReturn gst_azure_sink_wait_event (GstBaseSink * sink,
//     GstEvent * event);
// static GstFlowReturn gst_azure_sink_prepare (GstBaseSink * sink,
//     GstBuffer * buffer);
// static GstFlowReturn gst_azure_sink_prepare_list (GstBaseSink * sink,
//     GstBufferList * buffer_list);
// static GstFlowReturn gst_azure_sink_preroll (GstBaseSink * sink,
//     GstBuffer * buffer);
static GstFlowReturn gst_azure_sink_render (GstBaseSink * sink,
    GstBuffer * buffer);
// static GstFlowReturn gst_azure_sink_render_list (GstBaseSink * sink,
//     GstBufferList * buffer_list);

enum
{
  PROP_0,
  PROP_BLOB_ENDPOINT,
  PROP_ACCOUNT_NAME,
  PROP_ACCOUNT_KEY,
  PROP_CONTAINER_NAME,
  PROP_BLOB_NAME,
  PROP_LOCATION,
  PROP_USE_HTTPS,
  PROP_BLOCK_SIZE,
  PROP_WORKER_COUNT,
  PROP_COMMIT_BLOCK_COUNT,
  PROP_COMMIT_INTERVAL_MS,
};

/* pad templates */

static GstStaticPadTemplate gst_azure_sink_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstAzureSink, gst_azure_sink, GST_TYPE_BASE_SINK,
  GST_DEBUG_CATEGORY_INIT (gst_azure_sink_debug_category, "azuresink", 0,
  "debug category for azuresink element"));

static void
gst_azure_sink_class_init (GstAzureSinkClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseSinkClass *base_sink_class = GST_BASE_SINK_CLASS (klass);
  GST_INFO("azuresink doing configuration");

  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS(klass),
      &gst_azure_sink_sink_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
      "Azure storage sink",
      "Generic",
      "Write stream into azure blob storage.",
      "Eugene Chen-yijunc@microsoft.com");

  GST_INFO("azuresink set metadata");
  gobject_class->set_property = gst_azure_sink_set_property;
  gobject_class->get_property = gst_azure_sink_get_property;
  // gobject_class->dispose = gst_azure_sink_dispose;
  // gobject_class->finalize = gst_azure_sink_finalize;
  // base_sink_class->get_caps = GST_DEBUG_FUNCPTR (gst_azure_sink_get_caps);
  // base_sink_class->set_caps = GST_DEBUG_FUNCPTR (gst_azure_sink_set_caps);
  // base_sink_class->fixate = GST_DEBUG_FUNCPTR (gst_azure_sink_fixate);
  // base_sink_class->activate_pull = GST_DEBUG_FUNCPTR (gst_azure_sink_activate_pull);
  // base_sink_class->get_times = GST_DEBUG_FUNCPTR (gst_azure_sink_get_times);
  // base_sink_class->propose_allocation = GST_DEBUG_FUNCPTR (gst_azure_sink_propose_allocation);
  base_sink_class->start = GST_DEBUG_FUNCPTR (gst_azure_sink_start);
  base_sink_class->stop = GST_DEBUG_FUNCPTR (gst_azure_sink_stop);
  // base_sink_class->unlock = GST_DEBUG_FUNCPTR (gst_azure_sink_unlock);
  // base_sink_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_azure_sink_unlock_stop);
  base_sink_class->query = GST_DEBUG_FUNCPTR (gst_azure_sink_query);
  base_sink_class->event = GST_DEBUG_FUNCPTR (gst_azure_sink_event);
  // base_sink_class->wait_event = GST_DEBUG_FUNCPTR (gst_azure_sink_wait_event);
  // base_sink_class->prepare = GST_DEBUG_FUNCPTR (gst_azure_sink_prepare);
  // base_sink_class->prepare_list = GST_DEBUG_FUNCPTR (gst_azure_sink_prepare_list);
  // base_sink_class->preroll = GST_DEBUG_FUNCPTR (gst_azure_sink_preroll);
  base_sink_class->render = GST_DEBUG_FUNCPTR (gst_azure_sink_render);
  // base_sink_class->render_list = GST_DEBUG_FUNCPTR (gst_azure_sink_render_list);

  g_object_class_install_property (gobject_class, PROP_ACCOUNT_NAME,
    g_param_spec_string ("account-name", "azure account name",
      "Your azure storage account.", NULL,
      (G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_ACCOUNT_KEY,
    g_param_spec_string ("account-key", "azure account key",
      "Your azure storage account key.", NULL,
      (G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_CONTAINER_NAME,
    g_param_spec_string ("container-name", "azure blob storage container name",
      "The azure blob storage container to store the blob file.", NULL,
      (G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS)));
        
  g_object_class_install_property (gobject_class, PROP_BLOB_NAME,
    g_param_spec_string ("blob-name", "azure blob storage blob name",
      "The file name you want to write into.", NULL,
      (G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_LOCATION,
    g_param_spec_string ("location", "azure blob storage blob location",
      "The blob you want to write into in container-name/blob-name format. "
      "This property is to provide compatibility with other plugins. "
      "If container name and blob name are provided, this property is ignored.", NULL,
      (G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_BLOB_ENDPOINT,
    g_param_spec_string ("blob-endpoint", "azure blob storage endpoint",
      "Azure blob storage service endpoint. Set it to your blob service's url "
      "if you're using an emulator, leave it blank otherwise.", NULL,
      (G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS)));
  
  g_object_class_install_property (gobject_class, PROP_USE_HTTPS,
    g_param_spec_boolean("use-https", "azure storage use https",
      "Whether to use https or not in azure storage REST API. This is highly recommended "
      "and enabled by default, by you can turn it off for debug purporses.",
      TRUE, G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS));
  
  g_object_class_install_property (gobject_class, PROP_BLOCK_SIZE,
    g_param_spec_uint("block-size", "azure storage block size",
      "Block size for azure storage block blob in bytes. 4MiB by default.",
      1, 100 * 1024 * 1024, AZURE_SINK_DEFAULT_BLOCK_SIZE, G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS));
  
  g_object_class_install_property (gobject_class, PROP_WORKER_COUNT,
    g_param_spec_uint("worker-count", "azure storage worker count",
      "The number of concurrent block uploaders. 4 by default.",
      1, 64, AZURE_SINK_DEFAULT_WORKER_COUNT, G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS));
  
  g_object_class_install_property (gobject_class, PROP_COMMIT_BLOCK_COUNT,
    g_param_spec_uint("commit-block-count", "azure storage commit block count",
      "The number of blocks left uncommitted when put block list(commit) is triggered. 16 by default.",
      0, 50000, AZURE_SINK_DEFAULT_COMMIT_BLOCK_COUNT, G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS));
  
  g_object_class_install_property (gobject_class, PROP_COMMIT_INTERVAL_MS,
    g_param_spec_uint("commit-interval-ms", "azure storage commit interval",
      "The maximum duration between two put block list(commit)s in milliseconds. 60 seconds by default, 1 day max.",
      0, 86400000, AZURE_SINK_DEFAULT_COMMIT_INTERVAL_MS, G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS));
}

static void
gst_azure_sink_init (GstAzureSink *azuresink)
{
  azuresink->config = AZURE_SINK_DEFAULT_CONFIG;
  azuresink->uploader = NULL;
  azuresink->total_bytes_written = 0;
  GST_DEBUG_OBJECT(azuresink, "init");
  /* NOTE pads are configured here with gst_pad_set_*_function () */
  // gst_element_add_pad(GST_ELEMENT(azuresink), azuresink->sinkpad);
  gst_base_sink_set_sync(GST_BASE_SINK(azuresink), FALSE);
}

void
gst_azure_sink_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (object);

  GST_DEBUG_OBJECT (azuresink, "set_property");

  switch (property_id) {
    case PROP_BLOB_ENDPOINT:
      gst_azure_sink_set_string_property(azuresink, value,
        &azuresink->config.blob_endpoint, "blob-endpoint");
      break;
    case PROP_ACCOUNT_NAME:
      gst_azure_sink_set_string_property(azuresink, value,
        &azuresink->config.account_name, "account-name");
      break;
    case PROP_ACCOUNT_KEY:
      gst_azure_sink_set_string_property(azuresink, value,
        &azuresink->config.account_key, "account_key");
      break;
    case PROP_CONTAINER_NAME:
      gst_azure_sink_set_string_property(azuresink, value,
        &azuresink->config.container_name, "container-name");
      break;
    case PROP_BLOB_NAME:
      gst_azure_sink_set_string_property(azuresink, value,
        &azuresink->config.blob_name, "blob-name");
      break;
    case PROP_LOCATION:
    {
      const gchar *v = g_value_get_string(value);
      if(v != NULL) {
        gchar **tokens = g_strsplit(v, "/", 2);
        if(tokens[0] != NULL)
          azuresink->config.container_name = tokens[0];
        if(tokens[1] != NULL)
          azuresink->config.blob_name = tokens[1];
        gst_print("Settting container name to %s, blob name to %s.\n", tokens[0], tokens[1]);
      }
      break;
    }
    case PROP_USE_HTTPS:
      gst_azure_sink_set_boolean_property(azuresink, value,
        &azuresink->config.use_https, "use-https");
      break;
    case PROP_BLOCK_SIZE:
      gst_azure_sink_set_uint_property(azuresink, value,
        &azuresink->config.block_size, "block-size");
      break;
    case PROP_WORKER_COUNT:
      gst_azure_sink_set_uint_property(azuresink, value,
        &azuresink->config.worker_count, "worker-count");
      break;
    case PROP_COMMIT_BLOCK_COUNT:
      gst_azure_sink_set_uint_property(azuresink, value,
        &azuresink->config.commit_block_count, "commit-block-count");
      break;
    case PROP_COMMIT_INTERVAL_MS:
      gst_azure_sink_set_uint_property(azuresink, value,
        &azuresink->config.commit_interval_ms, "commit-interval-ms");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


void
gst_azure_sink_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (object);

  GST_DEBUG_OBJECT (azuresink, "get_property");

  switch (property_id) {
    case PROP_BLOB_ENDPOINT:
      g_value_set_string(value, azuresink->config.blob_endpoint);
      break;
    case PROP_ACCOUNT_NAME:
      g_value_set_string(value, azuresink->config.account_name);
      break;
    case PROP_ACCOUNT_KEY:
      g_value_set_string(value, azuresink->config.account_key);
      break;
    case PROP_CONTAINER_NAME:
      g_value_set_string(value, azuresink->config.container_name);
      break;
    case PROP_LOCATION:
    {
      const gchar *loc = g_strconcat(azuresink->config.container_name, "/", azuresink->config.blob_name, NULL);
      g_value_set_string(value, loc);
      break;
    }
    case PROP_BLOB_NAME:
      g_value_set_string(value, azuresink->config.blob_name);
      break;
    case PROP_USE_HTTPS:
      g_value_set_boolean(value, azuresink->config.use_https);
      break;
    case PROP_WORKER_COUNT:
      g_value_set_uint(value, azuresink->config.worker_count);
      break;
    case PROP_BLOCK_SIZE:
      g_value_set_uint(value, azuresink->config.block_size);
      break;
    case PROP_COMMIT_BLOCK_COUNT:
      g_value_set_uint(value, azuresink->config.commit_block_count);
      break;
    case PROP_COMMIT_INTERVAL_MS:
      g_value_set_uint(value, azuresink->config.commit_interval_ms);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

// NOTE not included
void
gst_azure_sink_dispose (GObject * object)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (object);

  GST_DEBUG_OBJECT (azuresink, "dispose");

  /* clean up as possible.  may be called multiple times */
  // release config
  gst_azure_sink_release_config(&azuresink->config);
  G_OBJECT_CLASS (gst_azure_sink_parent_class)->dispose (object);
}

// NOTE not included
// NOTE called when plguin is destroyed
// maybe not necessary?
void
gst_azure_sink_finalize (GObject * object)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (object);

  GST_DEBUG_OBJECT (azuresink, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_azure_sink_parent_class)->finalize (object);
}

// NOTE not included
static GstCaps *
gst_azure_sink_get_caps (GstBaseSink * sink, GstCaps * filter)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (sink);

  GST_DEBUG_OBJECT (azuresink, "get_caps");

  return NULL;
}

// NOTE not included
/* notify subclass of new caps */
static gboolean
gst_azure_sink_set_caps (GstBaseSink * sink, GstCaps * caps)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (sink);

  GST_DEBUG_OBJECT (azuresink, "set_caps");

  return TRUE;
}

// NOTE not included
/* fixate sink caps during pull-mode negotiation */
static GstCaps *
gst_azure_sink_fixate (GstBaseSink * sink, GstCaps * caps)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (sink);

  GST_DEBUG_OBJECT (azuresink, "fixate");

  return NULL;
}

// not included
/* start or stop a pulling thread */
static gboolean
gst_azure_sink_activate_pull (GstBaseSink * sink, gboolean active)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (sink);

  GST_DEBUG_OBJECT (azuresink, "activate_pull");

  return TRUE;
}

// not included
/* get the start and end times for syncing on this buffer */
static void
gst_azure_sink_get_times (GstBaseSink * sink, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (sink);

  GST_DEBUG_OBJECT (azuresink, "get_times");

}

// not included
/* propose allocation parameters for upstream */
static gboolean
gst_azure_sink_propose_allocation (GstBaseSink * sink, GstQuery * query)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (sink);

  GST_DEBUG_OBJECT (azuresink, "propose_allocation");

  return TRUE;
}

/* start and stop processing, ideal for opening/closing the resource */
static gboolean
gst_azure_sink_start (GstBaseSink * sink)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (sink);

  GST_DEBUG_OBJECT (azuresink, "start");
  
  // examine configuration
  // if(azuresink->config == NULL)
  // {
  //   GST_ELEMENT_ERROR(sink, RESOURCE, NO_CONFIG,
  //     ("Missing configuration."), (NULL));
  //   return FALSE;
  // }
  if(GSTR_IS_EMPTY(azuresink->config.account_key) ||
  GSTR_IS_EMPTY(azuresink->config.account_name))
  {
    GST_ELEMENT_ERROR(sink, RESOURCE, NOT_AUTHORIZED,
      ("Missing account name or account key."), (NULL));
    return FALSE;
  }
  if(GSTR_IS_EMPTY(azuresink->config.container_name) ||
    GSTR_IS_EMPTY(azuresink->config.blob_name))
  {
    GST_ELEMENT_ERROR(sink, RESOURCE, NOT_FOUND,
      ("Missing contianer name or blob name, cannot determine destination."), (NULL));
    return FALSE;
  }

  if(azuresink->uploader == NULL)
  {
    // azuresink->uploader = gst_azure_sink_uploader_new(&azuresink->config);
    azuresink->uploader = gst_azure_sink_block_uploader_new(&azuresink->config);
  }

  gboolean init_success = gst_azure_uploader_init(azuresink->uploader, azuresink->config.container_name, azuresink->config.blob_name);
  if(!init_success)
  {
    GST_ELEMENT_ERROR(sink, RESOURCE, OPEN_WRITE,
      ("Failed to initialize uploader."), (NULL));
    return FALSE;
  }

  azuresink->total_bytes_written = 0;

  GST_DEBUG_OBJECT(azuresink, "started azure storage upload to %s %s",
    azuresink->config.container_name, azuresink->config.blob_name);
  return TRUE;
}

static gboolean
gst_azure_sink_stop (GstBaseSink * sink)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (sink);

  GST_DEBUG_OBJECT (azuresink, "stop");

  // gboolean flush_success = gst_azure_uploader_flush(azuresink->uploader);
  // if(!flush_success)
  // {
  //   GST_ELEMENT_ERROR(sink, RESOURCE, SYNC,
  //   ("Failed to flush content before stopping the stream."), (NULL));
  //   return FALSE;
  // }
  gboolean destroy_success = gst_azure_uploader_destroy(azuresink->uploader);
  if(!destroy_success)
  {
    GST_ELEMENT_ERROR(sink, RESOURCE, CLOSE,
    ("Failed to destroy uploader entry."), (NULL));
    return FALSE;
  }
  return TRUE;
}

/* unlock any pending access to the resource. subclasses should unlock
 * any function ASAP. */
static gboolean
gst_azure_sink_unlock (GstBaseSink * sink)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (sink);

  GST_DEBUG_OBJECT (azuresink, "unlock");

  return TRUE;
}

/* Clear a previously indicated unlock request not that unlocking is
 * complete. Sub-classes should clear any command queue or indicator they
 * set during unlock */
static gboolean
gst_azure_sink_unlock_stop (GstBaseSink * sink)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (sink);

  GST_DEBUG_OBJECT (azuresink, "unlock_stop");

  return TRUE;
}

/* notify subclass of query */
static gboolean
gst_azure_sink_query (GstBaseSink * sink, GstQuery * query)
{
  GST_INFO("Entering query");
  gboolean ret = FALSE;
  GstAzureSink *azuresink = GST_AZURE_SINK (sink);
  GST_INFO("Got azure sink");
  GST_DEBUG_OBJECT (azuresink, "query");

  switch(GST_QUERY_TYPE(query)) {
    case GST_QUERY_FORMATS:
      gst_query_set_formats(query, 2, GST_FORMAT_DEFAULT, GST_FORMAT_BYTES);
      ret = TRUE;
      break;
    case GST_QUERY_POSITION:
    {
      GstFormat format;
      gst_query_parse_position(query, &format, NULL);
      switch(format) {
        case GST_FORMAT_DEFAULT:
        case GST_FORMAT_BYTES:
          gst_query_set_position(query, GST_FORMAT_BYTES,
            azuresink->total_bytes_written);
          break;
        default:
          break;
      }
      break;
    }
    case GST_QUERY_SEEKING:
    {
      // this sink is not seekable
      GstFormat format;
      gst_query_parse_seeking(query, &format, NULL, NULL, NULL);
      gst_query_set_seeking(query, format, FALSE, 0, -1);
      ret = TRUE;
      break;
    }
    default:
      ret = GST_BASE_SINK_CLASS(gst_azure_sink_parent_class)->query(sink, query);
      break;
  }

  return ret;
}

/* notify subclass of event */
static gboolean
gst_azure_sink_event (GstBaseSink * sink, GstEvent * event)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (sink);

  GST_DEBUG_OBJECT (azuresink, "event");
  
  GstEventType type = GST_EVENT_TYPE(event);

  switch(type) {
    case GST_EVENT_EOS:
    {
      gboolean flush_success = gst_azure_uploader_flush(azuresink->uploader);
      if(!flush_success)
      {
        GST_ELEMENT_ERROR(sink, RESOURCE, SYNC,
        ("Failed to flush content before stopping the stream."), (NULL));
        return FALSE;
      }
    }
    default:
      break;
  }

  return GST_BASE_SINK_CLASS(gst_azure_sink_parent_class)->event(sink, event);
}

// we do not need this since we do not have multiple sinks
/* wait for eos or gap, subclasses should chain up to parent first */
static GstFlowReturn
gst_azure_sink_wait_event (GstBaseSink * sink, GstEvent * event)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (sink);

  GST_DEBUG_OBJECT (azuresink, "wait_event");

  return GST_FLOW_OK;
}

/* notify subclass of buffer or list before doing sync */
static GstFlowReturn
gst_azure_sink_prepare (GstBaseSink * sink, GstBuffer * buffer)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (sink);

  GST_DEBUG_OBJECT (azuresink, "prepare");

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_azure_sink_prepare_list (GstBaseSink * sink, GstBufferList * buffer_list)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (sink);

  GST_DEBUG_OBJECT (azuresink, "prepare_list");

  return GST_FLOW_OK;
}

/* notify subclass of preroll buffer or real buffer */
static GstFlowReturn
gst_azure_sink_preroll (GstBaseSink * sink, GstBuffer * buffer)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (sink);

  GST_DEBUG_OBJECT (azuresink, "preroll");

  return GST_FLOW_OK;
}

static gboolean
gst_azure_sink_read_buffer(GstAzureSink *sink, GstBuffer *buffer)
{
  GstMapInfo map_info = GST_MAP_INFO_INIT;
  
  if(!gst_buffer_map(buffer, &map_info, GST_MAP_READ))
  {
    GST_ELEMENT_ERROR (sink, RESOURCE, WRITE,
        ("Failed to map the buffer."), (NULL));
    return FALSE;
  }

  // copy all of them to sstream
  if(!gst_azure_uploader_upload(sink->uploader, (const gchar *)map_info.data, map_info.size))
  {
    GST_ELEMENT_ERROR(sink, RESOURCE, WRITE,
      ("Failed to append to uploader's buffer."), (NULL));
    gst_buffer_unmap(buffer, &map_info);
    return FALSE;
  }
  sink->total_bytes_written += map_info.size;
  gst_buffer_unmap(buffer, &map_info);
  return TRUE;
}

static GstFlowReturn
gst_azure_sink_render (GstBaseSink * sink, GstBuffer * buffer)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (sink);
  GstFlowReturn ret;
  guint8 n_mem;

  GST_DEBUG_OBJECT (azuresink, "render");

  n_mem = gst_buffer_n_memory(buffer);

  if(n_mem > 0) {
    if(gst_azure_sink_read_buffer(azuresink, buffer))
      ret = GST_FLOW_OK;
    else
      ret = GST_FLOW_ERROR;
  } else {
    ret = GST_FLOW_OK;
  }

  return ret;
}

// not included
/* Render a BufferList */
// TODO maybe implement this
static GstFlowReturn
gst_azure_sink_render_list (GstBaseSink * sink, GstBufferList * buffer_list)
{
  GstAzureSink *azuresink = GST_AZURE_SINK (sink);

  GST_DEBUG_OBJECT (azuresink, "render_list");

  return GST_FLOW_OK;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "azuresink", GST_RANK_NONE,
      GST_TYPE_AZURE_SINK);
}

/* These are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "1.0"
#endif
#ifndef PACKAGE
#define PACKAGE "Gstreamer Azure Storage Package"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "Gstreamer Azure Storage Package"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "https://www.azure.com/"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    azureelements,
    "Azure Storage Elements",
    plugin_init,
    VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)