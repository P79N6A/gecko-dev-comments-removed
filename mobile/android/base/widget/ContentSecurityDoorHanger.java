




package org.mozilla.gecko.widget;

import android.util.Log;
import android.widget.Button;
import android.widget.TextView;
import org.mozilla.gecko.R;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.view.View;
import org.mozilla.gecko.toolbar.SiteIdentityPopup;

public class ContentSecurityDoorHanger extends DoorHanger {
    private static final String LOGTAG = "GeckoSecurityDoorHanger";

    private final TextView mTitle;
    private final TextView mSecurityState;
    private final TextView mMessage;

    public ContentSecurityDoorHanger(Context context, DoorhangerConfig config, Type type) {
        super(context, config, type);

        mTitle = (TextView) findViewById(R.id.security_title);
        mSecurityState = (TextView) findViewById(R.id.security_state);
        mMessage = (TextView) findViewById(R.id.security_message);

        loadConfig(config);
    }

    @Override
    protected void loadConfig(DoorhangerConfig config) {
        final String message = config.getMessage();
        if (message != null) {
            mMessage.setText(message);
        }

        final JSONObject options = config.getOptions();
        if (options != null) {
            setOptions(options);
        }

        final DoorhangerConfig.Link link = config.getLink();
        if (link != null) {
            addLink(link.label, link.url);
        }

        addButtonsToLayout(config);
    }

    @Override
    protected int getContentResource() {
        return R.layout.doorhanger_security;
    }

    @Override
    public void setOptions(final JSONObject options) {
        super.setOptions(options);
        final JSONObject link = options.optJSONObject("link");
        if (link != null) {
            try {
                final String linkLabel = link.getString("label");
                final String linkUrl = link.getString("url");
                addLink(linkLabel, linkUrl);
            } catch (JSONException e) { }
        }

        final JSONObject trackingProtection = options.optJSONObject("tracking_protection");
        if (trackingProtection != null) {
            mTitle.setVisibility(VISIBLE);
            mTitle.setText(R.string.doorhanger_tracking_title);
            try {
                final boolean enabled = trackingProtection.getBoolean("enabled");
                if (enabled) {
                    mMessage.setText(R.string.doorhanger_tracking_message_enabled);
                    mSecurityState.setText(R.string.doorhanger_tracking_state_enabled);
                    mSecurityState.setTextColor(getResources().getColor(R.color.affirmative_green));
                } else {
                    mMessage.setText(R.string.doorhanger_tracking_message_disabled);
                    mSecurityState.setText(R.string.doorhanger_tracking_state_disabled);
                    mSecurityState.setTextColor(getResources().getColor(R.color.rejection_red));
                }
                mMessage.setVisibility(VISIBLE);
                mSecurityState.setVisibility(VISIBLE);
            } catch (JSONException e) {}
        }
    }

    @Override
    protected OnClickListener makeOnButtonClickListener(final int id) {
        return new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                final JSONObject response = new JSONObject();
                try {
                    switch (mType) {
                        case MIXED_CONTENT:
                            response.put("allowContent", (id == SiteIdentityPopup.ButtonType.DISABLE.ordinal()));
                            response.put("contentType", ("mixed"));
                            break;
                        case TRACKING:
                            response.put("allowContent", (id == SiteIdentityPopup.ButtonType.DISABLE.ordinal()));
                            response.put("contentType", ("tracking"));
                            break;
                        default:
                            Log.w(LOGTAG, "Unknown doorhanger type " + mType.toString());
                    }
                } catch (JSONException e) {
                    Log.e(LOGTAG, "Error creating onClick response", e);
                }

                mOnButtonClickListener.onButtonClick(response, ContentSecurityDoorHanger.this);
            }
        };
    }
}
