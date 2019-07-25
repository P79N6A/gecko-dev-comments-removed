




































package org.mozilla.gecko.db;

import java.io.ByteArrayOutputStream;
import java.util.Date;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.CursorWrapper;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.provider.Browser;
import android.provider.Browser.BookmarkColumns;

public class AndroidBrowserDB implements BrowserDB.BrowserDBIface {
    private static final String LOGTAG = "GeckoAndroidBrowserDB";

    public Cursor filter(ContentResolver cr, CharSequence constraint, int limit) {
        Cursor c = cr.query(Browser.BOOKMARKS_URI,
                            new String[] { BookmarkColumns.URL, BookmarkColumns.TITLE, BookmarkColumns.FAVICON, "thumbnail" },
                            
                            
                            "(" + Browser.BookmarkColumns.URL + " LIKE ? OR " + Browser.BookmarkColumns.TITLE + " LIKE ?)"
                            + " AND LENGTH(" + Browser.BookmarkColumns.URL + ") > 0",
                            new String[] {"%" + constraint.toString() + "%", "%" + constraint.toString() + "%",},
                            
                            
                            
                            Browser.BookmarkColumns.VISITS + " * MAX(1, (" +
                            Browser.BookmarkColumns.DATE + " - " + new Date().getTime() + ") / 86400000 + 120) DESC LIMIT " + limit);

        return new AndroidDBCursor(c);
    }

    public void updateVisitedHistory(ContentResolver cr, String uri) {
        Browser.updateVisitedHistory(cr, uri, true);
    }

    public void updateHistoryTitle(ContentResolver cr, String uri, String title) {
        ContentValues values = new ContentValues();
        values.put(Browser.BookmarkColumns.TITLE, title);

        cr.update(Browser.BOOKMARKS_URI,
                  values,
                  Browser.BookmarkColumns.URL + " = ?",
                  new String[] { uri });
    }

    public Cursor getAllVisitedHistory(ContentResolver cr) {
        Cursor c = cr.query(Browser.BOOKMARKS_URI,
                            new String[] { Browser.BookmarkColumns.URL },
                            Browser.BookmarkColumns.BOOKMARK + " = 0 AND " +
                            Browser.BookmarkColumns.VISITS + " > 0",
                            null,
                            null);

        return new AndroidDBCursor(c);
    }

    public Cursor getRecentHistory(ContentResolver cr, int limit) {
        Cursor c = cr.query(Browser.BOOKMARKS_URI,
                            new String[] { BookmarkColumns.URL,
                                           BookmarkColumns.TITLE,
                                           BookmarkColumns.FAVICON,
                                           BookmarkColumns.DATE },
                            
                            
                            Browser.BookmarkColumns.DATE + " > 0",
                            null,
                            Browser.BookmarkColumns.DATE + " DESC LIMIT " + limit);

        return new AndroidDBCursor(c);
    }

    public void clearHistory(ContentResolver cr) {
        Browser.clearHistory(cr);
    }

    public Cursor getAllBookmarks(ContentResolver cr) {
        Cursor c = cr.query(Browser.BOOKMARKS_URI,
                            new String[] { BookmarkColumns.URL, BookmarkColumns.TITLE, BookmarkColumns.FAVICON },
                            
                            
                            
                            
                            Browser.BookmarkColumns.BOOKMARK + " = 1 AND LENGTH(" + Browser.BookmarkColumns.URL + ") > 0",
                            null,
                            Browser.BookmarkColumns.TITLE);

        return new AndroidDBCursor(c);
    }

    public boolean isBookmark(ContentResolver cr, String uri) {
        Cursor cursor = cr.query(Browser.BOOKMARKS_URI,
                                 new String[] { BookmarkColumns.URL },
                                 Browser.BookmarkColumns.URL + " = ? and " + Browser.BookmarkColumns.BOOKMARK + " = ?",
                                 new String[] { uri, "1" },
                                 Browser.BookmarkColumns.URL);

        int count = cursor.getCount();
        cursor.close();

        return (count == 1);
    }

