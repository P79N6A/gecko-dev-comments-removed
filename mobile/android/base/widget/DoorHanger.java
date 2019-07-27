




package org.mozilla.gecko.widget;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewStub;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
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

    private static final String LOGTAG = "GeckoDoorHanger";

    
    private final View mDivider;

    private final Button mNegativeButton;
    private final Button mPositiveButton;
    protected final OnButtonClickListener mOnButtonClickListener;

    
    private final int mTabId;

    
    private final String mIdentifier;

    protected final Type mType;

    protected final ImageView mIcon;
    protected final TextView mLink;
    protected final TextView mDoorhangerTitle;

    protected final Context mContext;
    protected final Resources mResources;

    protected int mDividerColor;

    protected boolean mPersistWhileVisible;
    protected int mPersistenceCount;
    protected long mTimeout;

    protected DoorHanger(Context context, DoorhangerConfig config, Type type) {
        super(context);

        mContext = context;
        mResources = context.getResources();
        mTabId = config.getTabId();
        mIdentifier = config.getId();
        mType = type;

        LayoutInflater.from(context).inflate(R.layout.doorhanger, this);
        setOrientation(VERTICAL);

        mDivider = findViewById(R.id.divider_doorhanger);
        mIcon = (ImageView) findViewById(R.id.doorhanger_icon);
        mLink = (TextView) findViewById(R.id.doorhanger_link);
        mDoorhangerTitle = (TextView) findViewById(R.id.doorhanger_title);

        mNegativeButton = (Button) findViewById(R.id.doorhanger_button_negative);
        mPositiveButton = (Button) findViewById(R.id.doorhanger_button_positive);
        mOnButtonClickListener = config.getButtonClickListener();

        mDividerColor = mResources.getColor(R.color.divider_light);

        final ViewStub contentStub = (ViewStub) findViewById(R.id.content);
        contentStub.setLayoutResource(getContentResource());
        contentStub.inflate();
    }

    protected abstract int getContentResource();

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

    protected void addButtonsToLayout(DoorhangerConfig config) {
        final DoorhangerConfig.ButtonConfig negativeButtonConfig = config.getNegativeButtonConfig();
        final DoorhangerConfig.ButtonConfig positiveButtonConfig = config.getPositiveButtonConfig();

        if (negativeButtonConfig != null) {
            mNegativeButton.setText(negativeButtonConfig.label);
            mNegativeButton.setOnClickListener(makeOnButtonClickListener(negativeButtonConfig.callback));
            mNegativeButton.setVisibility(VISIBLE);
        }

        if (positiveButtonConfig != null) {
            mPositiveButton.setText(positiveButtonConfig.label);
            mPositiveButton.setOnClickListener(makeOnButtonClickListener(positiveButtonConfig.callback));
            mPositiveButton.setVisibility(VISIBLE);
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

    protected abstract OnClickListener makeOnButtonClickListener(final int id);

    





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

    public void showTitle(Bitmap favicon, String title) {
        mDoorhangerTitle.setText(title);
        mDoorhangerTitle.setCompoundDrawablesWithIntrinsicBounds(new BitmapDrawable(getResources(), favicon), null, null, null);
        if (favicon != null) {
            mDoorhangerTitle.setCompoundDrawablePadding((int) mContext.getResources().getDimension(R.dimen.doorhanger_drawable_padding));
        }
        mDoorhangerTitle.setVisibility(VISIBLE);
    }

    public void hideTitle() {
        mDoorhangerTitle.setVisibility(GONE);
    }
}
