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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _AZURE_ELEMENTS_GST_AZURE_SINK_H_
#define _AZURE_ELEMENTS_GST_AZURE_SINK_H_
#include <gst/base/gstbasesink.h>
#include "gstazureuploader.h"
#include "gstazuresinkconfig.h"

G_BEGIN_DECLS

#define GST_TYPE_AZURE_SINK   (gst_azure_sink_get_type())
#define GST_AZURE_SINK(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AZURE_SINK,GstAzureSink))
#define GST_AZURE_SINK_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_AZURE_SINK,GstAzureSinkClass))
#define GST_IS_AZURE_SINK(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_AZURE_SINK))
#define GST_IS_AZURE_SINK_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_AZURE_SINK))

typedef struct _GstAzureSink GstAzureSink;
typedef struct _GstAzureSinkClass GstAzureSinkClass;

struct _GstAzureSink
{
  GstBaseSink base_azuresink;
  // GstPad *sinkpad;
  GstAzureUploader *uploader;
  GstAzureSinkConfig config;

  gsize total_bytes_written;
};

struct _GstAzureSinkClass
{
  GstBaseSinkClass base_azuresink_class;
};

GType gst_azure_sink_get_type (void);

G_END_DECLS

#endif
