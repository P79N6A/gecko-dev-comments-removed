






































#include <atk/atk.h>
#include "AtkSocketAccessible.h"
#include "nsMai.h"
#include "nsMaiInterfaceComponent.h"

void (*AtkSocketAccessible::g_atk_socket_embed) (AtkSocket*, gchar*) = NULL;
GType AtkSocketAccessible::g_atk_socket_type = G_TYPE_INVALID;
const char* AtkSocketAccessible::sATKSocketEmbedSymbol = "atk_socket_embed";
const char* AtkSocketAccessible::sATKSocketGetTypeSymbol = "atk_socket_get_type";

bool AtkSocketAccessible::gCanEmbed = FALSE;



#define MAI_TYPE_ATK_SOCKET              (mai_atk_socket_get_type ())
#define MAI_ATK_SOCKET(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                          MAI_TYPE_ATK_SOCKET, MaiAtkSocket))
#define MAI_IS_ATK_SOCKET(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                          MAI_TYPE_ATK_SOCKET))
#define MAI_ATK_SOCKET_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass),\
                                          MAI_TYPE_ATK_SOCKET,\
                                          MaiAtkSocketClass))
#define MAI_IS_ATK_SOCKET_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass),\
                                          MAI_TYPE_ATK_SOCKET))
#define MAI_ATK_SOCKET_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj),\
                                          MAI_TYPE_ATK_SOCKET,\
                                          MaiAtkSocketClass))

typedef struct _MaiAtkSocket             MaiAtkSocket;
typedef struct _MaiAtkSocketClass        MaiAtkSocketClass;

struct _MaiAtkSocket
{
  AtkSocket parent;

  nsAccessibleWrap* accWrap;
};

struct _MaiAtkSocketClass
{
  AtkSocketClass parent_class;
};

G_BEGIN_DECLS

GType mai_atk_socket_get_type(void);
AtkObject* mai_atk_socket_new(nsAccessibleWrap* aAccWrap);

void mai_atk_component_iface_init(AtkComponentIface* aIface);
AtkObject* mai_atk_socket_ref_accessible_at_point(AtkComponent *aComponent,
                                                  gint aAccX,
                                                  gint aAccY,
                                                  AtkCoordType aCoordType);
void mai_atk_socket_get_extents(AtkComponent* aComponent,
                                gint* aAccX,
                                gint* aAccY,
                                gint* aAccWidth,
                                gint* aAccHeight,
                                AtkCoordType aCoordType);

G_END_DECLS

G_DEFINE_TYPE_EXTENDED(MaiAtkSocket, mai_atk_socket,
                       AtkSocketAccessible::g_atk_socket_type, 0,
                       G_IMPLEMENT_INTERFACE(ATK_TYPE_COMPONENT,
                                             mai_atk_component_iface_init))

void
mai_atk_socket_class_init(MaiAtkSocketClass* aAcc)
{
}

void
mai_atk_socket_init(MaiAtkSocket* aAcc)
{
}

AtkObject*
mai_atk_socket_new(nsAccessibleWrap* aAccWrap)
{
  NS_ENSURE_TRUE(aAccWrap, NULL);

  MaiAtkSocket* acc = nsnull;
  acc = static_cast<MaiAtkSocket*>(g_object_new(MAI_TYPE_ATK_SOCKET, NULL));
  NS_ENSURE_TRUE(acc, NULL);

  acc->accWrap = aAccWrap;
  return ATK_OBJECT(acc);
}

void
mai_atk_component_iface_init(AtkComponentIface* aIface)
{
  NS_ASSERTION(aIface, "Invalid Interface");

  aIface->ref_accessible_at_point = mai_atk_socket_ref_accessible_at_point;
  aIface->get_extents = mai_atk_socket_get_extents;
}

AtkObject*
mai_atk_socket_ref_accessible_at_point(AtkComponent* aComponent,
                                       gint aX, gint aY,
                                       AtkCoordType aCoordType)
{
  NS_ENSURE_TRUE(MAI_IS_ATK_SOCKET(aComponent), nsnull);

  return refAccessibleAtPointHelper(MAI_ATK_SOCKET(aComponent)->accWrap,
                                    aX, aY, aCoordType);
}

void
mai_atk_socket_get_extents(AtkComponent* aComponent,
                           gint* aX, gint* aY, gint* aWidth, gint* aHeight,
                           AtkCoordType aCoordType)
{
  *aX = *aY = *aWidth = *aHeight = 0;

  if (!MAI_IS_ATK_SOCKET(aComponent))
    return;

  getExtentsHelper(MAI_ATK_SOCKET(aComponent)->accWrap,
                   aX, aY, aWidth, aHeight, aCoordType);
}

AtkSocketAccessible::AtkSocketAccessible(nsIContent* aContent,
                                         nsIWeakReference* aShell,
                                         const nsCString& aPlugId) :
  nsAccessibleWrap(aContent, aShell)
{
  mAtkObject = mai_atk_socket_new(this);
  if (!mAtkObject)
    return;

  
  
  
  
  if (gCanEmbed && G_TYPE_CHECK_INSTANCE_TYPE(mAtkObject, g_atk_socket_type) &&
      !aPlugId.IsVoid()) {
    AtkSocket* accSocket =
      G_TYPE_CHECK_INSTANCE_CAST(mAtkObject, g_atk_socket_type, AtkSocket);
    g_atk_socket_embed(accSocket, (gchar*)aPlugId.get());
  }
}

NS_IMETHODIMP
AtkSocketAccessible::GetNativeInterface(void** aOutAccessible)
{
  *aOutAccessible = mAtkObject;
  return NS_OK;
}

void
AtkSocketAccessible::Shutdown()
{
  if (mAtkObject) {
    if (MAI_IS_ATK_SOCKET(mAtkObject))
      MAI_ATK_SOCKET(mAtkObject)->accWrap = nsnull;
    g_object_unref(mAtkObject);
    mAtkObject = nsnull;
  }
  nsAccessibleWrap::Shutdown();
}
