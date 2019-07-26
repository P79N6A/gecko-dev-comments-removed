



package org.mozilla.gecko;

import android.net.Uri;

public class ReaderModeUtils {
    private static final String LOGTAG = "ReaderModeUtils";

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
