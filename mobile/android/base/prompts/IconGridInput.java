




package org.mozilla.gecko.prompts;

import java.util.ArrayList;
import java.util.List;

import org.json.JSONArray;
import org.json.JSONObject;
import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.R;
import org.mozilla.gecko.gfx.BitmapUtils;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.TextView;

public class IconGridInput extends PromptInput implements OnItemClickListener {
    public static final String INPUT_TYPE = "icongrid";
    public static final String LOGTAG = "GeckoIconGridInput";

    private ArrayAdapter<IconGridItem> mAdapter; 

    private static int mColumnWidth = -1;  
    private static int mMaxColumns = -1;  
    private static int mIconSize = -1;    
    private int mSelected;                
    private final JSONArray mArray;

    public IconGridInput(JSONObject obj) {
        super(obj);
        mArray = obj.optJSONArray("items");
    }

    @Override
    public View getView(Context context) throws UnsupportedOperationException {
        if (mColumnWidth < 0) {
            
            mColumnWidth = context.getResources().getDimensionPixelSize(R.dimen.icongrid_columnwidth);
        }

        if (mIconSize < 0) {
            mIconSize = GeckoAppShell.getPreferredIconSize();
        }

        if (mMaxColumns < 0) {
            mMaxColumns = context.getResources().getInteger(R.integer.max_icon_grid_columns);
        }

        
        final WindowManager wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        final Display display = wm.getDefaultDisplay();
        final int screenWidth = display.getWidth();
        int maxColumns = Math.min(mMaxColumns, screenWidth / mColumnWidth);

        final GridView view = (GridView) LayoutInflater.from(context).inflate(R.layout.icon_grid, null, false);
        view.setColumnWidth(mColumnWidth);

        final ArrayList<IconGridItem> items = new ArrayList<IconGridItem>(mArray.length());
        for (int i = 0; i < mArray.length(); i++) {
            IconGridItem item = new IconGridItem(context, mArray.optJSONObject(i));
            items.add(item);
            if (item.selected) {
                mSelected = i;
            }
        }

        view.setNumColumns(Math.min(items.size(), maxColumns));
        view.setOnItemClickListener(this);
        
        
        
        if (Versions.feature11Plus && mSelected > -1) {
            view.setItemChecked(mSelected, true);
        }

        mAdapter = new IconGridAdapter(context, -1, items);
        view.setAdapter(mAdapter);
        mView = view;
        return mView;
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        mSelected = position;
        notifyListeners(Integer.toString(position));
    }

    @Override
    public Object getValue() {
        return mSelected;
    }

    @Override
    public boolean getScrollable() {
        return true;
    }

    private class IconGridAdapter extends ArrayAdapter<IconGridItem> {
        public IconGridAdapter(Context context, int resource, List<IconGridItem> items) {
            super(context, resource, items);
        }

        @Override
        public View getView(int position, View convert, ViewGroup parent) {
            final Context context = parent.getContext();
            if (convert == null) {
                convert = LayoutInflater.from(context).inflate(R.layout.icon_grid_item, parent, false);
            }
            bindView(convert, context, position);
            return convert;
        }

        private void bindView(View v, Context c, int position) {
            final IconGridItem item = getItem(position);
            final TextView text1 = (TextView) v.findViewById(android.R.id.text1);
            text1.setText(item.label);

            final TextView text2 = (TextView) v.findViewById(android.R.id.text2);
            if (TextUtils.isEmpty(item.description)) {
                text2.setVisibility(View.GONE);
            } else {
                text2.setVisibility(View.VISIBLE);
                text2.setText(item.description);
            }

            final ImageView icon = (ImageView) v.findViewById(R.id.icon);
            icon.setImageDrawable(item.icon);
            ViewGroup.LayoutParams lp = icon.getLayoutParams();
            lp.width = lp.height = mIconSize;
        }
    }
 
    private class IconGridItem {
        final String label;
        final String description;
        final boolean selected;
        Drawable icon;

        public IconGridItem(final Context context, final JSONObject obj) {
            label = obj.optString("name");
            final String iconUrl = obj.optString("iconUri");
            description = obj.optString("description");
            selected = obj.optBoolean("selected");

            BitmapUtils.getDrawable(context, iconUrl, new BitmapUtils.BitmapLoader() {
                @Override
                public void onBitmapFound(Drawable d) {
                    icon = d;
                    if (mAdapter != null) {
                        mAdapter.notifyDataSetChanged();
                    }
                }
            });
        }
    }
}
