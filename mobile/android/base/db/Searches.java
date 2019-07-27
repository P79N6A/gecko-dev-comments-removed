




package org.mozilla.gecko.db;

import android.content.ContentResolver;
import android.content.ContentValues;

import org.mozilla.gecko.GeckoProfile;

public interface Searches {
    public void insert(ContentResolver cr, String query);
}
