




package org.mozilla.gecko.home;

import android.content.Context;
import android.util.AttributeSet;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.ExpandableListView;









public class HomeExpandableListView extends ExpandableListView
                                    implements OnItemLongClickListener {

    
    private HomeContextMenuInfo mContextMenuInfo;

    
    private HomeContextMenuInfo.ExpandableFactory mContextMenuInfoFactory;

    public HomeExpandableListView(Context context) {
        this(context, null);
    }

    public HomeExpandableListView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public HomeExpandableListView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        setOnItemLongClickListener(this);
    }

    @Override
    public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
        if (mContextMenuInfoFactory == null) {
            return false;
        }

        
        
        
        mContextMenuInfo = mContextMenuInfoFactory.makeInfoForAdapter(view, position, id, getExpandableListAdapter());
        return showContextMenuForChild(HomeExpandableListView.this);
    }

    @Override
    public ContextMenuInfo getContextMenuInfo() {
        return mContextMenuInfo;
    }

    public void setContextMenuInfoFactory(final HomeContextMenuInfo.ExpandableFactory factory) {
        mContextMenuInfoFactory = factory;
    }
}
