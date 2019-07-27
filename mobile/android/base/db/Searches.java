




package org.mozilla.gecko.db;

import android.content.ContentResolver;

public interface Searches {
    public void insert(ContentResolver cr, String query);
}
