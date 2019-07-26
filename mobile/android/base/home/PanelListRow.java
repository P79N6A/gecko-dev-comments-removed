




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserContract.HomeItems;

import com.squareup.picasso.Picasso;

import android.content.Context;
import android.database.Cursor;
import android.util.AttributeSet;
import android.view.View;
import android.widget.ImageView;

public class PanelListRow extends TwoLineRow {

    private final ImageView mIcon;

    public PanelListRow(Context context) {
        this(context, null);
    }

    public PanelListRow(Context context, AttributeSet attrs) {
        super(context, attrs);

        mIcon = (ImageView) findViewById(R.id.icon);
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

        int imageIndex = cursor.getColumnIndexOrThrow(HomeItems.IMAGE_URL);
        final String imageUrl = cursor.getString(imageIndex);

        Picasso.with(getContext())
               .load(imageUrl)
               .error(R.drawable.favicon)
               .into(mIcon);
    }
}
