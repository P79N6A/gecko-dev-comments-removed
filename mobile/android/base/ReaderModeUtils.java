



package org.mozilla.gecko;

import android.net.Uri;
import android.text.TextUtils;

public class ReaderModeUtils {
    private static final String LOGTAG = "ReaderModeUtils";

    public static String getUrlFromAboutReader(String aboutReaderUrl) {
        if (aboutReaderUrl == null)
            return null;

        String[] urlParts = aboutReaderUrl.split("\\?");
        if (urlParts.length < 2)
            return null;

        String query = urlParts[1];
        for (String param : query.split("&")) {
            String pair[] = param.split("=");
            String key = Uri.decode(pair[0]);

            
            if (TextUtils.isEmpty(key) || !key.equals("url"))
                continue;

            
            if (pair.length < 2)
                continue;

            String url = Uri.decode(pair[1]);
            if (TextUtils.isEmpty(url))
                return null;

            return url;
        }

        return null;
    }

    public static boolean isEnteringReaderMode(String currentUrl, String newUrl) {
        if (currentUrl == null || newUrl == null) {
            return false;
        }

        if (!AboutPages.isAboutReader(newUrl)) {
            return false;
        }

        String urlFromAboutReader = getUrlFromAboutReader(newUrl);
        if (urlFromAboutReader == null) {
            return false;
        }

        return urlFromAboutReader.equals(currentUrl);
    }

    public static String getAboutReaderForUrl(String url) {
        return getAboutReaderForUrl(url, -1);
    }

    public static String getAboutReaderForUrl(String url, int tabId) {
        String aboutReaderUrl = AboutPages.READER + "?url=" + Uri.encode(url);

        if (tabId >= 0) {
            aboutReaderUrl += "&tabId=" + tabId;
        }

        return aboutReaderUrl;
    }
}
