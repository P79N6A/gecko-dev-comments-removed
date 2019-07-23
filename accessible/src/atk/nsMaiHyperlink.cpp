







































#include "nsIURI.h"
#include "nsMaiHyperlink.h"



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
    gchar *uri;
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

static gpointer parent_class = NULL;
static nsIAccessibleHyperLink *
get_accessible_hyperlink(AtkHyperlink *aHyperlink);

GType
mai_atk_hyperlink_get_type(void)
{
    static GType type = 0;

    if (!type) {
        static const GTypeInfo tinfo = {
            sizeof(MaiAtkHyperlinkClass),
            (GBaseInitFunc)NULL,
            (GBaseFinalizeFunc)NULL,
            (GClassInitFunc)classInitCB,
            (GClassFinalizeFunc)NULL,
            NULL, 
            sizeof(MaiAtkHyperlink), 
            0, 
            (GInstanceInitFunc)NULL,
            NULL 
        };

        type = g_type_register_static(ATK_TYPE_HYPERLINK,
                                      "MaiAtkHyperlink",
                                      &tinfo, GTypeFlags(0));
    }
    return type;
}

MaiHyperlink::MaiHyperlink(nsIAccessibleHyperLink *aAcc):
    mHyperlink(aAcc),
    mMaiAtkHyperlink(nsnull)
{
}

MaiHyperlink::~MaiHyperlink()
{
    if (mMaiAtkHyperlink) {
        MAI_ATK_HYPERLINK(mMaiAtkHyperlink)->maiHyperlink = nsnull;
        g_object_unref(mMaiAtkHyperlink);
    }
}

AtkHyperlink *
MaiHyperlink::GetAtkHyperlink(void)
{
    NS_ENSURE_TRUE(mHyperlink, nsnull);

    if (mMaiAtkHyperlink)
        return mMaiAtkHyperlink;

    nsCOMPtr<nsIAccessibleHyperLink> accessIf(do_QueryInterface(mHyperlink));
    if (!accessIf)
        return nsnull;

    mMaiAtkHyperlink =
        NS_REINTERPRET_CAST(AtkHyperlink *,
                            g_object_new(mai_atk_hyperlink_get_type(), NULL));
    NS_ASSERTION(mMaiAtkHyperlink, "OUT OF MEMORY");
    NS_ENSURE_TRUE(mMaiAtkHyperlink, nsnull);

    
    MaiHyperlink::Initialize(mMaiAtkHyperlink, this);

    return mMaiAtkHyperlink;
}







nsresult
MaiHyperlink::Initialize(AtkHyperlink *aObj, MaiHyperlink *aHyperlink)
{
    NS_ENSURE_ARG(MAI_IS_ATK_HYPERLINK(aObj));
    NS_ENSURE_ARG(aHyperlink);

    
    MAI_ATK_HYPERLINK(aObj)->maiHyperlink = aHyperlink;
    MAI_ATK_HYPERLINK(aObj)->uri = nsnull;
    return NS_OK;
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
    if (maiAtkHyperlink->uri)
        g_free(maiAtkHyperlink->uri);
    maiAtkHyperlink->maiHyperlink = nsnull;

    
    if (G_OBJECT_CLASS (parent_class)->finalize)
        G_OBJECT_CLASS (parent_class)->finalize(aObj);
}

gchar *
getUriCB(AtkHyperlink *aLink, gint aLinkIndex)
{
    nsIAccessibleHyperLink *accHyperlink = get_accessible_hyperlink(aLink);
    NS_ENSURE_TRUE(accHyperlink, nsnull);

    MaiAtkHyperlink *maiAtkHyperlink = MAI_ATK_HYPERLINK(aLink);
    if (maiAtkHyperlink->uri)
        return g_strdup(maiAtkHyperlink->uri);

    nsCOMPtr<nsIURI> uri;
    nsresult rv = accHyperlink->GetURI(aLinkIndex,getter_AddRefs(uri));
    if (NS_FAILED(rv) || !uri)
        return nsnull;
    nsCAutoString cautoStr;
    rv = uri->GetSpec(cautoStr);

    maiAtkHyperlink->uri = ToNewCString(cautoStr);
    return g_strdup(maiAtkHyperlink->uri);
}

AtkObject *
getObjectCB(AtkHyperlink *aLink, gint aLinkIndex)
{
    nsIAccessibleHyperLink *accHyperlink = get_accessible_hyperlink(aLink);
    NS_ENSURE_TRUE(accHyperlink, nsnull);

    nsCOMPtr<nsIAccessible> accObj;
    accHyperlink->GetObject(aLinkIndex, getter_AddRefs(accObj));
    NS_ENSURE_TRUE(accObj, nsnull);

    void *atkObj = nsnull;
    accObj->GetNativeInterface(&atkObj);
    if (!atkObj) {
        return nsnull;
    }
    
    return ATK_OBJECT(atkObj);
}

gint
getEndIndexCB(AtkHyperlink *aLink)
{
    nsIAccessibleHyperLink *accHyperlink = get_accessible_hyperlink(aLink);
    NS_ENSURE_TRUE(accHyperlink, -1);

    PRInt32 endIndex = -1;
    nsresult rv = accHyperlink->GetEndIndex(&endIndex);

    return (NS_FAILED(rv)) ? -1 : NS_STATIC_CAST(gint, endIndex);
}

gint
getStartIndexCB(AtkHyperlink *aLink)
{
    nsIAccessibleHyperLink *accHyperlink = get_accessible_hyperlink(aLink);
    NS_ENSURE_TRUE(accHyperlink, -1);

    PRInt32 startIndex = -1;
    nsresult rv = accHyperlink->GetStartIndex(&startIndex);

    return (NS_FAILED(rv)) ? -1 : NS_STATIC_CAST(gint, startIndex);
}

gboolean
isValidCB(AtkHyperlink *aLink)
{
    nsIAccessibleHyperLink *accHyperlink = get_accessible_hyperlink(aLink);
    NS_ENSURE_TRUE(accHyperlink, FALSE);

    PRBool isValid = PR_FALSE;
    nsresult rv = accHyperlink->IsValid(&isValid);
    return (NS_FAILED(rv)) ? FALSE : NS_STATIC_CAST(gboolean, isValid);
}

gint
getAnchorCountCB(AtkHyperlink *aLink)
{
    nsIAccessibleHyperLink *accHyperlink = get_accessible_hyperlink(aLink);
    NS_ENSURE_TRUE(accHyperlink, -1);

    PRInt32 count = -1;
    nsresult rv = accHyperlink->GetAnchors(&count);
    return (NS_FAILED(rv)) ? -1 : NS_STATIC_CAST(gint, count);
}



nsIAccessibleHyperLink *
get_accessible_hyperlink(AtkHyperlink *aHyperlink)
{
    NS_ENSURE_TRUE(MAI_IS_ATK_HYPERLINK(aHyperlink), nsnull);
    MaiHyperlink * maiHyperlink =
        MAI_ATK_HYPERLINK(aHyperlink)->maiHyperlink;
    NS_ENSURE_TRUE(maiHyperlink != nsnull, nsnull);
    NS_ENSURE_TRUE(maiHyperlink->GetAtkHyperlink() == aHyperlink, nsnull);
    return maiHyperlink->GetAccHyperlink();
}
