



package org.mozilla.gecko.favicons;

import android.graphics.Bitmap;




public interface OnFaviconLoadedListener {
    void onFaviconLoaded(String url, Bitmap favicon);
}
