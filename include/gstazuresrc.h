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

#ifndef _AZURE_ELEMENTS_GST_AZURE_SRC_H_
#define _AZURE_ELEMENTS_GST_AZURE_SRC_H_

#include <gst/base/gstbasesrc.h>
#include "gstazuredownloader.h"
#include "gstazuresrcconfig.h"

G_BEGIN_DECLS

#define GST_TYPE_AZURE_SRC   (gst_azure_src_get_type())
#define GST_AZURE_SRC(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AZURE_SRC,GstAzureSrc))
#define GST_AZURE_SRC_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_AZURE_SRC,GstAzureSrcClass))
#define GST_IS_AZURE_SRC(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_AZURE_SRC))
#define GST_IS_AZURE_SRC_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_AZURE_SRC))

typedef struct _GstAzureSrc GstAzureSrc;
typedef struct _GstAzureSrcClass GstAzureSrcClass;

struct _GstAzureSrc
{
  GstBaseSrc base_azuresrc;
  GstAzureDownloader *downloader;
  GstAzureSrcConfig config;

  gsize read_position;
};

struct _GstAzureSrcClass
{
  GstBaseSrcClass base_azuresrc_class;
};

GType gst_azure_src_get_type (void);

G_END_DECLS

#endif
