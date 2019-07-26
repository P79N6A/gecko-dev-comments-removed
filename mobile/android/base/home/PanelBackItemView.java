




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.home.PanelLayout.FilterDetail;

import android.content.Context;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.widget.LinearLayout;
import android.widget.TextView;

class PanelBackItemView extends LinearLayout {
    private final TextView title;

    public PanelBackItemView(Context context) {
        super(context);

        LayoutInflater.from(context).inflate(R.layout.panel_back_item, this);
        setOrientation(HORIZONTAL);

        title = (TextView) findViewById(R.id.title);
    }

    public void updateFromFilter(FilterDetail filter) {
        final String backText = getResources()
            .getString(R.string.home_move_up_to_filter, filter.title);
        title.setText(backText);
    }
}
