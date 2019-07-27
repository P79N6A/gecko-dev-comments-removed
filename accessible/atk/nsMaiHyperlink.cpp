





#include "nsIURI.h"
#include "nsMaiHyperlink.h"

using namespace mozilla::a11y;



#define MAI_TYPE_ATK_HYPERLINK      (mai_atk_hyperlink_get_type ())
#define MAI_ATK_HYPERLINK(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                     MAI_TYPE_ATK_HYPERLINK, MaiAtkHyperlink))
#define MAI_ATK_HYPERLINK_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),\
                                 MAI_TYPE_ATK_HYPERLINK, MaiAtkHyperlinkClass))
#define MAI_IS_ATK_HYPERLINK(obj)      (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                        MAI_TYPE_ATK_HYPERLINK))
#define MAI_IS_ATK_HYPERLINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
                                        MAI_TYPE_ATK_HYPERLINK))
#define MAI_ATK_HYPERLINK_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
                                 MAI_TYPE_ATK_HYPERLINK, MaiAtkHyperlinkClass))






struct MaiAtkHyperlink
{
    AtkHyperlink parent;

    



    MaiHyperlink *maiHyperlink;
};

struct MaiAtkHyperlinkClass
{
    AtkHyperlinkClass parent_class;
};

GType mai_atk_hyperlink_get_type(void);

G_BEGIN_DECLS

static void classInitCB(AtkHyperlinkClass *aClass);
static void finalizeCB(GObject *aObj);


static gchar *getUriCB(AtkHyperlink *aLink, gint aLinkIndex);
static AtkObject *getObjectCB(AtkHyperlink *aLink, gint aLinkIndex);
static gint getEndIndexCB(AtkHyperlink *aLink);
static gint getStartIndexCB(AtkHyperlink *aLink);
static gboolean isValidCB(AtkHyperlink *aLink);
static gint getAnchorCountCB(AtkHyperlink *aLink);
G_END_DECLS

static gpointer parent_class = nullptr;
static Accessible*
get_accessible_hyperlink(AtkHyperlink *aHyperlink);

GType
mai_atk_hyperlink_get_type(void)
{
    static GType type = 0;

    if (!type) {
        static const GTypeInfo tinfo = {
            sizeof(MaiAtkHyperlinkClass),
            (GBaseInitFunc)nullptr,
            (GBaseFinalizeFunc)nullptr,
            (GClassInitFunc)classInitCB,
            (GClassFinalizeFunc)nullptr,
            nullptr, 
            sizeof(MaiAtkHyperlink), 
            0, 
            (GInstanceInitFunc)nullptr,
            nullptr 
        };

        type = g_type_register_static(ATK_TYPE_HYPERLINK,
                                      "MaiAtkHyperlink",
                                      &tinfo, GTypeFlags(0));
    }
    return type;
}

MaiHyperlink::MaiHyperlink(Accessible* aHyperLink) :
    mHyperlink(aHyperLink),
    mMaiAtkHyperlink(nullptr)
{
}

MaiHyperlink::~MaiHyperlink()
{
    if (mMaiAtkHyperlink) {
        MAI_ATK_HYPERLINK(mMaiAtkHyperlink)->maiHyperlink = nullptr;
        g_object_unref(mMaiAtkHyperlink);
    }
}

AtkHyperlink*
MaiHyperlink::GetAtkHyperlink(void)
{
  NS_ENSURE_TRUE(mHyperlink, nullptr);

  if (mMaiAtkHyperlink)
    return mMaiAtkHyperlink;

  if (!mHyperlink->IsLink())
    return nullptr;

    mMaiAtkHyperlink =
        reinterpret_cast<AtkHyperlink *>
                        (g_object_new(mai_atk_hyperlink_get_type(), nullptr));
    NS_ASSERTION(mMaiAtkHyperlink, "OUT OF MEMORY");
    NS_ENSURE_TRUE(mMaiAtkHyperlink, nullptr);

    MAI_ATK_HYPERLINK(mMaiAtkHyperlink)->maiHyperlink = this;

    return mMaiAtkHyperlink;
}




