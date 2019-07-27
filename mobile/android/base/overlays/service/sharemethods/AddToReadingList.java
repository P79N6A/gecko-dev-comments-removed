



package org.mozilla.gecko.overlays.service.sharemethods;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.os.Parcelable;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.R;
import org.mozilla.gecko.db.LocalBrowserDB;

import static org.mozilla.gecko.db.BrowserContract.Bookmarks;








public class AddToReadingList extends ShareMethod {
    private static final String LOGTAG = "GeckoAddToReadingList";

    @Override
    public Result handle(String title, String url, Parcelable unused) {
        ContentResolver resolver = context.getContentResolver();

        LocalBrowserDB browserDB = new LocalBrowserDB(GeckoProfile.DEFAULT_PROFILE);

        ContentValues values = new ContentValues();
        values.put(Bookmarks.TITLE, title);
        values.put(Bookmarks.URL, url);

        browserDB.addReadingListItem(resolver, values);

        return Result.SUCCESS;
    }

    @Override
    public String getSuccessMesssage() {
        return context.getResources().getString(R.string.reading_list_added);
    }

    
    @Override
    public String getFailureMessage() {
        return null;
    }

    public AddToReadingList(Context context) {
        super(context);
    }
}
