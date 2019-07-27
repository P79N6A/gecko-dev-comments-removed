



package org.mozilla.gecko.overlays.service.sharemethods;

import android.content.ContentResolver;
import android.content.Context;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.R;
import org.mozilla.gecko.db.LocalBrowserDB;
import org.mozilla.gecko.overlays.service.ShareData;

public class AddBookmark extends ShareMethod {
    private static final String LOGTAG = "GeckoAddBookmark";

    @Override
    public Result handle(ShareData shareData) {
        ContentResolver resolver = context.getContentResolver();

        LocalBrowserDB browserDB = new LocalBrowserDB(GeckoProfile.DEFAULT_PROFILE);
        browserDB.addBookmark(resolver, shareData.title, shareData.url);

        return Result.SUCCESS;
    }

    @Override
    public String getSuccessMesssage() {
        return context.getResources().getString(R.string.bookmark_added);
    }

    
    @Override
    public String getFailureMessage() {
        return null;
    }

    public AddBookmark(Context context) {
        super(context);
    }
}
