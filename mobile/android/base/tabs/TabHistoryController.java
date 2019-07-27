



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
        int historyIndex = tab.getHistoryIndex();
        int historySize = tab.getHistorySize();

        switch(action) {
        case BACK:
            if (!tab.canDoBack()) {
                return false;
            }
            return showHistory(Math.max(historyIndex - Tab.MAX_HISTORY_LIST_SIZE, 0), historyIndex, historyIndex);

        case FORWARD:
            if (!tab.canDoForward()) {
                return false;
            }
            return showHistory(historyIndex, Math.min(historySize - 1, historyIndex + Tab.MAX_HISTORY_LIST_SIZE), historyIndex);

        case ALL:
            if (!tab.canDoForward() && !tab.canDoBack()) {
                return false;
            }

            int min = historyIndex - Tab.MAX_HISTORY_LIST_SIZE / 2;
            int max = historyIndex + Tab.MAX_HISTORY_LIST_SIZE / 2;
            if (min < 0) {
                max -= min;
            }
            if (max > historySize - 1) {
                min -= max - (historySize - 1);
                max = historySize - 1;
            }
            min = Math.max(min, 0);

            return showHistory(min, max, historyIndex);

        default:
            return false;
        }
    }

    


    private boolean showHistory(final int fromIndex, final int toIndex, final int selIndex) {
        JSONObject json = new JSONObject();
        try {
            json.put("fromIndex", fromIndex);
            json.put("toIndex", toIndex);
            json.put("selIndex", selIndex);
        } catch (JSONException e) {
            Log.e(LOGTAG, "JSON error", e);
        }

        GeckoAppShell.sendRequestToGecko(new GeckoRequest("Session:GetHistory", json) {
            @Override
            public void onResponse(NativeJSObject nativeJSObject) {
                










                final NativeJSObject[] historyItems = nativeJSObject.getObjectArray("historyItems");
                final List<TabHistoryPage> historyPageList = new ArrayList<>(historyItems.length);

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
