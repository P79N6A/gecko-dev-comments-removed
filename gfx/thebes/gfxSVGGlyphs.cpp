



#include "gfxSVGGlyphs.h"

#include "nscore.h"
#include "nsError.h"
#include "nsAutoPtr.h"
#include "nsIParser.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNodeList.h"
#include "nsString.h"
#include "nsIDocument.h"
#include "nsICategoryManager.h"
#include "nsIDocumentLoaderFactory.h"
#include "nsIContentViewer.h"
#include "nsIStreamListener.h"
#include "nsServiceManagerUtils.h"
#include "nsIPresShell.h"
#include "nsQueryFrame.h"
#include "nsIContentSink.h"
#include "nsXMLContentSink.h"
#include "nsNetUtil.h"
#include "nsIInputStream.h"
#include "nsStringStream.h"
#include "nsStreamUtils.h"
#include "nsIPrincipal.h"
#include "mozilla/dom/Element.h"
#include "nsSVGUtils.h"
#include "nsIScriptSecurityManager.h"
#include "nsHostObjectProtocolHandler.h"
#include "nsContentUtils.h"
#include "harfbuzz/hb.h"

#define SVG_CONTENT_TYPE NS_LITERAL_CSTRING("image/svg+xml")
#define UTF8_CHARSET NS_LITERAL_CSTRING("utf-8")

typedef mozilla::dom::Element Element;

mozilla::gfx::UserDataKey gfxTextObjectPaint::sUserDataKey;

const float gfxSVGGlyphs::SVG_UNITS_PER_EM = 1000.0f;

const gfxRGBA SimpleTextObjectPaint::sZero = gfxRGBA(0.0f, 0.0f, 0.0f, 0.0f);

gfxSVGGlyphs::gfxSVGGlyphs(hb_blob_t *aSVGTable)
{
    mSVGData = aSVGTable;

    unsigned int length;
    const char* svgData = hb_blob_get_data(mSVGData, &length);
    mHeader = reinterpret_cast<const Header*>(svgData);
    mDocIndex = nullptr;

    if (sizeof(Header) <= length && uint16_t(mHeader->mVersion) == 0 &&
        uint64_t(mHeader->mDocIndexOffset) + 2 <= length) {
        const DocIndex* docIndex = reinterpret_cast<const DocIndex*>
            (svgData + mHeader->mDocIndexOffset);
        
        if (uint64_t(mHeader->mDocIndexOffset) + 2 +
                uint16_t(docIndex->mNumEntries) * sizeof(IndexEntry) <= length) {
            mDocIndex = docIndex;
        }
    }
}

gfxSVGGlyphs::~gfxSVGGlyphs()
{
    hb_blob_destroy(mSVGData);
}












 int
gfxSVGGlyphs::CompareIndexEntries(const void *aKey, const void *aEntry)
{
    const uint32_t key = *(uint32_t*)aKey;
    const IndexEntry *entry = (const IndexEntry*)aEntry;

    if (key < uint16_t(entry->mStartGlyph)) {
        return -1;
    }
    if (key > uint16_t(entry->mEndGlyph)) {
        return 1;
    }
    return 0;
}

gfxSVGGlyphsDocument *
gfxSVGGlyphs::FindOrCreateGlyphsDocument(uint32_t aGlyphId)
{
    if (!mDocIndex) {
        
        return nullptr;
    }

    IndexEntry *entry = (IndexEntry*)bsearch(&aGlyphId, mDocIndex->mEntries,
                                             uint16_t(mDocIndex->mNumEntries),
                                             sizeof(IndexEntry),
                                             CompareIndexEntries);
    if (!entry) {
        return nullptr;
    }

    gfxSVGGlyphsDocument *result = mGlyphDocs.Get(entry->mDocOffset);

    if (!result) {
        unsigned int length;
        const uint8_t *data = (const uint8_t*)hb_blob_get_data(mSVGData, &length);
        if (entry->mDocOffset > 0 &&
            uint64_t(mHeader->mDocIndexOffset) + entry->mDocOffset + entry->mDocLength <= length) {
            result = new gfxSVGGlyphsDocument(data + mHeader->mDocIndexOffset + entry->mDocOffset,
                                              entry->mDocLength);
            mGlyphDocs.Put(entry->mDocOffset, result);
        }
    }

    return result;
}

