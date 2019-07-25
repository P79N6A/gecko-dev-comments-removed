




package org.mozilla.gecko;

import android.content.Context;
import android.view.View;
import android.widget.TextView;
import android.widget.ImageView;
import android.widget.TabHost.TabContentFactory;

abstract public class AwesomeBarTab {
    abstract public String getTag();
    abstract public int getTitleStringId();
    abstract public void destroy();
    abstract public TabContentFactory getFactory();

    
    public static final int MAX_RESULTS = 100;
    protected Context mContext = null;

    public AwesomeBarTab(Context context) {
        mContext = context;
    }

    protected class AwesomeEntryViewHolder {
        public TextView titleView;
        public TextView urlView;
        public ImageView faviconView;
        public ImageView bookmarkIconView;
    }
}
