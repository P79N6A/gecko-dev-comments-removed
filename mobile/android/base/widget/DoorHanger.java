




package org.mozilla.gecko.widget;

import android.content.Context;
import android.text.Html;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.text.style.ForegroundColorSpan;
import android.text.style.URLSpan;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tabs;

public abstract class DoorHanger extends LinearLayout {

    public static DoorHanger Get(Context context, DoorhangerConfig config) {
        final Type type = config.getType();
        switch (type) {
            case LOGIN:
                return new LoginDoorHanger(context, config);
            case TRACKING:
            case MIXED_CONTENT:
                return new DefaultDoorHanger(context, config, type);
        }
        return new DefaultDoorHanger(context, config, type);
    }

    public static enum Type { DEFAULT, LOGIN, TRACKING, MIXED_CONTENT}

    public interface OnButtonClickListener {
        public void onButtonClick(JSONObject response, DoorHanger doorhanger);
    }

    protected static final LayoutParams sButtonParams;
    static {
        sButtonParams = new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT, 1.0f);
    }

    private static final String LOGTAG = "GeckoDoorHanger";

    
    private final View mDivider;

    protected final LinearLayout mButtonsContainer;
    protected final OnButtonClickListener mOnButtonClickListener;

    
    private final int mTabId;

    
    private final String mIdentifier;

    protected final Type mType;

    private final ImageView mIcon;
    private final TextView mMessage;

    protected Context mContext;

    protected int mDividerColor;

    protected boolean mPersistWhileVisible;
    protected int mPersistenceCount;
    protected long mTimeout;

    protected DoorHanger(Context context, DoorhangerConfig config, Type type) {
        super(context);
        mContext = context;
        mTabId = config.getTabId();
        mIdentifier = config.getId();

        int resource;
        switch (type) {
            case LOGIN:
                resource = R.layout.login_doorhanger;
                break;
            default:
                resource = R.layout.doorhanger;
        }

        LayoutInflater.from(context).inflate(resource, this);
        mDivider = findViewById(R.id.divider_doorhanger);
        mIcon = (ImageView) findViewById(R.id.doorhanger_icon);
        mMessage = (TextView) findViewById(R.id.doorhanger_message);

        
        if (type == Type.TRACKING || type == Type.MIXED_CONTENT) {
            mMessage.setTextAppearance(getContext(), R.style.TextAppearance_DoorHanger_Small);
        }

        mType = type;

        mButtonsContainer = (LinearLayout) findViewById(R.id.doorhanger_buttons);
        mOnButtonClickListener = config.getButtonClickListener();

        mDividerColor = getResources().getColor(R.color.divider_light);
        setOrientation(VERTICAL);
    }

    protected abstract void loadConfig(DoorhangerConfig config);

    protected void setOptions(final JSONObject options) {
        final int persistence = options.optInt("persistence");
        if (persistence > 0) {
            mPersistenceCount = persistence;
        }

        mPersistWhileVisible = options.optBoolean("persistWhileVisible");

        final long timeout = options.optLong("timeout");
        if (timeout > 0) {
            mTimeout = timeout;
        }
    }

    protected void setButtons(DoorhangerConfig config) {
        final JSONArray buttons = config.getButtons();
        final OnButtonClickListener listener = config.getButtonClickListener();
        for (int i = 0; i < buttons.length(); i++) {
            try {
                final JSONObject buttonObject = buttons.getJSONObject(i);
                final String label = buttonObject.getString("label");
                final int callbackId = buttonObject.getInt("callback");
                addButtonToLayout(label, callbackId);
            } catch (JSONException e) {
                Log.e(LOGTAG, "Error creating doorhanger button", e);
            }
        }
   }

    public int getTabId() {
        return mTabId;
    }

    public String getIdentifier() {
        return mIdentifier;
    }

    public void showDivider() {
        mDivider.setVisibility(View.VISIBLE);
    }

    public void hideDivider() {
        mDivider.setVisibility(View.GONE);
    }

    public void setIcon(int resId) {
        mIcon.setImageResource(resId);
        mIcon.setVisibility(View.VISIBLE);
    }

    protected void setMessage(String message) {
        Spanned markupMessage = Html.fromHtml(message);
        mMessage.setText(markupMessage);
    }

    protected void addLink(String label, String url, String delimiter) {
        String title = mMessage.getText().toString();
        SpannableString titleWithLink = new SpannableString(title + delimiter + label);
        URLSpan linkSpan = new URLSpan(url) {
            @Override
            public void onClick(View view) {
                Tabs.getInstance().loadUrlInTab(getURL());
            }
        };

        
        ForegroundColorSpan colorSpan = new ForegroundColorSpan(mMessage.getCurrentTextColor());
        titleWithLink.setSpan(colorSpan, 0, title.length(), 0);

        titleWithLink.setSpan(linkSpan, title.length() + 1, titleWithLink.length(), 0);
        mMessage.setText(titleWithLink);
        mMessage.setMovementMethod(LinkMovementMethod.getInstance());
    }

    




    private void addButtonToLayout(String text, int id) {
        final Button button = createButtonInstance(text, id);
        if (mButtonsContainer.getChildCount() == 0) {
            
            mButtonsContainer.setVisibility(View.VISIBLE);
            
            View divider = findViewById(R.id.divider_buttons);
            divider.setVisibility(View.VISIBLE);
        } else {
            
            Divider divider = new Divider(getContext(), null);
            divider.setOrientation(Divider.Orientation.VERTICAL);
            divider.setBackgroundColor(mDividerColor);
            mButtonsContainer.addView(divider);
        }

        mButtonsContainer.addView(button, sButtonParams);
    }

    protected abstract Button createButtonInstance(String text, int id);

    





    public boolean shouldRemove(boolean isShowing) {
        if (mPersistWhileVisible && isShowing) {
            
            if (mPersistenceCount != 0)
                mPersistenceCount--;
            return false;
        }

        
        
        if (mPersistenceCount != 0) {
            mPersistenceCount--;
            return false;
        }

        if (System.currentTimeMillis() <= mTimeout) {
            return false;
        }

        return true;
    }
}
