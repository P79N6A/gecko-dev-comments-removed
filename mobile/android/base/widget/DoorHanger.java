




package org.mozilla.gecko.widget;

import android.content.Context;
import android.text.Html;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.text.style.ForegroundColorSpan;
import android.text.style.URLSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import org.json.JSONObject;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.prompts.PromptInput;

import java.util.List;

public abstract class DoorHanger extends LinearLayout {

    public static DoorHanger Get(Context context, DoorhangerConfig config) {
        final Type type = config.getType();
        if (type != null) {
            switch (type) {
                case LOGIN:
                    return new LoginDoorHanger(context, config);
                case SITE:
                    return new DefaultDoorHanger(context, config, type);
            }
        }

        return new DefaultDoorHanger(context, config);
    }

    public static enum Type { DEFAULT, LOGIN, SITE }

    public interface OnButtonClickListener {
        public void onButtonClick(DoorHanger dh, String tag);
    }

    private static final LayoutParams sButtonParams;
    static {
        sButtonParams = new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT, 1.0f);
    }

    private static final String LOGTAG = "GeckoDoorHanger";

    
    private final View mDivider;

    private final LinearLayout mButtonsContainer;

    
    private final int mTabId;

    
    private final String mIdentifier;

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
        if (type == Type.SITE) {
            mMessage.setTextAppearance(getContext(), R.style.TextAppearance_DoorHanger_Small);
        }
        mButtonsContainer = (LinearLayout) findViewById(R.id.doorhanger_buttons);

        mDividerColor = getResources().getColor(R.color.divider_light);
        setOrientation(VERTICAL);
    }

    abstract protected void loadConfig(DoorhangerConfig config);

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

    public void addButton(final String text, final String tag, final OnButtonClickListener listener) {
        final Button button = (Button) LayoutInflater.from(getContext()).inflate(R.layout.doorhanger_button, null);
        button.setText(text);
        button.setTag(tag);

        button.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                listener.onButtonClick(DoorHanger.this, tag);
            }
        });

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

    
    public List<PromptInput> getInputs() {
        return null;
    }

    public CheckBox getCheckBox() {
        return null;
    }

}
