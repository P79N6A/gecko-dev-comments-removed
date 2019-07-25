




































#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsDependentString.h"

#include "prinrval.h"

#include "gfxContext.h"
#include "gfxFont.h"
#include "gfxPlatform.h"

#include "gfxFontTest.h"

#include "gfxTextRunWordCache.h"

#if defined(XP_MACOSX)
#include "gfxTestCocoaHelper.h"
#endif

#ifdef MOZ_WIDGET_GTK2
#include "gtk/gtk.h"
#endif

class FrameTextRunCache;

static FrameTextRunCache *gTextRuns = nsnull;




class FrameTextRunCache : public nsExpirationTracker<gfxTextRun,3> {
public:
 enum { TIMEOUT_SECONDS = 10 };
 FrameTextRunCache()
     : nsExpirationTracker<gfxTextRun,3>(TIMEOUT_SECONDS*1000) {}
 ~FrameTextRunCache() {
   AgeAllGenerations();
 }

 void RemoveFromCache(gfxTextRun* aTextRun) {
   if (aTextRun->GetExpirationState()->IsTracked()) {
     RemoveObject(aTextRun);
   }
   gfxTextRunWordCache::RemoveTextRun(aTextRun);
 }

 
 virtual void NotifyExpired(gfxTextRun* aTextRun) {
   RemoveFromCache(aTextRun);
   delete aTextRun;
 }
};

static gfxTextRun *
MakeTextRun(const PRUnichar *aText, PRUint32 aLength,
           gfxFontGroup *aFontGroup, const gfxFontGroup::Parameters* aParams,
           PRUint32 aFlags)
{
   nsAutoPtr<gfxTextRun> textRun;
   if (aLength == 0) {
       textRun = aFontGroup->MakeEmptyTextRun(aParams, aFlags);
   } else if (aLength == 1 && aText[0] == ' ') {
       textRun = aFontGroup->MakeSpaceTextRun(aParams, aFlags);
   } else {
       textRun = gfxTextRunWordCache::MakeTextRun(aText, aLength, aFontGroup,
           aParams, aFlags);
   }
   if (!textRun)
       return nsnull;
   nsresult rv = gTextRuns->AddObject(textRun);
   if (NS_FAILED(rv)) {
       gTextRuns->RemoveFromCache(textRun);
       return nsnull;
   }
   return textRun.forget();
}

already_AddRefed<gfxContext>
MakeContext ()
{
   const int size = 200;

   nsRefPtr<gfxASurface> surface;

   surface = gfxPlatform::GetPlatform()->
       CreateOffscreenSurface(gfxIntSize(size, size),
                              gfxASurface::ContentFromFormat(gfxASurface::ImageFormatRGB24));
   gfxContext *ctx = new gfxContext(surface);
   NS_IF_ADDREF(ctx);
   return ctx;
}

int
main (int argc, char **argv) {
#ifdef MOZ_WIDGET_GTK2
   gtk_init(&argc, &argv);
#endif
#ifdef XP_MACOSX
   CocoaPoolInit();
#endif

   
   nsresult rv = NS_InitXPCOM2(nsnull, nsnull, nsnull);
   if (NS_FAILED(rv))
       return -1;

   rv = gfxPlatform::Init();
   if (NS_FAILED(rv))
       return -1;

   
   fflush (stderr);
   fflush (stdout);

   gTextRuns = new FrameTextRunCache();

   nsRefPtr<gfxContext> ctx = MakeContext();
   {
       gfxFontStyle style (FONT_STYLE_NORMAL,
                           NS_FONT_STRETCH_NORMAL,
                           139,
                           10.0,
                           NS_NewPermanentAtom(NS_LITERAL_STRING("en")),
                           0.0,
                           PR_FALSE, PR_FALSE, PR_FALSE,
                           NS_LITERAL_STRING(""),
                           NS_LITERAL_STRING(""));

       nsRefPtr<gfxFontGroup> fontGroup =
           gfxPlatform::GetPlatform()->CreateFontGroup(NS_LITERAL_STRING("Geneva, MS Sans Serif, Helvetica,serif"), &style, nsnull);

       gfxTextRunFactory::Parameters params = {
           ctx, nsnull, nsnull, nsnull, 0, 60
       };

       PRUint32 flags = gfxTextRunFactory::TEXT_IS_PERSISTENT;

       
       const char cString[] = "\xd8\xaa\xd9\x85";
       nsDependentCString cStr(cString);
       NS_ConvertUTF8toUTF16 str(cStr);
       gfxTextRun *tr = MakeTextRun(str.get(), str.Length(), fontGroup, &params, flags);
       tr->GetAdvanceWidth(0, str.Length(), nsnull);

       
       
       
       const char cString2[] = "\xd8\xaa\xd9\x85\n\xd8\xaa\xd8\x85 ";
       nsDependentCString cStr2(cString2);
       NS_ConvertUTF8toUTF16 str2(cStr2);
       gfxTextRun *tr2 = MakeTextRun(str2.get(), str2.Length(), fontGroup, &params, flags);
       tr2->GetAdvanceWidth(0, str2.Length(), nsnull);
   }

   fflush (stderr);
   fflush (stdout);
}
