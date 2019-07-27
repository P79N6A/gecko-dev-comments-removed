




package org.mozilla.gecko.util;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.content.Intent;
import android.net.Uri;
import android.text.TextUtils;

import java.util.HashMap;
import java.util.Map;

public final class WebActivityMapper {
    private static final Map<String, WebActivityMapping> activityMap = new HashMap<String, WebActivityMapping>();
    static {
        activityMap.put("dial", new DialMapping());
        activityMap.put("open", new OpenMapping());
        activityMap.put("pick", new PickMapping());
        activityMap.put("send", new SendMapping());
        activityMap.put("view", new ViewMapping());
    };

    private static abstract class WebActivityMapping {
        
        public abstract String getAction();

        public String getMime(JSONObject data) throws JSONException {
            return null;
        }

        public String getUri(JSONObject data) throws JSONException {
            return null;
        }

        public void putExtras(JSONObject data, Intent intent) throws JSONException {}
    }

    


    private static abstract class BaseMapping extends WebActivityMapping {
        


        public String getMime(JSONObject data) throws JSONException {
            return data.optString("type", null);
        }

        


        public String getUri(JSONObject data) throws JSONException {
            
            String uri = data.optString("uri", null);
            return uri != null ? uri : data.optString("url", null);
        }
    }

    public static Intent getIntentForWebActivity(JSONObject message) throws JSONException {
        final String name = message.getString("name").toLowerCase();
        final JSONObject data = message.getJSONObject("data");

        final WebActivityMapping mapping = activityMap.get(name);
        final Intent intent = new Intent(mapping.getAction());

        final String mime = mapping.getMime(data);
        if (!TextUtils.isEmpty(mime)) {
            intent.setType(mime);
        }

        final String uri = mapping.getUri(data);
        if (!TextUtils.isEmpty(uri)) {
            intent.setData(Uri.parse(uri));
        }

        mapping.putExtras(data, intent);

        return intent;
    }

    private static class DialMapping extends WebActivityMapping {
        @Override
        public String getAction() {
            return Intent.ACTION_DIAL;
        }

        @Override
        public String getUri(JSONObject data) throws JSONException {
            return "tel:" + data.getString("number");
        }
    }

    private static class OpenMapping extends BaseMapping {
        @Override
        public String getAction() {
            return Intent.ACTION_VIEW;
        }
    }

    private static class PickMapping extends BaseMapping {
        @Override
        public String getAction() {
            return Intent.ACTION_GET_CONTENT;
        }

        @Override
        public String getMime(JSONObject data) throws JSONException {
            
            String mime = data.optString("type", null);
            return !TextUtils.isEmpty(mime) ? mime : "*/*";
        }
    }

    private static class SendMapping extends BaseMapping {
        @Override
        public String getAction() {
            return Intent.ACTION_SEND;
        }

        @Override
        public void putExtras(JSONObject data, Intent intent) throws JSONException {
            optPutExtra("text", Intent.EXTRA_TEXT, data, intent);
            optPutExtra("html_text", Intent.EXTRA_HTML_TEXT, data, intent);
            optPutExtra("stream", Intent.EXTRA_STREAM, data, intent);
        }

        private static void optPutExtra(String key, String extraName, JSONObject data, Intent intent) {
            final String extraValue = data.optString(key);
            if (!TextUtils.isEmpty(extraValue)) {
                intent.putExtra(extraName, extraValue);
            }
        }
    }

    private static class ViewMapping extends BaseMapping {
        @Override
        public String getAction() {
            return Intent.ACTION_VIEW;
        }

        @Override
        public String getMime(JSONObject data) {
            
            String type = data.optString("type", null);
            if ("url".equals(type) || "uri".equals(type)) {
                return null;
            } else {
                return type;
            }
        }
    }
}
