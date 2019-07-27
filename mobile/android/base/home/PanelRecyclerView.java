




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.home.PanelLayout.DatasetBacked;
import org.mozilla.gecko.home.PanelLayout.PanelView;
import org.mozilla.gecko.home.RecyclerViewItemClickListener.OnClickListener;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.Resources;
import android.database.Cursor;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.View;




@SuppressLint("ViewConstructor") 
public class PanelRecyclerView extends RecyclerView implements DatasetBacked, PanelView, OnClickListener {
    private final PanelRecyclerViewAdapter adapter;
    private final GridLayoutManager layoutManager;
    private final PanelViewItemHandler itemHandler;
    private final float columnWidth;
    private final boolean autoFit;

    private PanelLayout.OnItemOpenListener itemOpenListener;
    private HomeContextMenuInfo contextMenuInfo;
    private HomeContextMenuInfo.Factory contextMenuInfoFactory;

    public PanelRecyclerView(Context context, HomeConfig.ViewConfig viewConfig) {
        super(context);

        final Resources resources = context.getResources();

        int spanCount;
        if (viewConfig.getItemType() == HomeConfig.ItemType.ICON) {
            autoFit = false;
            spanCount = getResources().getInteger(R.integer.panel_icon_grid_view_columns);
        } else {
            autoFit = true;
            spanCount = 1;
        }

        columnWidth = resources.getDimension(R.dimen.panel_grid_view_column_width);
        layoutManager = new GridLayoutManager(context, spanCount);
        adapter = new PanelRecyclerViewAdapter(context, viewConfig);
        itemHandler = new PanelViewItemHandler();

        setLayoutManager(layoutManager);
        setAdapter(adapter);

        int horizontalSpacing = (int) resources.getDimension(R.dimen.panel_grid_view_horizontal_spacing);
        int verticalSpacing = (int) resources.getDimension(R.dimen.panel_grid_view_vertical_spacing);
        int outerSpacing = (int) resources.getDimension(R.dimen.panel_grid_view_outer_spacing);

        addItemDecoration(new SpacingDecoration(horizontalSpacing, verticalSpacing));

        setPadding(outerSpacing, outerSpacing, outerSpacing, outerSpacing);
        setClipToPadding(false);

        addOnItemTouchListener(new RecyclerViewItemClickListener(context, this, this));
    }

    @Override
    protected void onMeasure(int widthSpec, int heightSpec) {
        super.onMeasure(widthSpec, heightSpec);

        if (autoFit) {
            
            final int spanCount = (int) Math.max(1, getMeasuredWidth() / columnWidth);
            layoutManager.setSpanCount(spanCount);
        }
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();
        itemHandler.setOnItemOpenListener(itemOpenListener);
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        itemHandler.setOnItemOpenListener(null);
    }

    @Override
    public void setDataset(Cursor cursor) {
        adapter.swapCursor(cursor);
    }

    @Override
    public void setFilterManager(PanelLayout.FilterManager manager) {
        adapter.setFilterManager(manager);
        itemHandler.setFilterManager(manager);
    }

    @Override
    public void setOnItemOpenListener(PanelLayout.OnItemOpenListener listener) {
        itemOpenListener = listener;
        itemHandler.setOnItemOpenListener(listener);
    }

    @Override
    public HomeContextMenuInfo getContextMenuInfo() {
        return contextMenuInfo;
    }

    @Override
    public void setContextMenuInfoFactory(HomeContextMenuInfo.Factory factory) {
        contextMenuInfoFactory = factory;
    }

    @Override
    public void onClick(View view, int position) {
        itemHandler.openItemAtPosition(adapter.getCursor(), position);
    }

    @Override
    public void onLongClick(View view, int position) {
        Cursor cursor = adapter.getCursor();
        cursor.moveToPosition(position);

        contextMenuInfo = contextMenuInfoFactory.makeInfoForCursor(view, position, -1, cursor);
        showContextMenuForChild(PanelRecyclerView.this);
    }
}