nsresult
gfxSVGGlyphsDocument::SetupPresentation()
{
    mDocument->SetIsBeingUsedAsImage();

    nsCOMPtr<nsICategoryManager> catMan = do_GetService(NS_CATEGORYMANAGER_CONTRACTID);
    nsXPIDLCString contractId;
    nsresult rv = catMan->GetCategoryEntry("Gecko-Content-Viewers", "image/svg+xml", getter_Copies(contractId));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDocumentLoaderFactory> docLoaderFactory = do_GetService(contractId);
    NS_ASSERTION(docLoaderFactory, "Couldn't get DocumentLoaderFactory");

    nsCOMPtr<nsIContentViewer> viewer;
    rv = docLoaderFactory->CreateInstanceForDocument(nullptr, mDocument, nullptr, getter_AddRefs(viewer));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = viewer->Init(nullptr, nsIntRect(0, 0, 1000, 1000));
    if (NS_SUCCEEDED(rv)) {
        rv = viewer->Open(nullptr, nullptr);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    nsCOMPtr<nsIPresShell> presShell;
    rv = viewer->GetPresShell(getter_AddRefs(presShell));
    NS_ENSURE_SUCCESS(rv, rv);
    presShell->GetPresContext()->SetIsGlyph(true);

    if (!presShell->DidInitialize()) {
        nsRect rect = presShell->GetPresContext()->GetVisibleArea();
        rv = presShell->Initialize(rect.width, rect.height);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    mDocument->FlushPendingNotifications(Flush_Layout);

    mViewer = viewer;
    mPresShell = presShell;

    return NS_OK;
}






void
gfxSVGGlyphsDocument::FindGlyphElements(Element *aElem)
{
    for (nsIContent *child = aElem->GetLastChild(); child;
            child = child->GetPreviousSibling()) {
        if (!child->IsElement()) {
            continue;
        }
        FindGlyphElements(child->AsElement());
    }

    InsertGlyphId(aElem);
}









bool
gfxSVGGlyphs::RenderGlyph(gfxContext *aContext, uint32_t aGlyphId,
                          DrawMode aDrawMode, gfxTextObjectPaint *aObjectPaint)
{
    if (aDrawMode == gfxFont::GLYPH_PATH) {
        return false;
    }

    gfxContextAutoSaveRestore aContextRestorer(aContext);

    Element *glyph = mGlyphIdMap.Get(aGlyphId);
    NS_ASSERTION(glyph, "No glyph element. Should check with HasSVGGlyph() first!");

    return nsSVGUtils::PaintSVGGlyph(glyph, aContext, aDrawMode, aObjectPaint);
}

bool
gfxSVGGlyphs::GetGlyphExtents(uint32_t aGlyphId, const gfxMatrix& aSVGToAppSpace,
                              gfxRect *aResult)
{
    Element *glyph = mGlyphIdMap.Get(aGlyphId);
    NS_ASSERTION(glyph, "No glyph element. Should check with HasSVGGlyph() first!");

    return nsSVGUtils::GetSVGGlyphExtents(glyph, aSVGToAppSpace, aResult);
}

Element *
gfxSVGGlyphs::GetGlyphElement(uint32_t aGlyphId)
{
    Element *elem;

    if (!mGlyphIdMap.Get(aGlyphId, &elem)) {
        elem = nullptr;
        if (gfxSVGGlyphsDocument *set = FindOrCreateGlyphsDocument(aGlyphId)) {
            elem = set->GetGlyphElement(aGlyphId);
        }
        mGlyphIdMap.Put(aGlyphId, elem);
    }

    return elem;
}

bool
gfxSVGGlyphs::HasSVGGlyph(uint32_t aGlyphId)
{
    return !!GetGlyphElement(aGlyphId);
}

Element *
gfxSVGGlyphsDocument::GetGlyphElement(uint32_t aGlyphId)
{
    return mGlyphIdMap.Get(aGlyphId);
}

gfxSVGGlyphsDocument::gfxSVGGlyphsDocument(const uint8_t *aBuffer, uint32_t aBufLen)
{
    ParseDocument(aBuffer, aBufLen);
    if (!mDocument) {
        NS_WARNING("Could not parse SVG glyphs document");
        return;
    }

    Element *root = mDocument->GetRootElement();
    if (!root) {
        NS_WARNING("Could not parse SVG glyphs document");
        return;
    }

    nsresult rv = SetupPresentation();
    if (NS_FAILED(rv)) {
        NS_WARNING("Couldn't setup presentation for SVG glyphs document");
        return;
    }

    FindGlyphElements(root);
}

static nsresult
CreateBufferedStream(const uint8_t *aBuffer, uint32_t aBufLen,
                     nsCOMPtr<nsIInputStream> &aResult)
{
    nsCOMPtr<nsIInputStream> stream;
    nsresult rv = NS_NewByteInputStream(getter_AddRefs(stream),
                                        reinterpret_cast<const char *>(aBuffer),
                                        aBufLen, NS_ASSIGNMENT_DEPEND);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIInputStream> aBufferedStream;
    if (!NS_InputStreamIsBuffered(stream)) {
        rv = NS_NewBufferedInputStream(getter_AddRefs(aBufferedStream), stream, 4096);
        NS_ENSURE_SUCCESS(rv, rv);
        stream = aBufferedStream;
    }

    aResult = stream;

    return NS_OK;
}

nsresult
gfxSVGGlyphsDocument::ParseDocument(const uint8_t *aBuffer, uint32_t aBufLen)
{
    

    nsCOMPtr<nsIInputStream> stream;
    nsresult rv = CreateBufferedStream(aBuffer, aBufLen, stream);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIURI> uri;
    nsHostObjectProtocolHandler::GenerateURIString(NS_LITERAL_CSTRING(FONTTABLEURI_SCHEME),
                                                   mSVGGlyphsDocumentURI);
 
    rv = NS_NewURI(getter_AddRefs(uri), mSVGGlyphsDocumentURI);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIPrincipal> principal;
    nsContentUtils::GetSecurityManager()->
        GetNoAppCodebasePrincipal(uri, getter_AddRefs(principal));

    nsCOMPtr<nsIDOMDocument> domDoc;
    rv = NS_NewDOMDocument(getter_AddRefs(domDoc),
                           EmptyString(),   
                           EmptyString(),   
                           nullptr,          
                           uri, uri, principal,
                           false,           
                           nullptr,          
                           DocumentFlavorSVG);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDocument> document(do_QueryInterface(domDoc));
    if (!document) {
        return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIChannel> channel;
    rv = NS_NewInputStreamChannel(getter_AddRefs(channel), uri, nullptr ,
                                  SVG_CONTENT_TYPE, UTF8_CHARSET);
    NS_ENSURE_SUCCESS(rv, rv);

    channel->SetOwner(principal);

    document->SetReadyStateInternal(nsIDocument::READYSTATE_UNINITIALIZED);

    nsCOMPtr<nsIStreamListener> listener;
    rv = document->StartDocumentLoad("external-resource", channel,
                                     nullptr,    
                                     nullptr,    
                                     getter_AddRefs(listener),
                                     true );
    if (NS_FAILED(rv) || !listener) {
        return NS_ERROR_FAILURE;
    }

    rv = listener->OnStartRequest(channel, nullptr );
    if (NS_FAILED(rv)) {
        channel->Cancel(rv);
    }

    nsresult status;
    channel->GetStatus(&status);
    if (NS_SUCCEEDED(rv) && NS_SUCCEEDED(status)) {
        rv = listener->OnDataAvailable(channel, nullptr , stream, 0, aBufLen);
        if (NS_FAILED(rv)) {
            channel->Cancel(rv);
        }
        channel->GetStatus(&status);
    }

    rv = listener->OnStopRequest(channel, nullptr , status);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    document.swap(mDocument);

    return NS_OK;
}

void
gfxSVGGlyphsDocument::InsertGlyphId(Element *aGlyphElement)
{
    nsAutoString glyphIdStr;
    static const uint32_t glyphPrefixLength = 5;
    
    
    if (!aGlyphElement->GetAttr(kNameSpaceID_None, nsGkAtoms::id, glyphIdStr) ||
        !StringBeginsWith(glyphIdStr, NS_LITERAL_STRING("glyph")) ||
        glyphIdStr.Length() > glyphPrefixLength + 5) {
        return;
    }

    uint32_t id = 0;
    for (uint32_t i = glyphPrefixLength; i < glyphIdStr.Length(); ++i) {
      PRUnichar ch = glyphIdStr.CharAt(i);
      if (ch < '0' || ch > '9') {
        return;
      }
      if (ch == '0' && i == glyphPrefixLength) {
        return;
      }
      id = id * 10 + (ch - '0');
    }

    mGlyphIdMap.Put(id, aGlyphElement);
}

void
gfxTextObjectPaint::InitStrokeGeometry(gfxContext *aContext,
                                       float devUnitsPerSVGUnit)
{
    mStrokeWidth = aContext->CurrentLineWidth() / devUnitsPerSVGUnit;
    aContext->CurrentDash(mDashes, &mDashOffset);
    for (uint32_t i = 0; i < mDashes.Length(); i++) {
        mDashes[i] /= devUnitsPerSVGUnit;
    }
    mDashOffset /= devUnitsPerSVGUnit;
}
