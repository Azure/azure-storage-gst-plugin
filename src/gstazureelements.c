#include <gst/gst.h>
#include "gstazuresink.h"
#include "gstazuresrc.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  if(!gst_element_register (plugin, "azuresink", GST_RANK_NONE, GST_TYPE_AZURE_SINK))
    return FALSE;
  return gst_element_register(plugin, "azuresrc", GST_RANK_NONE, GST_TYPE_AZURE_SRC);
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