



package org.mozilla.gecko.tabs;

import java.util.ArrayList;
import java.util.List;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.util.GeckoRequest;
import org.mozilla.gecko.util.NativeJSObject;

import android.util.Log;

public class TabHistoryController {
    private static final String LOGTAG = "TabHistoryController";
    private final OnShowTabHistory showTabHistoryListener;

    public static enum HistoryAction {
        ALL,
        BACK,
        FORWARD
    };

    public interface OnShowTabHistory {
        void onShowHistory(List<TabHistoryPage>historyPageList, int toIndex);
    }

    public TabHistoryController(OnShowTabHistory showTabHistoryListener) {
        this.showTabHistoryListener = showTabHistoryListener;
    }

    


    public boolean showTabHistory(final Tab tab, final HistoryAction action) {
        JSONObject json = new JSONObject();
        try {
            json.put("action", action.name());
            json.put("tabId", tab.getId());
        } catch (JSONException e) {
            Log.e(LOGTAG, "JSON error", e);
        }

        GeckoAppShell.sendRequestToGecko(new GeckoRequest("Session:GetHistory", json) {
            @Override
            public void onResponse(NativeJSObject nativeJSObject) {
                













                final NativeJSObject[] historyItems = nativeJSObject.getObjectArray("historyItems");
                if (historyItems.length == 0) {
                    
                    return;
                }

                final List<TabHistoryPage> historyPageList = new ArrayList<>(historyItems.length);
                final int toIndex = nativeJSObject.getInt("toIndex");

                for (NativeJSObject obj : historyItems) {
                    final String title = obj.getString("title");
                    final String url = obj.getString("url");
                    final boolean selected = obj.getBoolean("selected");
                    historyPageList.add(new TabHistoryPage(title, url, selected));
                }

                showTabHistoryListener.onShowHistory(historyPageList, toIndex);
            }
        });
        return true;
    }
}
