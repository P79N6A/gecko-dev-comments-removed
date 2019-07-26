



package org.mozilla.gecko;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;





public class RemoteTabsContainer extends FrameLayout
                                 implements TabsPanel.PanelView {
    private final Context context;
    private RemoteTabsList list;

    public RemoteTabsContainer(Context context, AttributeSet attrs) {
        super(context, attrs);
        this.context = context;
    }

    @Override
    public void addView(View child, int index, ViewGroup.LayoutParams params) {
        super.addView(child, index, params);

        list = (RemoteTabsList) child;
    }

    @Override
    public ViewGroup getLayout() {
        return this;
    }

    @Override
    public void setTabsPanel(TabsPanel panel) {
        list.setTabsPanel(panel);
    }

    @Override
    public void show() {
        setVisibility(VISIBLE);
        TabsAccessor.getTabs(context, list);
    }

    @Override
    public void hide() {
        setVisibility(GONE);
    }

    @Override
    public boolean shouldExpand() {
        return true;
    }
}
