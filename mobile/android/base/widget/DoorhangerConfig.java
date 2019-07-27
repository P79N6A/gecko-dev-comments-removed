




package org.mozilla.gecko.widget;

import android.util.Log;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.mozilla.gecko.widget.DoorHanger.Type;

public class DoorhangerConfig {

    public static class Link {
        public final String label;
        public final String url;
        public final String delimiter;

        private Link(String label, String url, String delimiter) {
            this.label = label;
            this.url = url;
            this.delimiter = delimiter;
        }
    }

    private static final String LOGTAG = "DoorhangerConfig";

    private final int tabId;
    private final String id;
    private final DoorHanger.OnButtonClickListener buttonClickListener;
    private final DoorHanger.Type type;
    private String message;
    private JSONObject options;
    private Link link;
    private JSONArray buttons = new JSONArray();

    public DoorhangerConfig(Type type, DoorHanger.OnButtonClickListener listener) {
        
        

        this(-1, null, type, listener);
    }

    public DoorhangerConfig(int tabId, String id, DoorHanger.Type type, DoorHanger.OnButtonClickListener buttonClickListener) {
        this.tabId = tabId;
        this.id = id;
        this.type = type;
        this.buttonClickListener = buttonClickListener;
    }

    public int getTabId() {
        return tabId;
    }

    public String getId() {
        return id;
    }

    public Type getType() {
        return type;
    }

    public void setMessage(String message) {
        this.message = message;
    }

    public String getMessage() {
        return message;
    }

    public void setOptions(JSONObject options) {
        this.options = options;
    }

    public JSONObject getOptions() {
        return options;
    }

    



    public void appendButtonsFromJSON(JSONArray buttons) {
        try {
            for (int i = 0; i < buttons.length(); i++) {
                this.buttons.put(buttons.get(i));
            }
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error parsing buttons from JSON", e);
        }
    }

    public void appendButton(String label, int callbackId) {
        final JSONObject button = new JSONObject();
        try {
            button.put("label", label);
            button.put("callback", callbackId);
            this.buttons.put(button);
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error creating button", e);
        }
    }

    public DoorHanger.OnButtonClickListener getButtonClickListener() {
        return this.buttonClickListener;
    }

    public JSONArray getButtons() {
        return buttons;
    }

    public void setLink(String label, String url, String delimiter) {
        this.link = new Link(label, url, delimiter);
    }

    public Link getLink() {
        return link;
    }
}