void
classInitCB(AtkHyperlinkClass *aClass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(aClass);

    parent_class = g_type_class_peek_parent(aClass);

    aClass->get_uri = getUriCB;
    aClass->get_object = getObjectCB;
    aClass->get_end_index = getEndIndexCB;
    aClass->get_start_index = getStartIndexCB;
    aClass->is_valid = isValidCB;
    aClass->get_n_anchors = getAnchorCountCB;

    gobject_class->finalize = finalizeCB;
}

void
finalizeCB(GObject *aObj)
{
    NS_ASSERTION(MAI_IS_ATK_HYPERLINK(aObj), "Invalid MaiAtkHyperlink");
    if (!MAI_IS_ATK_HYPERLINK(aObj))
        return;

    MaiAtkHyperlink *maiAtkHyperlink = MAI_ATK_HYPERLINK(aObj);
    maiAtkHyperlink->maiHyperlink = nullptr;

    
    if (G_OBJECT_CLASS (parent_class)->finalize)
        G_OBJECT_CLASS (parent_class)->finalize(aObj);
}

gchar *
getUriCB(AtkHyperlink *aLink, gint aLinkIndex)
{
    Accessible* hyperlink = get_accessible_hyperlink(aLink);
    NS_ENSURE_TRUE(hyperlink, nullptr);

    nsCOMPtr<nsIURI> uri = hyperlink->AnchorURIAt(aLinkIndex);
    if (!uri)
        return nullptr;

    nsAutoCString cautoStr;
    nsresult rv = uri->GetSpec(cautoStr);
    NS_ENSURE_SUCCESS(rv, nullptr);

    return g_strdup(cautoStr.get());
}

AtkObject *
getObjectCB(AtkHyperlink *aLink, gint aLinkIndex)
{
    Accessible* hyperlink = get_accessible_hyperlink(aLink);
    NS_ENSURE_TRUE(hyperlink, nullptr);

    Accessible* anchor = hyperlink->AnchorAt(aLinkIndex);
    NS_ENSURE_TRUE(anchor, nullptr);

    AtkObject* atkObj = AccessibleWrap::GetAtkObject(anchor);
    
    return atkObj;
}

gint
getEndIndexCB(AtkHyperlink *aLink)
{
    Accessible* hyperlink = get_accessible_hyperlink(aLink);
    NS_ENSURE_TRUE(hyperlink, -1);

    return static_cast<gint>(hyperlink->EndOffset());
}

gint
getStartIndexCB(AtkHyperlink *aLink)
{
    Accessible* hyperlink = get_accessible_hyperlink(aLink);
    NS_ENSURE_TRUE(hyperlink, -1);

    return static_cast<gint>(hyperlink->StartOffset());
}

gboolean
isValidCB(AtkHyperlink *aLink)
{
    Accessible* hyperlink = get_accessible_hyperlink(aLink);
    NS_ENSURE_TRUE(hyperlink, FALSE);

    return static_cast<gboolean>(hyperlink->IsLinkValid());
}

gint
getAnchorCountCB(AtkHyperlink *aLink)
{
    Accessible* hyperlink = get_accessible_hyperlink(aLink);
    NS_ENSURE_TRUE(hyperlink, -1);

    return static_cast<gint>(hyperlink->AnchorCount());
}



Accessible*
get_accessible_hyperlink(AtkHyperlink *aHyperlink)
{
    NS_ENSURE_TRUE(MAI_IS_ATK_HYPERLINK(aHyperlink), nullptr);
    MaiHyperlink * maiHyperlink =
        MAI_ATK_HYPERLINK(aHyperlink)->maiHyperlink;
    NS_ENSURE_TRUE(maiHyperlink != nullptr, nullptr);
    NS_ENSURE_TRUE(maiHyperlink->GetAtkHyperlink() == aHyperlink, nullptr);
    return maiHyperlink->GetAccHyperlink();
}
