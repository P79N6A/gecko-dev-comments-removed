




package org.mozilla.gecko;

import org.mozilla.gecko.gfx.FloatSize;
import org.mozilla.gecko.gfx.ImmutableViewportMetrics;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.content.res.Resources;
import android.util.AttributeSet;
import android.util.Log;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import java.util.Arrays;
import java.util.Collection;

public class FormAssistPopup extends RelativeLayout implements GeckoEventListener {
    private Context mContext;
    private Animation mAnimation;

    private ListView mAutoCompleteList;
    private RelativeLayout mValidationMessage;
    private TextView mValidationMessageText;
    private ImageView mValidationMessageArrow;
    private ImageView mValidationMessageArrowInverted;

    private static int sAutoCompleteMinWidth = 0;
    private static int sAutoCompleteRowHeight = 0;
    private static int sValidationMessageHeight = 0;
    private static int sValidationTextMarginTop = 0;
    private static RelativeLayout.LayoutParams sValidationTextLayoutNormal;
    private static RelativeLayout.LayoutParams sValidationTextLayoutInverted;

    private static final String LOGTAG = "GeckoFormAssistPopup";

    
    private static final Collection<String> sInputMethodBlocklist = Arrays.asList(new String[] {
                                            InputMethods.METHOD_GOOGLE_JAPANESE_INPUT, 
                                            InputMethods.METHOD_OPENWNN_PLUS,          
                                            InputMethods.METHOD_SIMEJI,                
                                            InputMethods.METHOD_SWYPE,                 
                                            InputMethods.METHOD_SWYPE_BETA,            
                                            });

    public FormAssistPopup(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;

        mAnimation = AnimationUtils.loadAnimation(context, R.anim.grow_fade_in);
        mAnimation.setDuration(75);

        setFocusable(false);

        registerEventListener("FormAssist:AutoComplete");
        registerEventListener("FormAssist:ValidationMessage");
        registerEventListener("FormAssist:Hide");
    }

    void destroy() {
        unregisterEventListener("FormAssist:AutoComplete");
        unregisterEventListener("FormAssist:ValidationMessage");
        unregisterEventListener("FormAssist:Hide");
    }

