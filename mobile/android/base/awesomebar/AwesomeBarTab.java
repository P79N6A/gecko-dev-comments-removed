




package org.mozilla.gecko;

import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Resources;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;
import android.widget.ImageView;
import android.widget.TabHost.TabContentFactory;
import android.view.inputmethod.InputMethodManager;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;

import org.mozilla.gecko.db.BrowserDB.URLColumns;
import org.mozilla.gecko.AwesomeBar.ContextMenuSubject;

abstract class AwesomeBarTab {
    abstract public String getTag();
    abstract public int getTitleStringId();
    abstract public void destroy();
    abstract public TabContentFactory getFactory();
    abstract public boolean   onBackPressed();
    abstract public ContextMenuSubject getSubject(ContextMenu menu, View view, ContextMenuInfo menuInfo);

    protected View.OnTouchListener mListListener;
    private AwesomeBarTabs.OnUrlOpenListener mListener;
    private LayoutInflater mInflater = null;
    private ContentResolver mContentResolver = null;
    private Resources mResources;
    
    public static final int MAX_RESULTS = 100;
    protected Context mContext = null;

    public AwesomeBarTab(Context context) {
        mContext = context;
    }

    public void setListTouchListener(View.OnTouchListener listener) {
        mListListener = listener;
    }

    protected static final class AwesomeEntryViewHolder {
        public TextView titleView;
        public TextView urlView;
        public ImageView faviconView;
        public ImageView bookmarkIconView;
    }

    protected LayoutInflater getInflater() {
        if (mInflater == null) {
            mInflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        }
        return mInflater;
    }

    protected AwesomeBarTabs.OnUrlOpenListener getUrlListener() {
        return mListener;
    }

    protected void setUrlListener(AwesomeBarTabs.OnUrlOpenListener listener) {
        mListener = listener;
    }

    protected ContentResolver getContentResolver() {
        if (mContentResolver == null) {
            mContentResolver = mContext.getContentResolver();
        }
        return mContentResolver;
    }

    protected Resources getResources() {
        if (mResources == null) {
            mResources = mContext.getResources();
        }
        return mResources;
    }

    protected String getReaderForUrl(String url) {
        
        
        return "about:reader?url=" + Uri.encode(url);
    }

    protected void updateFavicon(ImageView faviconView, Cursor cursor) {
        byte[] b = cursor.getBlob(cursor.getColumnIndexOrThrow(URLColumns.FAVICON));
        if (b == null) {
            faviconView.setImageDrawable(null);
        } else {
            Bitmap bitmap = BitmapFactory.decodeByteArray(b, 0, b.length);
            faviconView.setImageBitmap(bitmap);
        }
    }

    protected void updateTitle(TextView titleView, Cursor cursor) {
        int titleIndex = cursor.getColumnIndexOrThrow(URLColumns.TITLE);
        String title = cursor.getString(titleIndex);

        
        
        if (TextUtils.isEmpty(title)) {
            int urlIndex = cursor.getColumnIndexOrThrow(URLColumns.URL);
            title = cursor.getString(urlIndex);
        }

        titleView.setText(title);
    }

    protected void updateUrl(TextView urlView, Cursor cursor) {
        int urlIndex = cursor.getColumnIndexOrThrow(URLColumns.URL);
        String url = cursor.getString(urlIndex);

        urlView.setText(url);
    }

    protected boolean hideSoftInput(View view) {
        InputMethodManager imm =
                (InputMethodManager) mContext.getSystemService(Context.INPUT_METHOD_SERVICE);

        return imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
    }
}
