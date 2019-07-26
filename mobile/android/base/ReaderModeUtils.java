



package org.mozilla.gecko;

import android.net.Uri;
import android.text.TextUtils;

public class ReaderModeUtils {
    private static final String LOGTAG = "ReaderModeUtils";

    public static boolean isAboutReader(String url) {
        if (url == null)
            return false;

        return url.startsWith("about:reader");
    }

    public static String getAboutReaderForUrl(String url, boolean inReadingList) {
        return getAboutReaderForUrl(url, -1, inReadingList);
    }

    public static String getAboutReaderForUrl(String url, int tabId, boolean inReadingList) {
        String aboutReaderUrl = "about:reader?url=" + Uri.encode(url) +
                                "&readingList=" + (inReadingList ? 1 : 0);

        if (tabId >= 0)
            aboutReaderUrl += "&tabId=" + tabId;

        return aboutReaderUrl;
    }
}
