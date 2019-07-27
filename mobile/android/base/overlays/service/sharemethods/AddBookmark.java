



package org.mozilla.gecko.overlays.service.sharemethods;

import android.content.ContentResolver;
import android.content.Context;
import android.os.Parcelable;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.R;
import org.mozilla.gecko.db.LocalBrowserDB;

public class AddBookmark extends ShareMethod {
    private static final String LOGTAG = "GeckoAddBookmark";

    @Override
    public Result handle(String title, String url, Parcelable unused) {
        ContentResolver resolver = context.getContentResolver();

        LocalBrowserDB browserDB = new LocalBrowserDB(GeckoProfile.DEFAULT_PROFILE);
        browserDB.addBookmark(resolver, url, title);

        return Result.SUCCESS;
    }

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
