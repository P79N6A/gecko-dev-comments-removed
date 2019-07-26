




package org.mozilla.gecko;

import android.content.Context;

public class PrivateTab extends Tab {
    public PrivateTab(Context context, int id, String url, boolean external, int parentId, String title) {
        super(context, id, url, external, parentId, title);
    }

    @Override
    protected void saveThumbnailToDB() {}

    @Override
    public boolean isPrivate() {
        return true;
    }
}
