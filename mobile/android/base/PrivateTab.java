




package org.mozilla.gecko;

import android.content.Context;

public class PrivateTab extends Tab {
    public PrivateTab(Context context, int id, String url, boolean external, int parentId, String title) {
        super(context, id, url, external, parentId, title);

        
        
        final int bgColor = context.getResources().getColor(R.color.background_private);
        setBackgroundColor(bgColor);
    }

    @Override
    protected void saveThumbnailToDB() {}

    @Override
    public boolean isPrivate() {
        return true;
    }
}
