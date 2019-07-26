




package org.mozilla.gecko;

import org.mozilla.gecko.gfx.LayerView;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.widget.LinearLayout;

public class BrowserToolbarLayout extends LinearLayout {
    private static final String LOGTAG = "GeckoToolbarLayout";

    public BrowserToolbarLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        
        
        if (event != null && event.getY() > getHeight() - getScrollY()) {
            return false;
        }

        return super.onTouchEvent(event);
    }

    @Override
    protected void onScrollChanged(int l, int t, int oldl, int oldt) {
        super.onScrollChanged(l, t, oldl, oldt);

        if (t != oldt) {
            refreshMargins();
        }
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);

        if (h != oldh) {
            refreshMargins();
        }
    }

    public void refreshMargins() {
        LayerView view = GeckoApp.mAppContext.getLayerView();
        if (view != null) {
            view.getLayerClient().setFixedLayerMargins(0, getHeight() - getScrollY(), 0, 0);
        }
    }
}

