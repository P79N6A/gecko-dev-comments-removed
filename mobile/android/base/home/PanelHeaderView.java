package org.mozilla.gecko.home;

import android.annotation.SuppressLint;
import android.content.Context;
import android.widget.ImageView;

@SuppressLint("ViewConstructor") 
public class PanelHeaderView extends ImageView {
    public PanelHeaderView(Context context, HomeConfig.HeaderConfig config) {
        super(context);

        setAdjustViewBounds(true);

        ImageLoader.with(context)
            .load(config.getImageUrl())
            .into(this);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = MeasureSpec.getSize(widthMeasureSpec);

        
        widthMeasureSpec = MeasureSpec.makeMeasureSpec(width, MeasureSpec.EXACTLY);

        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }
}
