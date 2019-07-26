




package org.mozilla.gecko.home;

import android.util.Log;
import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.db.BrowserContract.HomeItems;
import org.mozilla.gecko.favicons.OnFaviconLoadedListener;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.widget.FaviconView;

import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.View;

import java.lang.ref.WeakReference;

public class PanelListRow extends TwoLineRow {

    public PanelListRow(Context context) {
        this(context, null);
    }

    public PanelListRow(Context context, AttributeSet attrs) {
        super(context, attrs);

        
        
        final View iconView = findViewById(R.id.icon);
        iconView.setVisibility(View.GONE);
    }

    @Override
    public void updateFromCursor(Cursor cursor) {
        if (cursor == null) {
            return;
        }

        
        

        int titleIndex = cursor.getColumnIndexOrThrow(HomeItems.TITLE);
        final String title = cursor.getString(titleIndex);
        setPrimaryText(title);

        int urlIndex = cursor.getColumnIndexOrThrow(HomeItems.URL);
        final String url = cursor.getString(urlIndex);
        setSecondaryText(url);
    }
}
