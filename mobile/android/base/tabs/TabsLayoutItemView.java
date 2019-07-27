



package org.mozilla.gecko.tabs;

import org.mozilla.gecko.NewTabletUI;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.widget.TabThumbnailWrapper;

import android.content.Context;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.TouchDelegate;
import android.view.View;
import android.view.ViewTreeObserver;
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

    private int mTabId;
    private TextView mTitle;
    private ImageView mThumbnail;
    private ImageButton mCloseButton;
    private TabThumbnailWrapper mThumbnailWrapper;

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
        mCloseButton.setOnClickListener(mOnClickListener);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mTitle = (TextView) findViewById(R.id.title);
        mThumbnail = (ImageView) findViewById(R.id.thumbnail);
        mCloseButton = (ImageButton) findViewById(R.id.close);
        mThumbnailWrapper = (TabThumbnailWrapper) findViewById(R.id.wrapper);

        if (NewTabletUI.isEnabled(getContext())) {
            growCloseButtonHitArea();
        }
    }

    private void growCloseButtonHitArea() {
        getViewTreeObserver().addOnPreDrawListener(new ViewTreeObserver.OnPreDrawListener() {
            @Override
            public boolean onPreDraw() {
                getViewTreeObserver().removeOnPreDrawListener(this);

                final Rect hitRect = new Rect();
                mCloseButton.getHitRect(hitRect);

                
                
                final int targetHitArea = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 40, getResources().getDisplayMetrics());;
                final View parent = ((View) mCloseButton.getParent());


                hitRect.top = 0;
                hitRect.right = getWidth();
                hitRect.left = getWidth() - targetHitArea;
                hitRect.bottom = parent.getHeight();

                setTouchDelegate(new TouchDelegate(hitRect, mCloseButton));

                return true;
            }
        });
    }

    protected void assignValues(Tab tab)  {
        if (tab == null) {
            return;
        }

        mTabId = tab.getId();

        Drawable thumbnailImage = tab.getThumbnail();
        mThumbnail.setImageDrawable(thumbnailImage);

        if (mThumbnailWrapper != null) {
            mThumbnailWrapper.setRecording(tab.isRecording());
        }
        mTitle.setText(tab.getDisplayTitle());
        mCloseButton.setTag(this);

    }

    public int getTabId() {
        return mTabId;
    }

    public void setThumbnail(Drawable thumbnail) {
        mThumbnail.setImageDrawable(thumbnail);
    }

    public void setCloseVisible(boolean visible) {
        mCloseButton.setVisibility(visible ? View.VISIBLE : View.INVISIBLE);
    }
}
