





































package org.mozilla.gecko;

import org.mozilla.gecko.gfx.FloatSize;

import android.content.Context;
import android.util.Log;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
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

    private static enum PopupType { NONE, AUTOCOMPLETE, VALIDATION };

    
    private PopupType mTypeShowing = PopupType.NONE;

    public FormAssistPopup(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;

        mAnimation = AnimationUtils.loadAnimation(context, R.anim.grow_fade_in);
        mAnimation.setDuration(75);

        setFocusable(false);

        setOnItemClickListener(new OnItemClickListener() {
            public void onItemClick(AdapterView<?> parentView, View view, int position, long id) {
                if (mTypeShowing.equals(PopupType.AUTOCOMPLETE)) {
                    
                    
                    TextView textView = (TextView) view;
                    String value = (String) textView.getTag();
                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("FormAssist:AutoComplete", value));
                    hide();
                }
            }
        });

        GeckoAppShell.registerGeckoEventListener("FormAssist:AutoComplete", this);
        GeckoAppShell.registerGeckoEventListener("FormAssist:ValidationMessage", this);
        GeckoAppShell.registerGeckoEventListener("FormAssist:Hide", this);
    }

    public void handleMessage(String event, JSONObject message) {
        try {
            if (event.equals("FormAssist:AutoComplete")) {
                handleAutoCompleteMessage(message);
            } else if (event.equals("FormAssist:ValidationMessage")) {
                handleValidationMessage(message);
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
                showAutoCompleteSuggestions(suggestions, rect, zoom);
            }
        });
    }

    private void handleValidationMessage(JSONObject message) throws JSONException {
        final String validationMessage = message.getString("validationMessage");
        final JSONArray rect = message.getJSONArray("rect");
        final double zoom = message.getDouble("zoom");
        GeckoApp.mAppContext.mMainHandler.post(new Runnable() {
            public void run() {
                showValidationMessage(validationMessage, rect, zoom);
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

    private void showAutoCompleteSuggestions(JSONArray suggestions, JSONArray rect, double zoom) {
        AutoCompleteListAdapter adapter = new AutoCompleteListAdapter(mContext, R.layout.autocomplete_list_item);
        adapter.populateSuggestionsList(suggestions);
        setAdapter(adapter);

        if (positionAndShowPopup(rect, zoom))
            mTypeShowing = PopupType.AUTOCOMPLETE;
    }

    
    private void showValidationMessage(String validationMessage, JSONArray rect, double zoom) {
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext, R.layout.autocomplete_list_item);
        adapter.add(validationMessage);
        setAdapter(adapter);

        if (positionAndShowPopup(rect, zoom))
            mTypeShowing = PopupType.VALIDATION;
    }

    
    public boolean positionAndShowPopup(JSONArray rect, double zoom) {
        
        InputMethodManager imm =
                (InputMethodManager) GeckoApp.mAppContext.getSystemService(Context.INPUT_METHOD_SERVICE);
        if (imm.isFullscreenMode())
            return false;

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

        int popupWidth = mWidth;
        int popupHeight = mHeight;
        int popupLeft = left < 0 ? 0 : left;
        int popupTop = top + height;

        FloatSize viewport = GeckoApp.mAppContext.getLayerController().getViewportSize();

        
        if (sMinWidth == 0) {
            DisplayMetrics metrics = new DisplayMetrics();
            GeckoApp.mAppContext.getWindowManager().getDefaultDisplay().getMetrics(metrics);
            sMinWidth = (int) (POPUP_MIN_WIDTH_IN_DPI * metrics.density);
            sRowHeight = (int) (POPUP_ROW_HEIGHT_IN_DPI * metrics.density);
        }

        
        
        if ((left + width) < viewport.width) 
            popupWidth = left < 0 ? left + width : width;

        
        if (popupWidth >= 0 && popupWidth < sMinWidth) {
            popupWidth = sMinWidth;

            if ((popupLeft + popupWidth) > viewport.width)
                popupLeft = (int) (viewport.width - popupWidth);
        }

        popupHeight = sRowHeight * getAdapter().getCount();

        
        if ((popupTop + popupHeight) > viewport.height) {
            
            if ((viewport.height - popupTop) > top) {
                
                popupHeight = (int) (viewport.height - popupTop);
            } else {
                if (popupHeight < top) {
                    
                    popupTop = (top - popupHeight);
                } else {
                    
                    popupTop = 0;
                    popupHeight = top;
                }
           }
        }

        mLayout = new RelativeLayout.LayoutParams(popupWidth, popupHeight);
        mLayout.setMargins(popupLeft, popupTop, 0, 0);
        setLayoutParams(mLayout);
        requestLayout();

        return true;
    }

    public void hide() {
        if (isShown()) {
            setVisibility(View.GONE);
            mTypeShowing = PopupType.NONE;
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("FormAssist:Hidden", null));
        }
    }

    private class AutoCompleteListAdapter extends ArrayAdapter<Pair<String, String>> {
        private LayoutInflater mInflater;
        private int mTextViewResourceId;

        public AutoCompleteListAdapter(Context context, int textViewResourceId) {
            super(context, textViewResourceId);

            mInflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            mTextViewResourceId = textViewResourceId;
        }

        
        
        public void populateSuggestionsList(JSONArray suggestions) {
            try {
                for (int i = 0; i < suggestions.length(); i++) {
                    JSONObject suggestion = suggestions.getJSONObject(i);
                    String label = suggestion.getString("label");
                    String value = suggestion.getString("value");
                    add(new Pair<String, String>(label, value));
                }
            } catch (JSONException e) {
                Log.e(LOGTAG, "JSONException: " + e);
            }
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            if (convertView == null)
                convertView = mInflater.inflate(mTextViewResourceId, null);

            Pair<String, String> item = getItem(position);
            TextView itemView = (TextView) convertView;

            
            itemView.setText(item.first);

            
            itemView.setTag(item.second);

            return convertView;
        }
    }
}
