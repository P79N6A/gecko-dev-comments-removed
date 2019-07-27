



package org.mozilla.gecko.overlays.service;

import android.content.Intent;
import android.os.Bundle;
import android.os.Parcelable;
import org.mozilla.gecko.overlays.OverlayConstants;
import org.mozilla.gecko.overlays.service.sharemethods.ShareMethod;

import static org.mozilla.gecko.overlays.OverlayConstants.EXTRA_SHARE_METHOD;




public class ShareData {
    private static final String LOGTAG = "GeckoShareRequest";

    public final String url;
    public final String title;
    public final Parcelable extra;
    public final ShareMethod.Type shareMethodType;

    public ShareData(String url, String title, Parcelable extra, ShareMethod.Type shareMethodType) {
        if (url == null) {
            throw new IllegalArgumentException("Null url passed to ShareData!");
        }

        this.url = url;
        this.title = title;
        this.extra = extra;
        this.shareMethodType = shareMethodType;
    }

    public static ShareData fromIntent(Intent intent) {
        Bundle extras = intent.getExtras();

        
        final String url = extras.getString(OverlayConstants.EXTRA_URL);
        final String title = extras.getString(OverlayConstants.EXTRA_TITLE);
        final Parcelable extra = extras.getParcelable(OverlayConstants.EXTRA_PARAMETERS);
        ShareMethod.Type shareMethodType = (ShareMethod.Type) extras.get(EXTRA_SHARE_METHOD);

        return new ShareData(url, title, extra, shareMethodType);
    }
}
