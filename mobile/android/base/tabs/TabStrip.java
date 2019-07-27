




package org.mozilla.gecko.tabs;

import android.content.Context;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.TouchDelegate;
import android.view.View;
import android.view.ViewTreeObserver;
import android.widget.ImageButton;

import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.widget.ThemedLinearLayout;

public class TabStrip extends ThemedLinearLayout {
    private static final String LOGTAG = "GeckoTabStrip";

    private static final int IMAGE_LEVEL_NORMAL = 0;
    private static final int IMAGE_LEVEL_PRIVATE = 1;

    private final TabStripView tabStripView;
    private final ImageButton addTabButton;

    private final TabsListener tabsListener;

    public TabStrip(Context context) {
        this(context, null);
    }

    public TabStrip(Context context, AttributeSet attrs) {
        super(context, attrs);
        setOrientation(HORIZONTAL);

        LayoutInflater.from(context).inflate(R.layout.tab_strip, this);
        tabStripView = (TabStripView) findViewById(R.id.tab_strip);

        addTabButton = (ImageButton) findViewById(R.id.add_tab);
        addTabButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                final Tabs tabs = Tabs.getInstance();
                if (isPrivateMode()) {
                    tabs.addPrivateTab();
                } else {
                    tabs.addTab();
                }
            }
        });

        getViewTreeObserver().addOnPreDrawListener(new ViewTreeObserver.OnPreDrawListener() {
                @Override
                public boolean onPreDraw() {
                    getViewTreeObserver().removeOnPreDrawListener(this);

                    final Rect r = new Rect();
                    r.left = addTabButton.getRight();
                    r.right = getWidth();
                    r.top = 0;
                    r.bottom = getHeight();

                    
                    
                    setTouchDelegate(new TouchDelegate(r, addTabButton));

                    return true;
                }
            });

        tabsListener = new TabsListener();
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        Tabs.registerOnTabsChangedListener(tabsListener);
        tabStripView.refreshTabs();
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();

        Tabs.unregisterOnTabsChangedListener(tabsListener);
        tabStripView.clearTabs();
    }

    @Override
    public void setPrivateMode(boolean isPrivate) {
        super.setPrivateMode(isPrivate);
        addTabButton.setImageLevel(isPrivate ? IMAGE_LEVEL_PRIVATE : IMAGE_LEVEL_NORMAL);
    }

    private class TabsListener implements Tabs.OnTabsChangedListener {
        @Override
        public void onTabChanged(Tab tab, Tabs.TabEvents msg, Object data) {
            switch (msg) {
                case RESTORED:
                    tabStripView.restoreTabs();
                    break;

                case ADDED:
                    tabStripView.addTab(tab);
                    break;

                case CLOSED:
                    tabStripView.removeTab(tab);
                    break;

                case SELECTED:
                    
                    tabStripView.selectTab(tab);
                    setPrivateMode(tab.isPrivate());
                case UNSELECTED:
                    
                case TITLE:
                case FAVICON:
                case RECORDING_CHANGE:
                    tabStripView.updateTab(tab);
                    break;
            }
        }
    }
}
