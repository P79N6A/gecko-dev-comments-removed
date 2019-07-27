




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.TextView;

public class BookmarkFolderView extends TextView {
    private static final int[] STATE_OPEN = { R.attr.state_open };

    private boolean mIsOpen;

    public BookmarkFolderView(Context context) {
        super(context);
    }

    public BookmarkFolderView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public BookmarkFolderView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    public int[] onCreateDrawableState(int extraSpace) {
        final int[] drawableState = super.onCreateDrawableState(extraSpace + 1);

        if (mIsOpen) {
            mergeDrawableStates(drawableState, STATE_OPEN);
        }

        return drawableState;
    }

    public void open() {
        if (!mIsOpen) {
            mIsOpen = true;
            refreshDrawableState();
        }
    }

    public void close() {
        if (mIsOpen) {
            mIsOpen = false;
            refreshDrawableState();
        }
    }
}
