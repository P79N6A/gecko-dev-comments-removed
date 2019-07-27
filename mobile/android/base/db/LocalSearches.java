




package org.mozilla.gecko.db;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.net.Uri;




public class LocalSearches implements Searches {
    private final Uri uriWithProfile;

    public LocalSearches(String mProfile) {
        uriWithProfile = DBUtils.appendProfileWithDefault(mProfile, BrowserContract.SearchHistory.CONTENT_URI);
    }

    @Override
    public void insert(ContentResolver cr, String query) {
        final ContentValues values = new ContentValues();
        values.put(BrowserContract.SearchHistory.QUERY, query);
        cr.insert(uriWithProfile, values);
    }
}
