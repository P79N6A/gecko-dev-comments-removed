





































package org.mozilla.gecko;

import org.mozilla.gecko.gfx.FloatSize;

import android.content.Context;
import android.util.Log;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.view.inputmethod.InputMethodManager;
import android.widget.ArrayAdapter;
import android.widget.AdapterView;
import android.widget.RelativeLayout;
import android.widget.ListView;
import android.widget.TextView;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class FormAssistPopup extends ListView implements GeckoEventListener {
    private Context mContext;
    private RelativeLayout.LayoutParams mLayout;

    private int mWidth;
    private int mHeight;

    private Animation mAnimation; 

    private static final String LOGTAG = "FormAssistPopup";

    private static int sMinWidth = 0;
    private static int sRowHeight = 0;
    private static final int POPUP_MIN_WIDTH_IN_DPI = 200;
    private static final int POPUP_ROW_HEIGHT_IN_DPI = 32;

    public FormAssistPopup(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;

        mAnimation = AnimationUtils.loadAnimation(context, R.anim.grow_fade_in);
        mAnimation.setDuration(75);

        setFocusable(false);

        setOnItemClickListener(new OnItemClickListener() {
            public void onItemClick(AdapterView<?> parentView, View view, int position, long id) {
                String value = ((TextView) view).getText().toString();
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("FormAssist:AutoComplete", value));
                hide();
            }
        });

        GeckoAppShell.registerGeckoEventListener("FormAssist:AutoComplete", this);
        GeckoAppShell.registerGeckoEventListener("FormAssist:Hide", this);
    }

    public void handleMessage(String event, JSONObject message) {
        try {
            if (event.equals("FormAssist:AutoComplete")) {
                handleAutoCompleteMessage(message);
            } else if (event.equals("FormAssist:Hide")) {
                handleHideMessage(message);
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    private void handleAutoCompleteMessage(JSONObject message) throws JSONException  {
        final JSONArray suggestions = message.getJSONArray("suggestions");
        final JSONArray rect = message.getJSONArray("rect");
        final double zoom = message.getDouble("zoom");
        GeckoApp.mAppContext.mMainHandler.post(new Runnable() {
            public void run() {
                
                InputMethodManager imm =
                        (InputMethodManager) GeckoApp.mAppContext.getSystemService(Context.INPUT_METHOD_SERVICE);
                if (!imm.isFullscreenMode())
                    show(suggestions, rect, zoom);
            }
        });
    }

    private void handleHideMessage(JSONObject message) {
        GeckoApp.mAppContext.mMainHandler.post(new Runnable() {
            public void run() {
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

        if (!isShown()) {
            setVisibility(View.VISIBLE);
            startAnimation(mAnimation);
        }

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

        
        if (sMinWidth == 0) {
            DisplayMetrics metrics = new DisplayMetrics();
            GeckoApp.mAppContext.getWindowManager().getDefaultDisplay().getMetrics(metrics);
            sMinWidth = (int) (POPUP_MIN_WIDTH_IN_DPI * metrics.density);
            sRowHeight = (int) (POPUP_ROW_HEIGHT_IN_DPI * metrics.density);
        }

        
        
        if ((left + width) < viewport.width) 
            listWidth = left < 0 ? left + width : width;

        
        if (listWidth >= 0 && listWidth < sMinWidth) {
            listWidth = sMinWidth;

            if ((listLeft + listWidth) > viewport.width)
                listLeft = (int) (viewport.width - listWidth);
        }

        listHeight = sRowHeight * adapter.getCount();

        
        if ((listTop + listHeight) > viewport.height) {
            
            if ((viewport.height - listTop) > top) {
                
                listHeight = (int) (viewport.height - listTop);
            } else {
                if (listHeight < top) {
                    
                    listTop = (top - listHeight);
                } else {
                    
                    listTop = 0;
                    listHeight = top;
                }
           }
        }

        mLayout = new RelativeLayout.LayoutParams(listWidth, listHeight);
        mLayout.setMargins(listLeft, listTop, 0, 0);
        setLayoutParams(mLayout);
        requestLayout();
    }

    public void hide() {
        if (isShown()) {
            setVisibility(View.GONE);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("FormAssist:Hidden", null));
        }
    }
}
