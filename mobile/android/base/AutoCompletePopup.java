





































package org.mozilla.gecko;

import org.mozilla.gecko.gfx.FloatSize;

import android.content.Context;
import android.util.Log;
import android.util.AttributeSet;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.ArrayAdapter;
import android.widget.AdapterView;
import android.widget.RelativeLayout;
import android.widget.ListView;
import android.widget.TextView;

import org.json.JSONArray;
import org.json.JSONException;

public class AutoCompletePopup extends ListView {
    private Context mContext;
    private RelativeLayout.LayoutParams mLayout;

    private int mWidth;
    private int mHeight;

    private Animation mAnimation; 

    private static final String LOGTAG = "AutoCompletePopup";

    public AutoCompletePopup(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;

        mAnimation = AnimationUtils.loadAnimation(context, R.anim.grow_fade_in);
        mAnimation.setDuration(75);

        setFocusable(false);

        setOnItemClickListener(new OnItemClickListener() {
            public void onItemClick(AdapterView<?> parentView, View view, int position, long id) {
                String value = ((TextView) view).getText().toString();
                GeckoAppShell.sendEventToGecko(new GeckoEvent("FormAssist:AutoComplete", value));
                hide();
            }
        });
    }

    public void show(JSONArray suggestions, JSONArray rect, double zoom) {
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext, R.layout.autocomplete_list_item);
        for (int i = 0; i < suggestions.length(); i++) {
            try {
                adapter.add(suggestions.get(i).toString());
            } catch (JSONException e) {
                Log.i(LOGTAG, "JSONException: " + e);
            }
        }

        setAdapter(adapter);

        if (isShown())
            return;

        setVisibility(View.VISIBLE);
        startAnimation(mAnimation);

        if (mLayout == null) {
            mLayout = (RelativeLayout.LayoutParams) getLayoutParams();
            mWidth = mLayout.width;
            mHeight = mLayout.height;
        }

        int left = 0;
        int top = 0; 
        int width = 0;
        int height = 0;

        try {
            left = (int) (rect.getDouble(0) * zoom);
            top = (int) (rect.getDouble(1) * zoom);
            width = (int) (rect.getDouble(2) * zoom);
            height = (int) (rect.getDouble(3) * zoom);
        } catch (JSONException e) { } 

        int listWidth = mWidth;
        int listHeight = mHeight;
        int listLeft = left < 0 ? 0 : left;
        int listTop = top + height;

        FloatSize viewport = GeckoApp.mAppContext.getLayerController().getViewportSize();

        
        
        if ((left + width) < viewport.width) 
            listWidth = left + width;

        
        
        if (((listTop + listHeight) > viewport.height) && (listHeight <= top))
            listTop = (top - listHeight);

        mLayout = new RelativeLayout.LayoutParams(listWidth, listHeight);
        mLayout.setMargins(listLeft, listTop, 0, 0);
        setLayoutParams(mLayout);
        requestLayout();
    }

    public void hide() {
        if (isShown()) {
            setVisibility(View.GONE);
            GeckoAppShell.sendEventToGecko(new GeckoEvent("FormAssist:Closed", null));
        }
    }
}
