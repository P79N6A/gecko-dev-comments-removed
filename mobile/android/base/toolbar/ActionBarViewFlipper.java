




package org.mozilla.gecko.toolbar;

import org.mozilla.gecko.NewTabletUI;
import org.mozilla.gecko.R;
import org.mozilla.gecko.widget.GeckoViewFlipper;

import android.content.Context;
import android.util.AttributeSet;
import android.view.ViewGroup;







public class ActionBarViewFlipper extends GeckoViewFlipper {

    public ActionBarViewFlipper(final Context context, final AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void onAttachedToWindow() {
        if (NewTabletUI.isEnabled(getContext())) {
            final ViewGroup.LayoutParams lp = getLayoutParams();
            lp.height = getResources().getDimensionPixelSize(R.dimen.new_tablet_browser_toolbar_height);
            setLayoutParams(lp);
        }
    }
}
