



package org.mozilla.gecko.toolbar;

import org.json.JSONException;
import org.json.JSONArray;
import org.mozilla.gecko.AboutPages;
import org.mozilla.gecko.EventDispatcher;
import org.mozilla.gecko.R;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.SiteIdentity;
import org.mozilla.gecko.SiteIdentity.SecurityMode;
import org.mozilla.gecko.SiteIdentity.MixedMode;
import org.mozilla.gecko.SiteIdentity.TrackingMode;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.widget.AnchoredPopup;
import org.mozilla.gecko.widget.DoorHanger;
import org.mozilla.gecko.widget.DoorHanger.OnButtonClickListener;
import org.json.JSONObject;

import android.content.Context;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;
import org.mozilla.gecko.widget.DoorhangerConfig;
import org.mozilla.gecko.widget.SiteLogins;





public class SiteIdentityPopup extends AnchoredPopup implements GeckoEventListener {

    public static enum ButtonType { DISABLE, ENABLE, KEEP_BLOCKING };

    private static final String LOGTAG = "GeckoSiteIdentityPopup";

    private static final String MIXED_CONTENT_SUPPORT_URL =
        "https://support.mozilla.org/kb/how-does-insecure-content-affect-safety-android";

    private static final String TRACKING_CONTENT_SUPPORT_URL =
        "https://support.mozilla.org/kb/firefox-android-tracking-protection";

    
    private final static String FORMAT_S = "%s";

    private SiteIdentity mSiteIdentity;

    private LinearLayout mIdentity;

    private LinearLayout mIdentityKnownContainer;
    private LinearLayout mIdentityUnknownContainer;

    private TextView mHost;
    private TextView mOwnerLabel;
    private TextView mOwner;
    private TextView mVerifier;

    private View mDivider;

    private DoorHanger mMixedContentNotification;
    private DoorHanger mTrackingContentNotification;
    private DoorHanger mSelectLoginDoorhanger;

    private final OnButtonClickListener mButtonClickListener;

    public SiteIdentityPopup(Context context) {
        super(context);

        mButtonClickListener = new PopupButtonListener();
        EventDispatcher.getInstance().registerGeckoThreadListener(this, "Doorhanger:Logins");
    }

