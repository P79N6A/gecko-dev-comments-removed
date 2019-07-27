



package org.mozilla.gecko.tabs;

import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.widget.TabThumbnailWrapper;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.View;
import android.widget.Checkable;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

public class TabsLayoutItemView extends LinearLayout
                                implements Checkable {
    private static final String LOGTAG = "Gecko" + TabsLayoutItemView.class.getSimpleName();
    private static final int[] STATE_CHECKED = { android.R.attr.state_checked };
    private boolean mChecked;

    
    
    int id;
    TextView title;
    ImageView thumbnail;
    ImageButton close;
    TabThumbnailWrapper thumbnailWrapper;

    public TabsLayoutItemView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public int[] onCreateDrawableState(int extraSpace) {
        final int[] drawableState = super.onCreateDrawableState(extraSpace + 1);

        if (mChecked) {
            mergeDrawableStates(drawableState, STATE_CHECKED);
        }

        return drawableState;
    }

    @Override
    public boolean isChecked() {
        return mChecked;
    }

    @Override
    public void setChecked(boolean checked) {
        if (mChecked == checked) {
            return;
        }

        mChecked = checked;
        refreshDrawableState();

        int count = getChildCount();
        for (int i = 0; i < count; i++) {
            final View child = getChildAt(i);
            if (child instanceof Checkable) {
                ((Checkable) child).setChecked(checked);
            }
        }
    }

    @Override
    public void toggle() {
        mChecked = !mChecked;
    }

    public void setCloseOnClickListener(OnClickListener mOnClickListener) {
        close.setOnClickListener(mOnClickListener);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        title = (TextView) findViewById(R.id.title);
        thumbnail = (ImageView) findViewById(R.id.thumbnail);
        close = (ImageButton) findViewById(R.id.close);
        thumbnailWrapper = (TabThumbnailWrapper) findViewById(R.id.wrapper);
    }

    protected void assignValues(Tab tab)  {
        if (tab == null) {
            return;
        }

        id = tab.getId();

        Drawable thumbnailImage = tab.getThumbnail();
        if (thumbnailImage != null) {
            thumbnail.setImageDrawable(thumbnailImage);
        } else {
            thumbnail.setImageResource(R.drawable.tab_thumbnail_default);
        }
        if (thumbnailWrapper != null) {
            thumbnailWrapper.setRecording(tab.isRecording());
        }
        title.setText(tab.getDisplayTitle());
        close.setTag(this);
    }
}
