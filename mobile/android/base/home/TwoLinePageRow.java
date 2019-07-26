




package org.mozilla.gecko.home;

import org.mozilla.gecko.Favicons;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.db.BrowserDB.URLColumns;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.widget.FaviconView;

import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.widget.LinearLayout;
import android.widget.TextView;

public class TwoLinePageRow extends LinearLayout {
    private static final int NO_ICON = 0;

    private final TextView mTitle;
    private final TextView mUrl;
    private final FaviconView mFavicon;

    private int mUrlIconId;
    private int mBookmarkIconId;

    public TwoLinePageRow(Context context) {
        this(context, null);
    }

    public TwoLinePageRow(Context context, AttributeSet attrs) {
        super(context, attrs);

        setGravity(Gravity.CENTER_VERTICAL);

        mUrlIconId = NO_ICON;
        mBookmarkIconId = NO_ICON;

        LayoutInflater.from(context).inflate(R.layout.two_line_page_row, this);
        mTitle = (TextView) findViewById(R.id.title);
        mUrl = (TextView) findViewById(R.id.url);
        mFavicon = (FaviconView) findViewById(R.id.favicon);
    }

    private void setTitle(String title) {
        mTitle.setText(title);
    }

    private void setUrl(String url) {
        mUrl.setText(url);
    }

    private void setUrl(int stringId) {
        mUrl.setText(stringId);
    }

    private void setUrlIcon(int urlIconId) {
        if (mUrlIconId == urlIconId) {
            return;
        }

        mUrlIconId = urlIconId;
        mUrl.setCompoundDrawablesWithIntrinsicBounds(mUrlIconId, 0, mBookmarkIconId, 0);
    }

    private void setFaviconWithUrl(Bitmap favicon, String url) {
        mFavicon.updateImage(favicon, url);
    }

    private void setBookmarkIcon(int bookmarkIconId) {
        if (mBookmarkIconId == bookmarkIconId) {
            return;
        }

        mBookmarkIconId = bookmarkIconId;
        mUrl.setCompoundDrawablesWithIntrinsicBounds(mUrlIconId, 0, mBookmarkIconId, 0);
    }

    public void updateFromCursor(Cursor cursor) {
        if (cursor == null) {
            return;
        }

        int titleIndex = cursor.getColumnIndexOrThrow(URLColumns.TITLE);
        final String title = cursor.getString(titleIndex);

        int urlIndex = cursor.getColumnIndexOrThrow(URLColumns.URL);
        final String url = cursor.getString(urlIndex);

        
        
        setTitle(TextUtils.isEmpty(title) ? url : title);

        
        if (Tabs.getInstance().hasUrl(url)) {
            setUrl(R.string.switch_to_tab);
            setUrlIcon(R.drawable.ic_url_bar_tab);
        } else {
            setUrl(url);
            setUrlIcon(NO_ICON);
        }

        int faviconIndex = cursor.getColumnIndex(URLColumns.FAVICON);
        if (faviconIndex != -1) {
            byte[] b = cursor.getBlob(faviconIndex);

            Bitmap favicon = null;
            if (b != null) {
                Bitmap bitmap = BitmapUtils.decodeByteArray(b);
                if (bitmap != null) {
                    favicon = Favicons.getInstance().scaleImage(bitmap);
                }
            }

            setFaviconWithUrl(favicon, url);
        } else {
            
            setFaviconWithUrl(Favicons.getInstance().getFaviconFromMemCache(url), url);
        }

        final int bookmarkIdIndex = cursor.getColumnIndex(Combined.BOOKMARK_ID);
        if (bookmarkIdIndex != -1) {
            final long bookmarkId = cursor.getLong(bookmarkIdIndex);
            final int displayIndex = cursor.getColumnIndex(Combined.DISPLAY);

            final int display;
            if (displayIndex != -1) {
                display = cursor.getInt(displayIndex);
            } else {
                display = Combined.DISPLAY_NORMAL;
            }

            
            
            if (bookmarkId == 0) {
                setBookmarkIcon(NO_ICON);
            } else if (display == Combined.DISPLAY_READER) {
                setBookmarkIcon(R.drawable.ic_url_bar_reader);
            } else {
                setBookmarkIcon(R.drawable.ic_url_bar_star);
            }
        } else {
            setBookmarkIcon(NO_ICON);
        }
    }
}
