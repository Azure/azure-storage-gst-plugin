/* GStreamer
 * Copyright (C) 2020 FIXME <fixme@example.com>
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
 * The azuresrc element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! azuresrc ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include "gstazuresrc.h"
#include "gstazuresrcconfig.h"

GST_DEBUG_CATEGORY_STATIC (gst_azuresrc_debug_category);
#define GST_CAT_DEFAULT gst_azuresrc_debug_category

/* prototypes */


static void gst_azuresrc_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_azuresrc_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_azuresrc_dispose (GObject * object);
static void gst_azuresrc_finalize (GObject * object);


enum
{
  PROP_0,
  PROP_BLOB_ENDPOINT,
  PROP_ACCOUNT_NAME,
  PROP_ACCOUNT_KEY,
  PROP_LOCATION,
  PROP_USE_HTTPS,
  PROP_WORKER_COUNT
};

/* pad templates */


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstAzureSrc, gst_azuresrc, GST_TYPE_BASE_SRC,
  GST_DEBUG_CATEGORY_INIT (gst_azuresrc_debug_category, "azuresrc", 0,
  "debug category for azuresrc element"));

static void
gst_azuresrc_class_init (GstAzureSrcClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
      "Azure storage source",
      "Generic",
      "Read blob from azure blob storage.",
      "Eugene Chen<t-yijunc@microsoft.com>");

  gobject_class->set_property = gst_azuresrc_set_property;
  gobject_class->get_property = gst_azuresrc_get_property;
  gobject_class->dispose = gst_azuresrc_dispose;
  gobject_class->finalize = gst_azuresrc_finalize;

  g_object_class_install_property (gobject_class, PROP_ACCOUNT_NAME,
    g_param_spec_string ("account-name", "azure account name",
      "Your azure storage account.", NULL,
      (G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_ACCOUNT_KEY,
    g_param_spec_string ("account-key", "azure account key",
      "Your azure storage account key.", NULL,
      (G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_LOCATION,
    g_param_spec_string ("location", "azure blob storage blob location",
      "The blob you want to read from in container-name/blob-name format.", NULL,
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
  
  g_object_class_install_property (gobject_class, PROP_WORKER_COUNT,
    g_param_spec_uint("worker-count", "azure storage worker count",
      "The number of concurrent block uploaders. 4 by default.",
      1, 64, AZURE_SINK_DEFAULT_WORKER_COUNT, G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY | G_PARAM_STATIC_STRINGS));
  
}

static void
gst_azuresrc_init (GstAzureSrc *azuresrc)
{
  azuresrc->config = AZURE_SRC_DEFAULT_CONFIG;
  azuresrc->downloader = NULL;
  azuresrc->total_bytes_read = 0;
  GST_DEBUG_OBJECT(azuresrc, "init");
  /* NOTE pads are configured here with gst_pad_set_*_function () */
  // gst_element_add_pad(GST_ELEMENT(azuresink), azuresink->sinkpad);
}

void
gst_azuresrc_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAzureSrc *azuresrc = GST_AZURE_SRC (object);

  GST_DEBUG_OBJECT (azuresrc, "set_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_azuresrc_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstAzureSrc *azuresrc = GST_AZURE_SRC (object);

  GST_DEBUG_OBJECT (azuresrc, "get_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_azuresrc_dispose (GObject * object)
{
  GstAzureSrc *azuresrc = GST_AZURE_SRC (object);

  GST_DEBUG_OBJECT (azuresrc, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_azuresrc_parent_class)->dispose (object);
}

void
gst_azuresrc_finalize (GObject * object)
{
  GstAzureSrc *azuresrc = GST_AZURE_SRC (object);

  GST_DEBUG_OBJECT (azuresrc, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_azuresrc_parent_class)->finalize (object);
}