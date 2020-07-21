#ifndef _AZURE_SINK_GST_UTILS_H_
#define _AZURE_SINK_GST_UTILS_H_

#include <gst/gst.h>

G_BEGIN_DECLS

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

void gst_azure_sink_set_uint64_property(GstAzureSink *sink, const GValue *value,
    guint64 *property, const gchar *property_name)
{
    if(value != NULL) {
        *property = g_value_get_uint(value);
        GST_INFO_OBJECT(sink, "Setting property %s: %lu", property_name, *property);
    } else {
        GST_WARNING_OBJECT(sink, "Attempted to set property %s to invalid value, skipped.", property_name);
    }
}

void gst_azure_sink_set_boolean_property(GstAzureSink *sink, const GValue *value,
    gboolean *property, const gchar *property_name)
{
    if(value != NULL)
    {
        *property = g_value_get_boolean(value);
        GST_INFO_OBJECT(sink, "Settin property %s: %d", property_name, (int)(*property));
    } else {
        GST_WARNING_OBJECT(sink, "Attempting to set property %s to invalid value, skipped.", property_name);
    }
}
// some handy functions
#define GSTR_IS_EMPTY(_gstr) ((_gstr) == NULL || *(_gstr) == '\0')

G_END_DECLS
#endif