    public void addBookmark(ContentResolver cr, String title, String uri) {
        Cursor cursor = cr.query(Browser.BOOKMARKS_URI,
                                 new String[] { BookmarkColumns.URL },
                                 Browser.BookmarkColumns.URL + " = ?",
                                 new String[] { uri },
                                 Browser.BookmarkColumns.URL);

        ContentValues values = new ContentValues();
        values.put(Browser.BookmarkColumns.BOOKMARK, "1");
        values.put(Browser.BookmarkColumns.TITLE, title);

        if (cursor.getCount() == 1) {
            
            cr.update(Browser.BOOKMARKS_URI,
                      values,
                      Browser.BookmarkColumns.URL + " = ?",
                      new String[] { uri });
        } else {
            
            values.put(Browser.BookmarkColumns.URL, uri);
            cr.insert(Browser.BOOKMARKS_URI, values);
        }

        cursor.close();
    }

    public void removeBookmark(ContentResolver cr, String uri) {
        ContentValues values = new ContentValues();
        values.put(Browser.BookmarkColumns.BOOKMARK, "0");

        cr.update(Browser.BOOKMARKS_URI,
                  values,
                  Browser.BookmarkColumns.URL + " = ?",
                  new String[] { uri });
    }

    public BitmapDrawable getFaviconForUrl(ContentResolver cr, String uri) {
        Cursor c = cr.query(Browser.BOOKMARKS_URI,
                            new String[] { Browser.BookmarkColumns.FAVICON },
                            Browser.BookmarkColumns.URL + " = ?",
                            new String[] { uri },
                            null);

        if (!c.moveToFirst()) {
            c.close();
            return null;
        }

        int faviconIndex = c.getColumnIndexOrThrow(Browser.BookmarkColumns.FAVICON);

        byte[] b = c.getBlob(faviconIndex);
        c.close();

        if (b == null)
            return null;

        Bitmap bitmap = BitmapFactory.decodeByteArray(b, 0, b.length);
        return new BitmapDrawable(bitmap);
    }

    public void updateFaviconForUrl(ContentResolver cr, String uri,
            BitmapDrawable favicon) {
        Bitmap bitmap = favicon.getBitmap();

        ByteArrayOutputStream stream = new ByteArrayOutputStream();
        bitmap.compress(Bitmap.CompressFormat.PNG, 100, stream);

        ContentValues values = new ContentValues();
        values.put(Browser.BookmarkColumns.FAVICON, stream.toByteArray());
        values.put(Browser.BookmarkColumns.URL, uri);

        cr.update(Browser.BOOKMARKS_URI,
                  values,
                  Browser.BookmarkColumns.URL + " = ?",
                  new String[] { uri });
    }

    public void updateThumbnailForUrl(ContentResolver cr, String uri,
            BitmapDrawable thumbnail) {
        Bitmap bitmap = thumbnail.getBitmap();

        Cursor cursor = cr.query(Browser.BOOKMARKS_URI,
                                 new String[] { BookmarkColumns.URL },
                                 Browser.BookmarkColumns.URL + " = ?",
                                 new String[] { uri },
                                 Browser.BookmarkColumns.URL);

        ContentValues values = new ContentValues();
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        bitmap.compress(Bitmap.CompressFormat.PNG, 0, bos);
        values.put("thumbnail", bos.toByteArray());

        if (cursor.getCount() == 1) {
            
            cr.update(Browser.BOOKMARKS_URI,
                      values,
                      Browser.BookmarkColumns.URL + " = ?",
                      new String[] { uri });
        } else {
            
            values.put(Browser.BookmarkColumns.URL, uri);
            cr.insert(Browser.BOOKMARKS_URI, values);
        }

        cursor.close();
    }

    private static class AndroidDBCursor extends CursorWrapper {
        public AndroidDBCursor(Cursor c) {
            super(c);
        }

        private String translateColumnName(String columnName) {
            if (columnName.equals(BrowserDB.URLColumns.URL)) {
                columnName = Browser.BookmarkColumns.URL;
            } else if (columnName.equals(BrowserDB.URLColumns.TITLE)) {
                columnName = Browser.BookmarkColumns.TITLE;
            } else if (columnName.equals(BrowserDB.URLColumns.FAVICON)) {
                columnName = Browser.BookmarkColumns.FAVICON;
            } else if (columnName.equals(BrowserDB.URLColumns.THUMBNAIL)) {
                columnName = "thumbnail";
            } else if (columnName.equals(BrowserDB.URLColumns.DATE_LAST_VISITED)) {
                columnName = Browser.BookmarkColumns.DATE;
            }

            return columnName;
        }

        @Override
        public int getColumnIndex(String columnName) {
            return super.getColumnIndex(translateColumnName(columnName));
        }

        @Override
        public int getColumnIndexOrThrow(String columnName) {
            return super.getColumnIndexOrThrow(translateColumnName(columnName));
        }
    }
}
