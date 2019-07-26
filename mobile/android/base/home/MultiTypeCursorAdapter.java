




package org.mozilla.gecko.home;

import android.content.Context;
import android.database.Cursor;
import android.support.v4.widget.CursorAdapter;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;






abstract class MultiTypeCursorAdapter extends CursorAdapter {
    private final int[] mViewTypes;
    private final int[] mLayouts;

    
    abstract public void bindView(View view, Context context, int position);

    public MultiTypeCursorAdapter(Context context, Cursor cursor, int[] viewTypes, int[] layouts) {
        super(context, cursor, 0);

        if (viewTypes.length != layouts.length) {
            throw new IllegalStateException("The view types and the layouts should be of same size");
        }

        mViewTypes = viewTypes;
        mLayouts = layouts;
    }

    @Override
    public final int getViewTypeCount() {
        return mViewTypes.length;
    }

    


    public final Cursor getCursor(int position) {
        final Cursor cursor = getCursor();
        if (cursor == null || !cursor.moveToPosition(position)) {
            throw new IllegalStateException("Couldn't move cursor to position " + position);
        }

        return cursor;
    }

    @Override
    public final View getView(int position, View convertView, ViewGroup parent) {
        final Context context = parent.getContext();
        if (convertView == null) {
            convertView = newView(context, position, parent);
        }

        bindView(convertView, context, position);
        return convertView;
    }

    @Override
    public final void bindView(View view, Context context, Cursor cursor) {
        
    }

    @Override
    public final View newView(Context context, Cursor cursor, ViewGroup parent) {
        return null;
    }

    






    private View newView(Context context, int position, ViewGroup parent) {
        final int type = getItemViewType(position);
        final int count = mViewTypes.length;

        for (int i = 0; i < count; i++) {
            if (mViewTypes[i] == type) {
                return LayoutInflater.from(context).inflate(mLayouts[i], parent, false);
            }
        }

        return null;
    }
}
