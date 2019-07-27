



package org.mozilla.gecko.tabs;

import org.mozilla.gecko.R;
import org.mozilla.gecko.widget.TabThumbnailWrapper;

import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

public class TabsLayoutItemView {
    int id;
    TextView title;
    ImageView thumbnail;
    ImageButton close;
    ViewGroup info;
    TabThumbnailWrapper thumbnailWrapper;

    public TabsLayoutItemView(View view) {
        info = (ViewGroup) view;
        title = (TextView) view.findViewById(R.id.title);
        thumbnail = (ImageView) view.findViewById(R.id.thumbnail);
        close = (ImageButton) view.findViewById(R.id.close);
        thumbnailWrapper = (TabThumbnailWrapper) view.findViewById(R.id.wrapper);
    }
}