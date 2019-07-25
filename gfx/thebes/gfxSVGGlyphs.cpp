



































#include "gfxSVGGlyphs.h"

#include "nscore.h"
#include "nsError.h"
#include "nsAutoPtr.h"
#include "nsIParser.h"
#include "nsIDOMNodeList.h"
#include "nsString.h"
#include "nsIDocument.h"
#include "nsPrintfCString.h"
#include "nsICategoryManager.h"
#include "nsIDocumentLoaderFactory.h"
#include "nsIContentViewer.h"
#include "nsIStreamListener.h"
#include "nsServiceManagerUtils.h"
#include "nsIPresShell.h"
#include "nsIFrame.h"
#include "nsQueryFrame.h"
#include "nsIContentSink.h"
#include "nsXMLContentSink.h"
#include "nsNetUtil.h"
#include "nsIInputStream.h"
#include "nsContentUtils.h"
#include "nsStringStream.h"
#include "nsStreamUtils.h"
#include "nsIPrincipal.h"
#include "Element.h"

#define SVG_CONTENT_TYPE NS_LITERAL_CSTRING("image/svg+xml")
#define UTF8_CHARSET NS_LITERAL_CSTRING("utf-8")

typedef mozilla::dom::Element Element;

mozilla::gfx::UserDataKey gfxTextObjectPaint::sUserDataKey;

const float gfxSVGGlyphs::SVG_UNITS_PER_EM = 1000.0f;

const gfxRGBA SimpleTextObjectPaint::sZero = gfxRGBA(0.0f, 0.0f, 0.0f, 0.0f);

gfxSVGGlyphs::gfxSVGGlyphs(FallibleTArray<uint8_t>& aSVGTable,
                           const FallibleTArray<uint8_t>& aCmapTable)
{
    mSVGData.SwapElements(aSVGTable);

    mHeader = reinterpret_cast<Header*>(mSVGData.Elements());
    UnmangleHeaders();

    mGlyphDocs.Init();
    mGlyphIdMap.Init();
    mCmapData = aCmapTable;
}

void
gfxSVGGlyphs::UnmangleHeaders()
{
    mHeader->mIndexLength = NS_SWAP16(mHeader->mIndexLength);

    mIndex = reinterpret_cast<IndexEntry*>(mSVGData.Elements() + sizeof(Header));

    for (uint16_t i = 0; i < mHeader->mIndexLength; i++) {
        mIndex[i].mStartGlyph = NS_SWAP16(mIndex[i].mStartGlyph);
        mIndex[i].mEndGlyph = NS_SWAP16(mIndex[i].mEndGlyph);
        mIndex[i].mDocOffset = NS_SWAP32(mIndex[i].mDocOffset);
        mIndex[i].mDocLength = NS_SWAP32(mIndex[i].mDocLength);
    }
}












 int
gfxSVGGlyphs::CompareIndexEntries(const void *_key, const void *_entry)
{
    const uint32_t key = *(uint32_t*)_key;
    const IndexEntry *entry = (const IndexEntry*)_entry;

    if (key < entry->mStartGlyph) return -1;
    if (key >= entry->mEndGlyph) return 1;
    return 0;
}