    @Override
    protected void init() {
        super.init();

        
        
        setFocusable(true);

        LayoutInflater inflater = LayoutInflater.from(mContext);
        mIdentity = (LinearLayout) inflater.inflate(R.layout.site_identity, null);
        mContent.addView(mIdentity);

        mIdentityKnownContainer =
                (LinearLayout) mIdentity.findViewById(R.id.site_identity_known_container);
        mIdentityUnknownContainer =
                (LinearLayout) mIdentity.findViewById(R.id.site_identity_unknown_container);

        mHost = (TextView) mIdentityKnownContainer.findViewById(R.id.host);
        mOwnerLabel = (TextView) mIdentityKnownContainer.findViewById(R.id.owner_label);
        mOwner = (TextView) mIdentityKnownContainer.findViewById(R.id.owner);
        mVerifier = (TextView) mIdentityKnownContainer.findViewById(R.id.verifier);
        mDivider = mIdentity.findViewById(R.id.divider_doorhanger);

        final TextView siteSettingsLink = (TextView) mIdentity.findViewById(R.id.site_settings_link);
        siteSettingsLink.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Permissions:Get", null));
                dismiss();
            }
        });
    }

    private void updateIdentity(final SiteIdentity siteIdentity) {
        if (!mInflated) {
            init();
        }

        final boolean isIdentityKnown = (siteIdentity.getSecurityMode() != SecurityMode.UNKNOWN);
        toggleIdentityKnownContainerVisibility(isIdentityKnown);

        if (isIdentityKnown) {
            updateIdentityInformation(siteIdentity);
        }
    }

    @Override
    public void handleMessage(String event, JSONObject geckoObject) {
        if ("Doorhanger:Logins".equals(event)) {
            try {
                final Tab selectedTab = Tabs.getInstance().getSelectedTab();
                if (selectedTab != null) {
                    final JSONObject data = geckoObject.getJSONObject("data");
                    addLoginsToTab(data);
                }
                if (isShowing()) {
                    addSelectLoginDoorhanger(selectedTab);
                }
            } catch (JSONException e) {
                Log.e(LOGTAG, "Error accessing logins in Doorhanger:Logins message", e);
            }
        }
    }

    private void addLoginsToTab(JSONObject data) throws JSONException {
        final JSONObject titleObj = data.getJSONObject("title");
        final JSONArray logins = data.getJSONArray("logins");

        final SiteLogins siteLogins = new SiteLogins(titleObj, logins);
        Tabs.getInstance().getSelectedTab().setSiteLogins(siteLogins);
    }

    private void addSelectLoginDoorhanger(Tab tab) throws JSONException {
        final SiteLogins siteLogins = tab.getSiteLogins();
        if (siteLogins == null) {
            return;
        }

        final JSONArray logins = siteLogins.getLogins();
        if (logins.length() == 0) {
            return;
        }

        final DoorhangerConfig config = new DoorhangerConfig(DoorHanger.Type.LOGIN, null);

        
        String username = ((JSONObject) logins.get(0)).getString("username");
        if (TextUtils.isEmpty(username)) {
            username = mContext.getString(R.string.doorhanger_login_no_username);
        }

        final String message = mContext.getString(R.string.doorhanger_login_select_message).replace(FORMAT_S, username);
        config.setMessage(message);

        
        final JSONObject options = new JSONObject();
        final JSONObject titleObj = siteLogins.getTitle();
        options.put("title", titleObj);

        if (logins.length() > 1) {
            
            final JSONObject actionText = null;
            options.put("actionText", actionText);
        }

        config.setOptions(options);

        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                if (!mInflated) {
                    init();
                }

                removeSelectLoginDoorhanger();

                mSelectLoginDoorhanger = DoorHanger.Get(mContext, config);
                mContent.addView(mSelectLoginDoorhanger);
                mDivider.setVisibility(View.VISIBLE);
            }
        });
    }

    private void removeSelectLoginDoorhanger() {
        if (mSelectLoginDoorhanger != null) {
            mContent.removeView(mSelectLoginDoorhanger);
            mSelectLoginDoorhanger = null;
        }
    }

    private void toggleIdentityKnownContainerVisibility(final boolean isIdentityKnown) {
        if (isIdentityKnown) {
            mIdentityKnownContainer.setVisibility(View.VISIBLE);
            mIdentityUnknownContainer.setVisibility(View.GONE);
        } else {
            mIdentityKnownContainer.setVisibility(View.GONE);
            mIdentityUnknownContainer.setVisibility(View.VISIBLE);
        }
    }

    private void updateIdentityInformation(final SiteIdentity siteIdentity) {
        mHost.setText(siteIdentity.getHost());

        String owner = siteIdentity.getOwner();
        if (owner == null) {
            mOwnerLabel.setVisibility(View.GONE);
            mOwner.setVisibility(View.GONE);
        } else {
            mOwnerLabel.setVisibility(View.VISIBLE);
            mOwner.setVisibility(View.VISIBLE);

            
            final String supplemental = siteIdentity.getSupplemental();
            if (!TextUtils.isEmpty(supplemental)) {
                owner += "\n" + supplemental;
            }
            mOwner.setText(owner);
        }

        final String verifier = siteIdentity.getVerifier();
        final String encrypted = siteIdentity.getEncrypted();
        mVerifier.setText(verifier + "\n" + encrypted);
    }

    private void addMixedContentNotification(boolean blocked) {
        
        removeMixedContentNotification();

        final DoorhangerConfig config = new DoorhangerConfig(DoorHanger.Type.MIXED_CONTENT, mButtonClickListener);
        int icon;
        if (blocked) {
            icon = R.drawable.shield_enabled_doorhanger;
            config.setMessage(mContext.getString(R.string.blocked_mixed_content_message_top) + "\n\n" +
                      mContext.getString(R.string.blocked_mixed_content_message_bottom));
        } else {
            icon = R.drawable.shield_disabled_doorhanger;
            config.setMessage(mContext.getString(R.string.loaded_mixed_content_message));
        }

        config.setLink(mContext.getString(R.string.learn_more), MIXED_CONTENT_SUPPORT_URL, "\n\n");
        addNotificationButtons(config, blocked);

        mMixedContentNotification = DoorHanger.Get(mContext, config);
        mMixedContentNotification.setIcon(icon);

        mContent.addView(mMixedContentNotification);
        mDivider.setVisibility(View.VISIBLE);
    }

    private void removeMixedContentNotification() {
        if (mMixedContentNotification != null) {
            mContent.removeView(mMixedContentNotification);
            mMixedContentNotification = null;
        }
    }

    private void addTrackingContentNotification(boolean blocked) {
        
        removeTrackingContentNotification();

        final DoorhangerConfig config = new DoorhangerConfig(DoorHanger.Type.TRACKING, mButtonClickListener);

        int icon;
        if (blocked) {
            icon = R.drawable.shield_enabled_doorhanger;
            config.setMessage(mContext.getString(R.string.blocked_tracking_content_message_top) + "\n\n" +
                      mContext.getString(R.string.blocked_tracking_content_message_bottom));
        } else {
            icon = R.drawable.shield_disabled_doorhanger;
            config.setMessage(mContext.getString(R.string.loaded_tracking_content_message_top) + "\n\n" +
                      mContext.getString(R.string.loaded_tracking_content_message_bottom));
        }

        config.setLink(mContext.getString(R.string.learn_more), TRACKING_CONTENT_SUPPORT_URL, "\n\n");
        addNotificationButtons(config, blocked);

        mTrackingContentNotification = DoorHanger.Get(mContext, config);

        mTrackingContentNotification.setIcon(icon);


        mContent.addView(mTrackingContentNotification);
        mDivider.setVisibility(View.VISIBLE);
    }

    private void removeTrackingContentNotification() {
        if (mTrackingContentNotification != null) {
            mContent.removeView(mTrackingContentNotification);
            mTrackingContentNotification = null;
        }
    }

    private void addNotificationButtons(DoorhangerConfig config, boolean blocked) {
        if (blocked) {
            config.appendButton(mContext.getString(R.string.disable_protection), ButtonType.DISABLE.ordinal());
            config.appendButton(mContext.getString(R.string.keep_blocking), ButtonType.KEEP_BLOCKING.ordinal());
        } else {
            config.appendButton(mContext.getString(R.string.enable_protection), ButtonType.ENABLE.ordinal());
        }
    }

    


    void setSiteIdentity(SiteIdentity siteIdentity) {
        mSiteIdentity = siteIdentity;
    }

    @Override
    public void show() {
        if (mSiteIdentity == null) {
            Log.e(LOGTAG, "Can't show site identity popup for undefined state");
            return;
        }

        
        
        final Tab selectedTab = Tabs.getInstance().getSelectedTab();
        if (selectedTab != null && AboutPages.isAboutPage(selectedTab.getURL())) {
            Log.d(LOGTAG, "We don't show site identity popups for about: pages");
            return;
        }

        updateIdentity(mSiteIdentity);

        final MixedMode mixedMode = mSiteIdentity.getMixedMode();
        if (mixedMode != MixedMode.UNKNOWN) {
            addMixedContentNotification(mixedMode == MixedMode.MIXED_CONTENT_BLOCKED);
        }

        final TrackingMode trackingMode = mSiteIdentity.getTrackingMode();
        if (trackingMode != TrackingMode.UNKNOWN) {
            addTrackingContentNotification(trackingMode == TrackingMode.TRACKING_CONTENT_BLOCKED);
        }

        try {
            addSelectLoginDoorhanger(selectedTab);
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error adding selectLogin doorhanger", e);
        }

        showDividers();

        super.show();
    }

    
    private void showDividers() {
        final int count = mContent.getChildCount();
        DoorHanger lastVisibleDoorHanger = null;

        for (int i = 0; i < count; i++) {
            final View child = mContent.getChildAt(i);

            if (!(child instanceof DoorHanger)) {
                continue;
            }

            DoorHanger dh = (DoorHanger) child;
            dh.showDivider();
            if (dh.getVisibility() == View.VISIBLE) {
                lastVisibleDoorHanger = dh;
            }
        }

        if (lastVisibleDoorHanger != null) {
            lastVisibleDoorHanger.hideDivider();
        }
    }

    void destroy() {
        EventDispatcher.getInstance().unregisterGeckoThreadListener(this, "Doorhanger:Logins");
    }

    @Override
    public void dismiss() {
        super.dismiss();
        removeMixedContentNotification();
        removeTrackingContentNotification();
        removeSelectLoginDoorhanger();
        mDivider.setVisibility(View.GONE);
    }

    private class PopupButtonListener implements OnButtonClickListener {
        @Override
        public void onButtonClick(JSONObject response, DoorHanger doorhanger) {
            GeckoEvent e = GeckoEvent.createBroadcastEvent("Session:Reload", response.toString());
            GeckoAppShell.sendEventToGecko(e);
            dismiss();
        }
    }
}
