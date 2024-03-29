





package org.mozilla.gecko.widget;

import org.mozilla.gecko.GeckoApplication;
import org.mozilla.gecko.R;
import org.mozilla.gecko.lwt.LightweightTheme;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.drawable.ColorDrawable;
import android.util.AttributeSet;

public class ThemedTextView extends android.widget.TextView
                                     implements LightweightTheme.OnChangeListener {
    private LightweightTheme mTheme;

    private static final int[] STATE_PRIVATE_MODE = { R.attr.state_private };
    private static final int[] STATE_LIGHT = { R.attr.state_light };
    private static final int[] STATE_DARK = { R.attr.state_dark };

    protected static final int[] PRIVATE_PRESSED_STATE_SET = { R.attr.state_private, android.R.attr.state_pressed };
    protected static final int[] PRIVATE_FOCUSED_STATE_SET = { R.attr.state_private, android.R.attr.state_focused };
    protected static final int[] PRIVATE_STATE_SET = { R.attr.state_private };

    private boolean mIsPrivate;
    private boolean mIsLight;
    private boolean mIsDark;
    private boolean mAutoUpdateTheme;        

    public ThemedTextView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initialize(context, attrs);
    }

    public ThemedTextView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initialize(context, attrs);
    }

    private void initialize(final Context context, final AttributeSet attrs) {
        
        
        final Context applicationContext = context.getApplicationContext();
        if (applicationContext instanceof GeckoApplication) {
            mTheme = ((GeckoApplication) applicationContext).getLightweightTheme();
        }

        final TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.LightweightTheme);
        mAutoUpdateTheme = mTheme != null && a.getBoolean(R.styleable.LightweightTheme_autoUpdateTheme, true);
        a.recycle();
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        if (mAutoUpdateTheme)
            mTheme.addListener(this);
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();

        if (mAutoUpdateTheme)
            mTheme.removeListener(this);
    }

    @Override
    public int[] onCreateDrawableState(int extraSpace) {
        final int[] drawableState = super.onCreateDrawableState(extraSpace + 1);

        if (mIsPrivate)
            mergeDrawableStates(drawableState, STATE_PRIVATE_MODE);
        else if (mIsLight)
            mergeDrawableStates(drawableState, STATE_LIGHT);
        else if (mIsDark)
            mergeDrawableStates(drawableState, STATE_DARK);

        return drawableState;
    }

    @Override
    public void onLightweightThemeChanged() {
        if (mAutoUpdateTheme && mTheme.isEnabled())
            setTheme(mTheme.isLightTheme());
    }

    @Override
    public void onLightweightThemeReset() {
        if (mAutoUpdateTheme)
            resetTheme();
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);
        onLightweightThemeChanged();
    }

    public boolean isPrivateMode() {
        return mIsPrivate;
    }

    public void setPrivateMode(boolean isPrivate) {
        if (mIsPrivate != isPrivate) {
            mIsPrivate = isPrivate;
            refreshDrawableState();
        }
    }

    public void setTheme(boolean isLight) {
        
        if ((isLight && mIsLight != isLight) ||
            (!isLight && mIsDark == isLight)) {
            if (isLight) {
                mIsLight = true;
                mIsDark = false;
            } else {
                mIsLight = false;
                mIsDark = true;
            }

            refreshDrawableState();
        }
    }

    public void resetTheme() {
        if (mIsLight || mIsDark) {
            mIsLight = false;
            mIsDark = false;
            refreshDrawableState();
        }
    }

    public void setAutoUpdateTheme(boolean autoUpdateTheme) {
        if (mTheme == null) {
            return;
        }

        if (mAutoUpdateTheme != autoUpdateTheme) {
            mAutoUpdateTheme = autoUpdateTheme;

            if (mAutoUpdateTheme)
                mTheme.addListener(this);
            else
                mTheme.removeListener(this);
        }
    }

    public ColorDrawable getColorDrawable(int id) {
        return new ColorDrawable(getResources().getColor(id));
    }

    protected LightweightTheme getTheme() {
        return mTheme;
    }
}
