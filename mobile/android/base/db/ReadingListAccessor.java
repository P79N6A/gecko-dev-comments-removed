



package org.mozilla.gecko.db;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.ContentObserver;
import android.database.Cursor;

public interface ReadingListAccessor {
    


    Cursor getReadingList(ContentResolver cr);

    int getCount(ContentResolver cr);

    Cursor getReadingListUnfetched(ContentResolver cr);

    boolean isReadingListItem(ContentResolver cr, String uri);

    void addReadingListItem(ContentResolver cr, ContentValues values);

    void updateReadingListItem(ContentResolver cr, ContentValues values);

    void removeReadingListItemWithURL(ContentResolver cr, String uri);

    void registerContentObserver(Context context, ContentObserver observer);
}
