#ifndef _AZURE_ELEMENTS_UTILS_GST_UTILS_H_
#define _AZURE_ELEMENTS_UTILS_GST_UTILS_H_

#include <gst/gst.h>

G_BEGIN_DECLS

static inline void gst_azure_elements_set_string_property(void *obj, const GValue *value,
    gchar **property, const gchar *property_name)
{
    const gchar *v = g_value_get_string(value);
    g_free(*property);

    if(v != NULL) {
        *property = g_strdup(v);
        GST_INFO_OBJECT(obj, "Setting property %s: %s", property_name, *property);
    } else {
        GST_WARNING_OBJECT(obj, "Resetting property %s...\n", property_name);
        *property = NULL;
    }
}

#define DEFINE_SET_PROPERTY(__type, __format) \
static inline void gst_azure_elements_set_ ## __type ## _property (\
  void *obj, const GValue *value, g ## __type *property, const gchar *property_name)\
{\
  if(value != NULL) {\
    *property = g_value_get_ ## __type (value);\
    GST_INFO_OBJECT(obj, "Setting property %s to " __format "\n", property_name, *property);\
  } else\
    GST_WARNING_OBJECT(obj, "Attempting to set property %s to invalid value.", property_name);\
}

DEFINE_SET_PROPERTY(boolean, "%d")
DEFINE_SET_PROPERTY(uint64, "%lu")
DEFINE_SET_PROPERTY(int64, "%ld")
DEFINE_SET_PROPERTY(uint, "%u")

// some handy functions
#define GSTR_IS_EMPTY(_gstr) ((_gstr) == NULL || *(_gstr) == '\0')

G_END_DECLS
#endif