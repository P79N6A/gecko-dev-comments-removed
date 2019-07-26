package org.mozilla.gecko.preferences;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.os.Build;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.util.AttributeSet;
import android.util.Log;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.R;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.util.GeckoEventListener;

public class SearchPreferenceCategory extends PreferenceCategory implements GeckoEventListener {
    public static final String LOGTAG = "SearchPrefCategory";

    private static int sIconSize;

    public SearchPreferenceCategory(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init();
    }
    public SearchPreferenceCategory(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public SearchPreferenceCategory(Context context) {
        super(context);
        init();
    }

    private void init() {
        sIconSize = getContext().getResources().getDimensionPixelSize(R.dimen.searchpreferences_icon_size);
    }

    @Override
    protected void onAttachedToActivity() {
        super.onAttachedToActivity();

        
        GeckoAppShell.registerEventListener("SearchEngines:Data", this);
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("SearchEngines:Get", null));
    }

    @Override
    public void handleMessage(String event, final JSONObject data) {
        if (event.equals("SearchEngines:Data")) {
            JSONArray engines;
            try {
                engines = data.getJSONArray("searchEngines");
            } catch (JSONException e) {
                Log.e(LOGTAG, "Unable to decode search engine data from Gecko.", e);
                return;
            }

            
            for (int i = 0; i < engines.length(); i++) {
                try {
                    JSONObject engineJSON = engines.getJSONObject(i);
                    final String engineName = engineJSON.getString("name");

                    Preference engine = new Preference(getContext());
                    engine.setTitle(engineName);
                    engine.setKey(engineName);

                    
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
                        String iconURI = engineJSON.getString("iconURI");
                        Bitmap iconBitmap = BitmapUtils.getBitmapFromDataURI(iconURI);
                        Bitmap scaledIconBitmap = Bitmap.createScaledBitmap(iconBitmap, sIconSize, sIconSize, false);
                        BitmapDrawable drawable = new BitmapDrawable(scaledIconBitmap);
                        engine.setIcon(drawable);
                    }
                    addPreference(engine);
                    
                } catch (JSONException e) {
                    Log.e(LOGTAG, "JSONException parsing engine at index " + i, e);
                }
            }
        }
    }
}
