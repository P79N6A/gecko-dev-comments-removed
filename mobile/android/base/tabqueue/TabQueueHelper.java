




package org.mozilla.gecko.tabqueue;

import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.util.ThreadUtils;

import android.text.TextUtils;
import android.util.Log;
import org.json.JSONArray;
import org.json.JSONException;

import java.io.IOException;

public class TabQueueHelper {
    private static final String LOGTAG = "Gecko" + TabQueueHelper.class.getSimpleName();

    public static final String FILE_NAME = "tab_queue_url_list.json";

    







    public static void queueURL(final GeckoProfile profile, final String url, final String filename) {
        ThreadUtils.assertNotOnUiThread();

        JSONArray jsonArray = profile.readJSONArrayFromFile(filename);

        jsonArray.put(url);

        profile.writeFile(filename, jsonArray.toString());
    }
}