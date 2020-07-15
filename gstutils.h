#ifndef _GST_AZURE_SINK_UTILS_H_
#define _GST_AZURE_SINK_UTILS_H_

#include "gstazuresink.h"

void gst_azure_sink_set_string_property(GstAzureSink *sink, const GValue *value,
    gchar **property, const gchar *property_name)
{
    const gchar *v = g_value_get_string(value);
    g_free(*property);

    if(v != NULL) {
        *property = g_strdup(v);
        GST_INFO_OBJECT(sink, "Setting property %s: %s", property_name, *property);
    } else {
        *property = NULL;
    }
}

void gst_azure_sink_set_uint_property(GstAzureSink *sink, const GValue *value,
    guint *property, const gchar *property_name)
{
    if(value != NULL) {
        *property = g_value_get_uint(value);
        GST_INFO_OBJECT(sink, "Setting property %s: %u", property_name, *property);
    } else {
        GST_WARNING_OBJECT(sink, "Attempted to set property %s to invalid value, skipped.", property_name);
    }
}


// some handy functions
#define GSTR_IS_EMPTY(_gstr) ((_gstr) == NULL || *(_gstr) == '\0')

#endif