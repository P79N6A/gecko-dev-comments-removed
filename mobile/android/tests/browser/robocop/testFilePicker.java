



package org.mozilla.gecko.tests;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.fFail;

import org.mozilla.gecko.EventDispatcher;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.util.GeckoEventListener;

import org.json.JSONException;
import org.json.JSONObject;

public class testFilePicker extends JavascriptTest implements GeckoEventListener {
    private static final String TEST_FILENAME = "/mnt/sdcard/my-favorite-martian.png";

    public testFilePicker() {
        super("testFilePicker.js");
    }

    @Override
    public void handleMessage(String event, final JSONObject message) {
        
        
        if (event.equals("FilePicker:Show")) {
            try {
                message.put("file", TEST_FILENAME);
            } catch (JSONException ex) {
                fFail("Can't add filename to message " + TEST_FILENAME);
            }

            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("FilePicker:Result", message.toString()));
        }
    }

    @Override
    public void setUp() throws Exception {
        super.setUp();

        EventDispatcher.getInstance().registerGeckoThreadListener(this, "FilePicker:Show");
    }

    @Override
    public void tearDown() throws Exception {
        super.tearDown();

        EventDispatcher.getInstance().unregisterGeckoThreadListener(this, "FilePicker:Show");
    }
}