    @Override
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
        final JSONObject rect = message.getJSONObject("rect");
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                showAutoCompleteSuggestions(suggestions, rect);
            }
        });
    }

    private void handleValidationMessage(JSONObject message) throws JSONException {
        final String validationMessage = message.getString("validationMessage");
        final JSONObject rect = message.getJSONObject("rect");
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                showValidationMessage(validationMessage, rect);
            }
        });
    }

    private void handleHideMessage(JSONObject message) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                hide();
            }
        });
    }

    private void showAutoCompleteSuggestions(JSONArray suggestions, JSONObject rect) {
        if (mAutoCompleteList == null) {
            LayoutInflater inflater = LayoutInflater.from(mContext);
            mAutoCompleteList = (ListView) inflater.inflate(R.layout.autocomplete_list, null);

            mAutoCompleteList.setOnItemClickListener(new OnItemClickListener() {
                @Override
                public void onItemClick(AdapterView<?> parentView, View view, int position, long id) {
                    
                    
                    TextView textView = (TextView) view;
                    String value = (String) textView.getTag();
                    broadcastGeckoEvent("FormAssist:AutoComplete", value);
                    hide();
                }
            });

            addView(mAutoCompleteList);
        }
        
        AutoCompleteListAdapter adapter = new AutoCompleteListAdapter(mContext, R.layout.autocomplete_list_item);
        adapter.populateSuggestionsList(suggestions);
        mAutoCompleteList.setAdapter(adapter);

        positionAndShowPopup(rect, true);
    }

    private void showValidationMessage(String validationMessage, JSONObject rect) {
        if (mValidationMessage == null) {
            LayoutInflater inflater = LayoutInflater.from(mContext);
            mValidationMessage = (RelativeLayout) inflater.inflate(R.layout.validation_message, null);

            addView(mValidationMessage);
            mValidationMessageText = (TextView) mValidationMessage.findViewById(R.id.validation_message_text);

            sValidationTextMarginTop = (int) (mContext.getResources().getDimension(R.dimen.validation_message_margin_top));

            sValidationTextLayoutNormal = new RelativeLayout.LayoutParams(mValidationMessageText.getLayoutParams());
            sValidationTextLayoutNormal.setMargins(0, sValidationTextMarginTop, 0, 0);

            sValidationTextLayoutInverted = new RelativeLayout.LayoutParams(sValidationTextLayoutNormal);
            sValidationTextLayoutInverted.setMargins(0, 0, 0, 0);

            mValidationMessageArrow = (ImageView) mValidationMessage.findViewById(R.id.validation_message_arrow);
            mValidationMessageArrowInverted = (ImageView) mValidationMessage.findViewById(R.id.validation_message_arrow_inverted);
        }

        mValidationMessageText.setText(validationMessage);

        
        mValidationMessageText.setSelected(true);

        positionAndShowPopup(rect, false);
    }

    private void positionAndShowPopup(JSONObject rect, boolean isAutoComplete) {
        
        InputMethodManager imm =
                (InputMethodManager) GeckoApp.mAppContext.getSystemService(Context.INPUT_METHOD_SERVICE);
        if (imm.isFullscreenMode())
            return;

        
        if (mAutoCompleteList != null)
            mAutoCompleteList.setVisibility(isAutoComplete ? VISIBLE : GONE);
        if (mValidationMessage != null)
            mValidationMessage.setVisibility(isAutoComplete ? GONE : VISIBLE);

        if (sAutoCompleteMinWidth == 0) {
            Resources res = mContext.getResources();
            sAutoCompleteMinWidth = (int) (res.getDimension(R.dimen.autocomplete_min_width));
            sAutoCompleteRowHeight = (int) (res.getDimension(R.dimen.autocomplete_row_height));
            sValidationMessageHeight = (int) (res.getDimension(R.dimen.validation_message_height));
        }

        ImmutableViewportMetrics viewportMetrics = GeckoApp.mAppContext.getLayerView().getViewportMetrics();
        float zoom = viewportMetrics.zoomFactor;

        
        
        int left = 0;
        int top = 0; 
        int width = 0;
        int height = 0;

        try {
            left = (int) (rect.getDouble("x") * zoom - viewportMetrics.viewportRectLeft);
            top = (int) (rect.getDouble("y") * zoom - viewportMetrics.viewportRectTop);
            width = (int) (rect.getDouble("w") * zoom);
            height = (int) (rect.getDouble("h") * zoom);
        } catch (JSONException e) {
            
            Log.e(LOGTAG, "Error getting FormAssistPopup dimensions", e);
            return;
        }

        int popupWidth = RelativeLayout.LayoutParams.FILL_PARENT;
        int popupLeft = left < 0 ? 0 : left;

        FloatSize viewport = viewportMetrics.getSize();

        
        
        if (isAutoComplete && (left + width) < viewport.width) {
            popupWidth = left < 0 ? left + width : width;

            
            if (popupWidth < sAutoCompleteMinWidth) {
                popupWidth = sAutoCompleteMinWidth;

                
                if ((popupLeft + popupWidth) > viewport.width)
                    popupLeft = (int) (viewport.width - popupWidth);
            }
        }

        int popupHeight;
        if (isAutoComplete)
            popupHeight = sAutoCompleteRowHeight * mAutoCompleteList.getAdapter().getCount();
        else
            popupHeight = sValidationMessageHeight;

        int popupTop = top + height;

        if (!isAutoComplete) {
            mValidationMessageText.setLayoutParams(sValidationTextLayoutNormal);
            mValidationMessageArrow.setVisibility(VISIBLE);
            mValidationMessageArrowInverted.setVisibility(GONE);
        }

        
        
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

                if (!isAutoComplete) {
                    mValidationMessageText.setLayoutParams(sValidationTextLayoutInverted);
                    mValidationMessageArrow.setVisibility(GONE);
                    mValidationMessageArrowInverted.setVisibility(VISIBLE);
                }
           }
        }

        RelativeLayout.LayoutParams layoutParams =
                new RelativeLayout.LayoutParams(popupWidth, popupHeight);
        layoutParams.setMargins(popupLeft, popupTop, 0, 0);
        setLayoutParams(layoutParams);
        requestLayout();

        if (!isShown()) {
            setVisibility(VISIBLE);
            startAnimation(mAnimation);
        }
    }

    public void hide() {
        if (isShown()) {
            setVisibility(GONE);
            broadcastGeckoEvent("FormAssist:Hidden", null);
        }
    }

    void onInputMethodChanged(String newInputMethod) {
        boolean blocklisted = sInputMethodBlocklist.contains(newInputMethod);
        broadcastGeckoEvent("FormAssist:Blocklisted", String.valueOf(blocklisted));
    }

    private static void broadcastGeckoEvent(String eventName, String eventData) {
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent(eventName, eventData));
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
                Log.e(LOGTAG, "JSONException", e);
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

    private void registerEventListener(String event) {
        GeckoAppShell.getEventDispatcher().registerEventListener(event, this);
    }

    private void unregisterEventListener(String event) {
        GeckoAppShell.getEventDispatcher().unregisterEventListener(event, this);
    }
}
