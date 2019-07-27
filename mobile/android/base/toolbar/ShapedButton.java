



package org.mozilla.gecko.toolbar;

import org.mozilla.gecko.GeckoApplication;
import org.mozilla.gecko.LightweightTheme;
import org.mozilla.gecko.LightweightThemeDrawable;
import org.mozilla.gecko.R;
import org.mozilla.gecko.tabs.TabCurve;
import org.mozilla.gecko.widget.ThemedImageButton;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Path;
import android.graphics.PorterDuff.Mode;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.StateListDrawable;
import android.util.AttributeSet;

public class ShapedButton extends ThemedImageButton
                          implements CanvasDelegate.DrawManager {
    protected final LightweightTheme mTheme;

    protected final Path mPath;
    protected final CanvasDelegate mCanvasDelegate;

    public ShapedButton(Context context, AttributeSet attrs) {
        super(context, attrs);
        mTheme = ((GeckoApplication) context.getApplicationContext()).getLightweightTheme();

        
        mPath = new Path();
        mCanvasDelegate = new CanvasDelegate(this, Mode.DST_IN);

        setWillNotDraw(false);
    }

    @Override
    public void draw(Canvas canvas) {
        if (mCanvasDelegate != null)
            mCanvasDelegate.draw(canvas, mPath, getWidth(), getHeight());
        else
            defaultDraw(canvas);
    }

    @Override
    public void defaultDraw(Canvas canvas) {
        super.draw(canvas);
    }

    
    @Override
    public void onLightweightThemeChanged() {
        final int background = getResources().getColor(R.color.background_tabs);
        final LightweightThemeDrawable lightWeight = mTheme.getColorDrawable(this, background);

        if (lightWeight == null)
            return;

        lightWeight.setAlpha(34, 34);

        final StateListDrawable stateList = new StateListDrawable();
        stateList.addState(PRESSED_ENABLED_STATE_SET, getColorDrawable(R.color.highlight_shaped));
        stateList.addState(FOCUSED_STATE_SET, getColorDrawable(R.color.highlight_shaped_focused));
        stateList.addState(PRIVATE_STATE_SET, getColorDrawable(R.color.background_tabs));
        stateList.addState(EMPTY_STATE_SET, lightWeight);

        setBackgroundDrawable(stateList);
    }

    @Override
    public void onLightweightThemeReset() {
        setBackgroundResource(R.drawable.shaped_button);
    }

    @Override
    public void setBackgroundDrawable(Drawable drawable) {
        if (getBackground() == null || drawable == null) {
            super.setBackgroundDrawable(drawable);
            return;
        }

        int[] padding =  new int[] { getPaddingLeft(),
                                     getPaddingTop(),
                                     getPaddingRight(),
                                     getPaddingBottom()
                                   };
        drawable.setLevel(getBackground().getLevel());
        super.setBackgroundDrawable(drawable);

        setPadding(padding[0], padding[1], padding[2], padding[3]);
    }

    @Override
    public void setBackgroundResource(int resId) {
        setBackgroundDrawable(getResources().getDrawable(resId));
    }
}
