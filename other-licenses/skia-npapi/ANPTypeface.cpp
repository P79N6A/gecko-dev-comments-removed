

























#include "SkANP.h"
#include "SkFontHost.h"

static ANPTypeface* anp_createFromName(const char name[], ANPTypefaceStyle s) {
    SkTypeface* tf = SkTypeface::CreateFromName(name,
                                        static_cast<SkTypeface::Style>(s));
    return reinterpret_cast<ANPTypeface*>(tf);
}

static ANPTypeface* anp_createFromTypeface(const ANPTypeface* family,
                                           ANPTypefaceStyle s) {
    SkTypeface* tf = SkTypeface::CreateFromTypeface(family,
                                          static_cast<SkTypeface::Style>(s));
    return reinterpret_cast<ANPTypeface*>(tf);
}

static int32_t anp_getRefCount(const ANPTypeface* tf) {
    return tf ? tf->getRefCnt() : 0;
}

static void anp_ref(ANPTypeface* tf) {
    SkSafeRef(tf);
}

static void anp_unref(ANPTypeface* tf) {
    SkSafeUnref(tf);
}

static ANPTypefaceStyle anp_getStyle(const ANPTypeface* tf) {
    SkTypeface::Style s = tf ? tf->style() : SkTypeface::kNormal;
    return static_cast<ANPTypefaceStyle>(s);
}

static int32_t anp_getFontPath(const ANPTypeface* tf, char fileName[],
                               int32_t length, int32_t* index) {
    size_t size = SkFontHost::GetFileName(SkTypeface::UniqueID(tf), fileName,
                                          length, index);
    return static_cast<int32_t>(size);
}

static const char* gFontDir;
#define FONT_DIR_SUFFIX     "/fonts/"

static const char* anp_getFontDirectoryPath() {
    if (NULL == gFontDir) {
        const char* root = getenv("ANDROID_ROOT");
        size_t len = strlen(root);
        char* storage = (char*)malloc(len + sizeof(FONT_DIR_SUFFIX));
        if (NULL == storage) {
            return NULL;
        }
        memcpy(storage, root, len);
        memcpy(storage + len, FONT_DIR_SUFFIX, sizeof(FONT_DIR_SUFFIX));
        
        
        
        gFontDir = storage;
    }
    return gFontDir;
}



#define ASSIGN(obj, name)   (obj)->name = anp_##name

void InitTypeFaceInterface(ANPTypefaceInterfaceV0* i) {
    ASSIGN(i, createFromName);
    ASSIGN(i, createFromTypeface);
    ASSIGN(i, getRefCount);
    ASSIGN(i, ref);
    ASSIGN(i, unref);
    ASSIGN(i, getStyle);
    ASSIGN(i, getFontPath);
    ASSIGN(i, getFontDirectoryPath);
}
