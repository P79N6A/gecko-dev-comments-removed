




package org.mozilla.gecko.widget;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import ch.boye.httpclientandroidlib.util.TextUtils;
import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.R;
import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.favicons.OnFaviconLoadedListener;

public class LoginDoorHanger extends DoorHanger {
    private static final String LOGTAG = "LoginDoorHanger";

    final TextView mTitle;
    final TextView mLogin;

    public LoginDoorHanger(Context context, DoorhangerConfig config) {
        super(context, config, Type.LOGIN);

        mTitle = (TextView) findViewById(R.id.doorhanger_title);
        mLogin = (TextView) findViewById(R.id.doorhanger_login);

        loadConfig(config);
    }

    @Override
    protected void loadConfig(DoorhangerConfig config) {
        setOptions(config.getOptions());
        setMessage(config.getMessage());
        setButtons(config);
    }

    @Override
    protected void setOptions(final JSONObject options) {
        super.setOptions(options);

        final JSONObject titleObj = options.optJSONObject("title");
        if (titleObj != null) {

            try {
                final String text = titleObj.getString("text");
                mTitle.setText(text);
            } catch (JSONException e) {
                Log.e(LOGTAG, "Error loading title from options JSON");
            }

            final String resource = titleObj.optString("resource");
            if (resource != null) {
                Favicons.getSizedFaviconForPageFromLocal(mContext, resource, 32, new OnFaviconLoadedListener() {
                    @Override
                    public void onFaviconLoaded(String url, String faviconURL, Bitmap favicon) {
                        if (favicon != null) {
                            mTitle.setCompoundDrawablesWithIntrinsicBounds(new BitmapDrawable(mContext.getResources(), favicon), null, null, null);
                            mTitle.setCompoundDrawablePadding((int) mContext.getResources().getDimension(R.dimen.doorhanger_drawable_padding));
                        }
                    }
                });
            }
        }

        final String subtext = options.optString("subtext");
        if (!TextUtils.isEmpty(subtext)) {
            mLogin.setText(subtext);
            mLogin.setVisibility(View.VISIBLE);
        } else {
            mLogin.setVisibility(View.GONE);
        }
    }

    @Override
    protected Button createButtonInstance(final String text, final int id) {
        final Button button = (Button) LayoutInflater.from(getContext()).inflate(R.layout.doorhanger_button, null);
        button.setText(text);

        button.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                final JSONObject response = new JSONObject();
                try {
                    response.put("callback", id);
                } catch (JSONException e) {
                    Log.e(LOGTAG, "Error making doorhanger response message");
                }
                mOnButtonClickListener.onButtonClick(response, LoginDoorHanger.this);
            }
        });

        return button;
    }
}
