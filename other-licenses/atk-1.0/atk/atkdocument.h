


















#ifndef __ATK_DOCUMENT_H__
#define __ATK_DOCUMENT_H__

#include <atk/atkobject.h>
#include <atk/atkutil.h>

#ifdef __cplusplus
extern "C" {
#endif







#define ATK_TYPE_DOCUMENT                   (atk_document_get_type ())
#define ATK_IS_DOCUMENT(obj)                G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATK_TYPE_DOCUMENT)
#define ATK_DOCUMENT(obj)                   G_TYPE_CHECK_INSTANCE_CAST ((obj), ATK_TYPE_DOCUMENT, AtkDocument)
#define ATK_DOCUMENT_GET_IFACE(obj)         (G_TYPE_INSTANCE_GET_INTERFACE ((obj), ATK_TYPE_DOCUMENT, AtkDocumentIface))

#ifndef _TYPEDEF_ATK_DOCUMENT_
#define _TYPEDEF_ATK_DOCUMENT_
typedef struct _AtkDocument AtkDocument;
#endif
typedef struct _AtkDocumentIface AtkDocumentIface;

struct _AtkDocumentIface
{
  GTypeInterface parent;
  G_CONST_RETURN gchar* ( *get_document_type) (AtkDocument              *document);
  gpointer              ( *get_document)      (AtkDocument              *document);

  G_CONST_RETURN gchar* ( *get_document_locale) (AtkDocument              *document);
  AtkAttributeSet *     ( *get_document_attributes) (AtkDocument        *document);
  G_CONST_RETURN gchar* ( *get_document_attribute_value) (AtkDocument   *document,
                                                          const gchar   *attribute_name);
  gboolean              ( *set_document_attribute) (AtkDocument         *document,
                                                    const gchar         *attribute_name,
                                                    const gchar         *attribute_value);
  AtkFunction pad1;
  AtkFunction pad2;
  AtkFunction pad3;
  AtkFunction pad4;
};

GType  atk_document_get_type             (void);

G_CONST_RETURN gchar* atk_document_get_document_type (AtkDocument   *document);
gpointer atk_document_get_document (AtkDocument   *document);
G_CONST_RETURN gchar* atk_document_get_locale (AtkDocument *document);
AtkAttributeSet*      atk_document_get_attributes (AtkDocument *document);
G_CONST_RETURN gchar* atk_document_get_attribute_value (AtkDocument *document, 
                                                        const gchar *attribute_name);
gboolean              atk_document_set_attribute_value (AtkDocument *document,
                                                        const gchar *attribute_name,
                                                        const gchar *attribute_value);
#ifdef __cplusplus
}
#endif 
#endif
