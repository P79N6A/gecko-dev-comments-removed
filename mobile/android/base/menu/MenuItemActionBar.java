



package org.mozilla.gecko.menu;

import org.mozilla.gecko.NewTabletUI;
import org.mozilla.gecko.R;
import org.mozilla.gecko.widget.ThemedImageButton;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.widget.ImageButton;

public class MenuItemActionBar extends ThemedImageButton
                               implements GeckoMenuItem.Layout {
    private static final String LOGTAG = "GeckoMenuItemActionBar";

    public MenuItemActionBar(Context context) {
        this(context, null);
    }

    public MenuItemActionBar(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.menuItemActionBarStyle);
    }

    public MenuItemActionBar(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    public void initialize(GeckoMenuItem item) {
        if (item == null)
            return;

        setIcon(item.getIcon());
        setTitle(item.getTitle());
        setEnabled(item.isEnabled());
        setId(item.getItemId());
    }

    void setIcon(Drawable icon) {
        if (icon == null) {
            setVisibility(GONE);
        } else {
            setVisibility(VISIBLE);
            setImageDrawable(icon);
        }
    }

    void setIcon(int icon) {
        setIcon((icon == 0) ? null : getResources().getDrawable(icon));
    }

    void setTitle(CharSequence title) {
        
        setContentDescription(title);
    }

    @Override
    public void setEnabled(boolean enabled) {
        super.setEnabled(enabled);
        setColorFilter(enabled ? 0 : 0xFF999999);
    }

    @Override
    public void setShowIcon(boolean show) {
        
    }
}
