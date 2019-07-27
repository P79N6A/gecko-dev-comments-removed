




package org.mozilla.gecko;

import android.content.Context;

import org.mozilla.gecko.db.BrowserDB;

public class PrivateTab extends Tab {
    public PrivateTab(Context context, int id, String url, boolean external, int parentId, String title) {
        super(context, id, url, external, parentId, title);

        
        
        final int bgColor = context.getResources().getColor(R.color.private_toolbar_grey);
        setBackgroundColor(bgColor);
    }

    @Override
    protected void saveThumbnailToDB(final BrowserDB db) {}

    @Override
    public boolean isPrivate() {
        return true;
    }
}
