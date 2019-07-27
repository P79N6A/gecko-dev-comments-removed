




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.home.PanelLayout.FilterDetail;

import android.content.Context;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.squareup.picasso.Picasso;

class PanelBackItemView extends LinearLayout {
    private final TextView title;

    public PanelBackItemView(Context context, String backImageUrl) {
        super(context);

        LayoutInflater.from(context).inflate(R.layout.panel_back_item, this);
        setOrientation(HORIZONTAL);

        title = (TextView) findViewById(R.id.title);

        final ImageView image = (ImageView) findViewById(R.id.image);

        if (TextUtils.isEmpty(backImageUrl)) {
            image.setImageResource(R.drawable.folder_up);
        } else {
            ImageLoader.with(getContext())
                       .load(backImageUrl)
                       .placeholder(R.drawable.folder_up)
                       .into(image);
        }
    }

    public void updateFromFilter(FilterDetail filter) {
        final String backText = getResources()
            .getString(R.string.home_move_up_to_filter, filter.title);
        title.setText(backText);
    }
}
