



package org.mozilla.gecko.toolbar;

import org.mozilla.gecko.BrowserApp;
import org.mozilla.gecko.R;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.widget.ArrowPopup;
import org.mozilla.gecko.widget.DoorHanger;
import org.mozilla.gecko.widget.DoorHanger.OnButtonClickListener;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.res.Resources;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;





public class SiteIdentityPopup extends ArrowPopup {
    private static final String LOGTAG = "GeckoSiteIdentityPopup";

    public static final String UNKNOWN = "unknown";
    public static final String VERIFIED = "verified";
    public static final String IDENTIFIED = "identified";
    public static final String MIXED_CONTENT_BLOCKED = "mixed_content_blocked";
    public static final String MIXED_CONTENT_LOADED = "mixed_content_loaded";

    
    static final int LEVEL_UKNOWN = 0;
    static final int LEVEL_IDENTIFIED = 1;
    static final int LEVEL_VERIFIED = 2;
    static final int LEVEL_MIXED_CONTENT_BLOCKED = 3;
    static final int LEVEL_MIXED_CONTENT_LOADED = 4;

    
    private static final String MIXED_CONTENT_SUPPORT_URL =
        "https://support.mozilla.org/kb/how-does-content-isnt-secure-affect-my-safety";

    private Resources mResources;

    private LinearLayout mIdentity;
    private TextView mHost;
    private TextView mOwner;
    private TextView mVerifier;

    private DoorHanger mMixedContentNotification;

    private final OnButtonClickListener mButtonClickListener;

    SiteIdentityPopup(BrowserApp activity) {
        super(activity);

        mResources = activity.getResources();
        mButtonClickListener = new PopupButtonListener();
    }

    static int getSecurityImageLevel(String mode) {
        if (IDENTIFIED.equals(mode)) {
            return LEVEL_IDENTIFIED;
        }
        if (VERIFIED.equals(mode)) {
            return LEVEL_VERIFIED;
        }
        if (MIXED_CONTENT_BLOCKED.equals(mode)) {
            return LEVEL_MIXED_CONTENT_BLOCKED;
        }
        if (MIXED_CONTENT_LOADED.equals(mode)) {
            return LEVEL_MIXED_CONTENT_LOADED;
        }
        return LEVEL_UKNOWN;
    }

    @Override
    protected void init() {
        super.init();

        
        
        setFocusable(true);

        LayoutInflater inflater = LayoutInflater.from(mActivity);
        mIdentity = (LinearLayout) inflater.inflate(R.layout.site_identity, null);
        mContent.addView(mIdentity);

        mHost = (TextView) mIdentity.findViewById(R.id.host);
        mOwner = (TextView) mIdentity.findViewById(R.id.owner);
        mVerifier = (TextView) mIdentity.findViewById(R.id.verifier);
    }

    private void setIdentity(JSONObject identityData) {
        try {
            String host = identityData.getString("host");
            mHost.setText(host);

            String owner = identityData.getString("owner");

            
            String supplemental = identityData.optString("supplemental");
            if (!TextUtils.isEmpty(supplemental)) {
                owner += "\n" + supplemental;
            }
            mOwner.setText(owner);

            String verifier = identityData.getString("verifier");
            String encrypted = identityData.getString("encrypted");
            mVerifier.setText(verifier + "\n" + encrypted);

            mContent.setPadding(0, 0, 0, 0);
            mIdentity.setVisibility(View.VISIBLE);

        } catch (JSONException e) {
            
            
            
            mContent.setPadding(0, (int) mResources.getDimension(R.dimen.identity_padding_top), 0, 0);
            mIdentity.setVisibility(View.GONE);
        }
    }

    private void addMixedContentNotification(boolean blocked) {
        
        removeMixedContentNotification();
        mMixedContentNotification = new DoorHanger(mActivity, DoorHanger.Theme.DARK);

        String message;
        if (blocked) {
            message = mActivity.getString(R.string.blocked_mixed_content_message_top) + "\n\n" +
                      mActivity.getString(R.string.blocked_mixed_content_message_bottom);
        } else {
            message = mActivity.getString(R.string.loaded_mixed_content_message);
        }
        mMixedContentNotification.setMessage(message);
        mMixedContentNotification.addLink(mActivity.getString(R.string.learn_more), MIXED_CONTENT_SUPPORT_URL, "\n\n");

        if (blocked) {
            mMixedContentNotification.setIcon(R.drawable.shield_doorhanger);
            mMixedContentNotification.addButton(mActivity.getString(R.string.disable_protection),
                                                "disable", mButtonClickListener);
            mMixedContentNotification.addButton(mActivity.getString(R.string.keep_blocking),
                                                "keepBlocking", mButtonClickListener);
        } else {
            mMixedContentNotification.setIcon(R.drawable.warning_doorhanger);
            mMixedContentNotification.addButton(mActivity.getString(R.string.enable_protection),
                                                "enable", mButtonClickListener);
        }

        mContent.addView(mMixedContentNotification);
    }

    private void removeMixedContentNotification() {
        if (mMixedContentNotification != null) {
            mContent.removeView(mMixedContentNotification);
            mMixedContentNotification = null;
        }
    }

    


    void updateIdentity(JSONObject identityData) {
        String mode;
        try {
            mode = identityData.getString("mode");
        } catch (JSONException e) {
            Log.e(LOGTAG, "Exception trying to get identity mode", e);
            return;
        }

        if (UNKNOWN.equals(mode)) {
            Log.e(LOGTAG, "Can't show site identity popup in non-identified state");
            return;
        }

        if (!mInflated)
            init();

        setIdentity(identityData);

        if (MIXED_CONTENT_BLOCKED.equals(mode) || MIXED_CONTENT_LOADED.equals(mode)) {
            addMixedContentNotification(MIXED_CONTENT_BLOCKED.equals(mode));
        }
    }

    @Override
    public void dismiss() {
        super.dismiss();
        removeMixedContentNotification();
    }

    private class PopupButtonListener implements OnButtonClickListener {
        @Override
        public void onButtonClick(DoorHanger dh, String tag) {
            try {
                JSONObject data = new JSONObject();
                data.put("allowMixedContent", tag.equals("disable"));
                GeckoEvent e = GeckoEvent.createBroadcastEvent("Session:Reload", data.toString());
                GeckoAppShell.sendEventToGecko(e);
            } catch (JSONException e) {
                Log.e(LOGTAG, "Exception creating message to enable/disable mixed content blocking", e);
            }

            dismiss();
        }
    }
}
