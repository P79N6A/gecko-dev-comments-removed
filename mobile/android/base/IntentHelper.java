




package org.mozilla.gecko;

import org.mozilla.gecko.util.ActivityResultHandler;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.JSONUtils;
import org.mozilla.gecko.util.WebActivityMapper;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;

import java.net.URISyntaxException;
import java.util.Arrays;
import java.util.List;

public final class IntentHelper implements GeckoEventListener {
    private static final String LOGTAG = "GeckoIntentHelper";
    private static final String[] EVENTS = {
        "Intent:GetHandlers",
        "Intent:Open",
        "Intent:OpenForResult",
        "Intent:OpenNoHandler",
        "WebActivity:Open"
    };

    
    private static String MARKET_INTENT_URI_PACKAGE_PREFIX = "market://details?id=";
    private static String EXTRA_BROWSER_FALLBACK_URL = "browser_fallback_url";

    private static IntentHelper instance;

    private final Activity activity;

    private IntentHelper(Activity activity) {
        this.activity = activity;
        EventDispatcher.getInstance().registerGeckoThreadListener(this, EVENTS);
    }

    public static IntentHelper init(Activity activity) {
        if (instance == null) {
            instance = new IntentHelper(activity);
        } else {
            Log.w(LOGTAG, "IntentHelper.init() called twice, ignoring.");
        }

        return instance;
    }

    public static void destroy() {
        if (instance != null) {
            EventDispatcher.getInstance().unregisterGeckoThreadListener(instance, EVENTS);
            instance = null;
        }
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        try {
            if (event.equals("Intent:GetHandlers")) {
                getHandlers(message);
            } else if (event.equals("Intent:Open")) {
                open(message);
            } else if (event.equals("Intent:OpenForResult")) {
                openForResult(message);
            } else if (event.equals("Intent:OpenNoHandler")) {
                openNoHandler(message);
            } else if (event.equals("WebActivity:Open")) {
                openWebActivity(message);
            }
        } catch (JSONException e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    private void getHandlers(JSONObject message) throws JSONException {
        final Intent intent = GeckoAppShell.getOpenURIIntent(activity,
                                                             message.optString("url"),
                                                             message.optString("mime"),
                                                             message.optString("action"),
                                                             message.optString("title"));
        final List<String> appList = Arrays.asList(GeckoAppShell.getHandlersForIntent(intent));

        final JSONObject response = new JSONObject();
        response.put("apps", new JSONArray(appList));
        EventDispatcher.sendResponse(message, response);
    }

    private void open(JSONObject message) throws JSONException {
        GeckoAppShell.openUriExternal(message.optString("url"),
                                      message.optString("mime"),
                                      message.optString("packageName"),
                                      message.optString("className"),
                                      message.optString("action"),
                                      message.optString("title"));
    }

    private void openForResult(final JSONObject message) throws JSONException {
        Intent intent = GeckoAppShell.getOpenURIIntent(activity,
                                                       message.optString("url"),
                                                       message.optString("mime"),
                                                       message.optString("action"),
                                                       message.optString("title"));
        intent.setClassName(message.optString("packageName"), message.optString("className"));
        intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);

        final ResultHandler handler = new ResultHandler(message);
        try {
            ActivityHandlerHelper.startIntentForActivity(activity, intent, handler);
        } catch (SecurityException e) {
            Log.w(LOGTAG, "Forbidden to launch activity.", e);
        }
    }

    





    private void openNoHandler(final JSONObject msg) {
        final String uri = msg.optString("uri");

        if (TextUtils.isEmpty(uri)) {
            displayToastCannotOpenLink();
            Log.w(LOGTAG, "Received empty URL. Ignoring...");
            return;
        }

        final Intent intent;
        try {
            
            intent = Intent.parseUri(uri, 0);
        } catch (final URISyntaxException e) {
            displayToastCannotOpenLink();
            
            Log.w(LOGTAG, "Unable to parse Intent URI");
            return;
        }

        
        
        
        
        
        
        
        
        
        
        
        if (intent.getPackage() != null) {
            final String marketUri = MARKET_INTENT_URI_PACKAGE_PREFIX + intent.getPackage();
            final Intent marketIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(marketUri));
            marketIntent.addCategory(Intent.CATEGORY_BROWSABLE);
            marketIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            activity.startActivity(marketIntent);

        } else if (intent.hasExtra(EXTRA_BROWSER_FALLBACK_URL)) {
            final String fallbackUrl = intent.getStringExtra(EXTRA_BROWSER_FALLBACK_URL);
            Tabs.getInstance().loadUrl(fallbackUrl);

        }  else {
            displayToastCannotOpenLink();
            
            Log.w(LOGTAG, "Unable to handle URI");
        }
    }

    private void displayToastCannotOpenLink() {
        final String errText = activity.getResources().getString(R.string.intent_uri_cannot_open);
        Toast.makeText(activity, errText, Toast.LENGTH_LONG).show();
    }

    private void openWebActivity(JSONObject message) throws JSONException {
        final Intent intent = WebActivityMapper.getIntentForWebActivity(message.getJSONObject("activity"));
        ActivityHandlerHelper.startIntentForActivity(activity, intent, new ResultHandler(message));
    }

    private static class ResultHandler implements ActivityResultHandler {
        private final JSONObject message;

        public ResultHandler(JSONObject message) {
            this.message = message;
        }

        @Override
        public void onActivityResult(int resultCode, Intent data) {
            JSONObject response = new JSONObject();

            try {
                if (data != null) {
                    response.put("extras", JSONUtils.bundleToJSON(data.getExtras()));
                    response.put("uri", data.getData().toString());
                }

                response.put("resultCode", resultCode);
            } catch (JSONException e) {
                Log.w(LOGTAG, "Error building JSON response.", e);
            }

            EventDispatcher.sendResponse(message, response);
        }
    }
}
