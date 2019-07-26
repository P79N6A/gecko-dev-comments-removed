




package org.mozilla.gecko.home;

import org.mozilla.gecko.Favicons;
import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserDB.URLColumns;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.widget.FaviconView;

import android.content.Context;
import android.content.res.TypedArray;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

public class TwoLinePageRow extends LinearLayout {

    private final TextView mTitle;
    private final TextView mUrl;
    private final FaviconView mFavicon;

    public TwoLinePageRow(Context context) {
        this(context, null);
    }

    public TwoLinePageRow(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public TwoLinePageRow(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        setGravity(Gravity.CENTER_VERTICAL);

        LayoutInflater.from(context).inflate(R.layout.two_line_page_row, this);
        mTitle = (TextView) findViewById(R.id.title);
        mUrl = (TextView) findViewById(R.id.url);
        mFavicon = (FaviconView) findViewById(R.id.favicon);
    }

    public void updateFromCursor(Cursor cursor) {
        if (cursor == null) {
            return;
        }

        int titleIndex = cursor.getColumnIndexOrThrow(URLColumns.TITLE);
        final String title = cursor.getString(titleIndex);

        int urlIndex = cursor.getColumnIndexOrThrow(URLColumns.URL);
        final String url = cursor.getString(urlIndex);

        
        
        if (TextUtils.isEmpty(title)) {
            mTitle.setText(url);
        } else {
            mTitle.setText(title);
        }

        
        
        Integer tabId = null;
        if (tabId != null) {
            mUrl.setText(R.string.switch_to_tab);
            mUrl.setCompoundDrawablesWithIntrinsicBounds(R.drawable.ic_url_bar_tab, 0, 0, 0);
        } else {
            mUrl.setText(url);
            mUrl.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
        }

        byte[] b = cursor.getBlob(cursor.getColumnIndexOrThrow(URLColumns.FAVICON));
        Bitmap favicon = null;
        if (b != null) {
            Bitmap bitmap = BitmapUtils.decodeByteArray(b);
            if (bitmap != null) {
                favicon = Favicons.getInstance().scaleImage(bitmap);
            }
        }

        mFavicon.updateImage(favicon, url);
    }
}