gfxSVGGlyphsDocument *
gfxSVGGlyphs::FindOrCreateGlyphsDocument(uint32_t aGlyphId)
{
    IndexEntry *entry = (IndexEntry*)bsearch(&aGlyphId, mIndex,
                                             mHeader->mIndexLength,
                                             sizeof(IndexEntry),
                                             CompareIndexEntries);
    if (!entry) {
        return nullptr;
    }

    gfxSVGGlyphsDocument *result = mGlyphDocs.Get(entry->mDocOffset);

    if (!result) {
        result = new gfxSVGGlyphsDocument(mSVGData.Elements() + entry->mDocOffset,
                                          entry->mDocLength, mCmapData);
        mGlyphDocs.Put(entry->mDocOffset, result);
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

    if (!presShell->DidInitialReflow()) {
        nsRect rect = presShell->GetPresContext()->GetVisibleArea();
        rv = presShell->InitialReflow(rect.width, rect.height);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    mDocument->FlushPendingNotifications(Flush_Layout);

    mViewer = viewer;
    mPresShell = presShell;

    return NS_OK;
}







void
gfxSVGGlyphsDocument::FindGlyphElements(Element *aElem,
                                        const FallibleTArray<uint8_t> &aCmapTable)
{
    for (nsIContent *child = aElem->GetLastChild(); child;
            child = child->GetPreviousSibling()) {
        if (!child->IsElement()) {
            continue;
        }
        FindGlyphElements(child->AsElement(), aCmapTable);
    }

    InsertGlyphChar(aElem, aCmapTable);
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

    return nsContentUtils::PaintSVGGlyph(glyph, aContext, aDrawMode, aObjectPaint);
}

bool
gfxSVGGlyphs::GetGlyphExtents(uint32_t aGlyphId, const gfxMatrix& aSVGToAppSpace,
                              gfxRect *aResult)
{
    Element *glyph = mGlyphIdMap.Get(aGlyphId);
    NS_ASSERTION(glyph, "No glyph element. Should check with HasSVGGlyph() first!");

    return nsContentUtils::GetSVGGlyphExtents(glyph, aSVGToAppSpace, aResult);
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

gfxSVGGlyphsDocument::gfxSVGGlyphsDocument(uint8_t *aBuffer, uint32_t aBufLen,
                                           const FallibleTArray<uint8_t>& aCmapTable)
{
    mGlyphIdMap.Init();
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

    FindGlyphElements(root, aCmapTable);
}

static nsresult
CreateBufferedStream(uint8_t *aBuffer, uint32_t aBufLen,
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
gfxSVGGlyphsDocument::ParseDocument(uint8_t *aBuffer, uint32_t aBufLen)
{
    

    nsCOMPtr<nsIInputStream> stream;
    nsresult rv = CreateBufferedStream(aBuffer, aBufLen, stream);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIPrincipal> principal =
        do_CreateInstance("@mozilla.org/nullprincipal;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIURI> uri;
    principal->GetURI(getter_AddRefs(uri));

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

    nsCOMPtr<nsIChannel> channel;
    rv = NS_NewInputStreamChannel(getter_AddRefs(channel), uri, nullptr ,
                                  SVG_CONTENT_TYPE, UTF8_CHARSET);
    NS_ENSURE_SUCCESS(rv, rv);

    channel->SetOwner(principal);

    nsCOMPtr<nsIDocument> document(do_QueryInterface(domDoc));
    if (!document) {
        return NS_ERROR_FAILURE;
    }
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

    document->SetBaseURI(uri);
    document->SetPrincipal(principal);

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
    if (!aGlyphElement->GetAttr(kNameSpaceID_None, nsGkAtoms::glyphid, glyphIdStr)) {
        return;
    }

    nsresult rv;
    uint32_t glyphId = glyphIdStr.ToInteger(&rv, kRadix10);

    if (NS_FAILED(rv)) {
        return;
    }

    mGlyphIdMap.Put(glyphId, aGlyphElement);
}

void
gfxSVGGlyphsDocument::InsertGlyphChar(Element *aGlyphElement,
                                      const FallibleTArray<uint8_t> &aCmapTable)
{
    nsAutoString glyphChar;
    if (!aGlyphElement->GetAttr(kNameSpaceID_None, nsGkAtoms::glyphchar, glyphChar)) {
        return;
    }

    uint32_t varSelector;

    switch (glyphChar.Length()) {
        case 0:
            NS_WARNING("glyphchar is empty");
            return;
        case 1:
            varSelector = 0;
            break;
        case 2:
            if (gfxFontUtils::IsVarSelector(glyphChar.CharAt(1))) {
                varSelector = glyphChar.CharAt(1);
                break;
            }
        default:
            NS_WARNING("glyphchar contains more than one character");
            return;
    }

    uint32_t glyphId = gfxFontUtils::MapCharToGlyph(aCmapTable.Elements(),
                                                    aCmapTable.Length(),
                                                    glyphChar.CharAt(0),
                                                    varSelector);

    if (glyphId) {
        mGlyphIdMap.Put(glyphId, aGlyphElement);
    }
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
