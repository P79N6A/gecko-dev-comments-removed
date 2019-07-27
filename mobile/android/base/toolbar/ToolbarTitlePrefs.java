




package org.mozilla.gecko.toolbar;

import org.mozilla.gecko.PrefsHelper;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.util.ThreadUtils;

class ToolbarTitlePrefs {
    static final String PREF_TITLEBAR_MODE = "browser.chrome.titlebarMode";
    static final String PREF_TRIM_URLS = "browser.urlbar.trimURLs";

    interface OnChangeListener {
        public void onChange();
    }

    final String[] prefs = {
        PREF_TITLEBAR_MODE,
        PREF_TRIM_URLS
    };

    private boolean mShowUrl;
    private boolean mTrimUrls;

    private Integer mPrefObserverId;

    ToolbarTitlePrefs() {
        mTrimUrls = true;

        mPrefObserverId = PrefsHelper.getPrefs(prefs, new TitlePrefsHandler());
    }

    boolean shouldShowUrl() {
        return mShowUrl;
    }

    boolean shouldTrimUrls() {
        return mTrimUrls;
    }

    void close() {
        if (mPrefObserverId != null) {
             PrefsHelper.removeObserver(mPrefObserverId);
             mPrefObserverId = null;
        }
    }

    private class TitlePrefsHandler extends PrefsHelper.PrefHandlerBase {
        @Override
        public void prefValue(String pref, String str) {
            
            int value = Integer.parseInt(str);
            boolean shouldShowUrl = (value == 1);

            if (shouldShowUrl == mShowUrl) {
                return;
            }
            mShowUrl = shouldShowUrl;

            triggerChangeListener();
        }

        @Override
        public void prefValue(String pref, boolean value) {
            
            if (value == mTrimUrls) {
                return;
            }
            mTrimUrls = value;

            triggerChangeListener();
        }

        @Override
        public boolean isObserver() {
            
            
            return true;
        }

        private void triggerChangeListener() {
            ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    final Tabs tabs = Tabs.getInstance();
                    final Tab tab = tabs.getSelectedTab();
                    if (tab != null) {
                        tabs.notifyListeners(tab, Tabs.TabEvents.TITLE);
                    }
                }
            });
        }
    }
}
