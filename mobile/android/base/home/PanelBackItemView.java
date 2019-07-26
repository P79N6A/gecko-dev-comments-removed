




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;

import android.content.Context;
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

    public void updateFromFilter(String filter) {
        title.setText(filter);
    }
}
