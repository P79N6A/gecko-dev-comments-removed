




package org.mozilla.gecko.util;

import android.net.Uri;
import android.text.TextUtils;

public class StringUtils {
    private static final String LOGTAG = "GeckoStringUtils";

    private static final String FILTER_URL_PREFIX = "filter://";
    private static final String USER_ENTERED_URL_PREFIX = "user-entered:";

    


















    public static boolean isSearchQuery(String text, boolean wasSearchQuery) {
        
        text = text.trim();
        if (text.length() == 0)
            return wasSearchQuery;

        int colon = text.indexOf(':');
        int dot = text.indexOf('.');
        int space = text.indexOf(' ');

        
        if (space > -1 && (colon == -1 || space < colon) && (dot == -1 || space < dot)) {
            return true;
        }
        
        if (dot > -1 || colon > -1) {
            return false;
        }
        
        return wasSearchQuery;
    }

    public static class UrlFlags {
        public static final int NONE = 0;
        public static final int STRIP_HTTPS = 1;
    }

    public static String stripScheme(String url) {
        return stripScheme(url, UrlFlags.NONE);
    }

    public static String stripScheme(String url, int flags) {
        if (url == null) {
            return url;
        }

        int start = 0;
        int end = url.length();

        if (url.startsWith("http://")) {
            start = 7;
        } else if (url.startsWith("https://") && flags == UrlFlags.STRIP_HTTPS) {
            start = 8;
        }

        if (url.endsWith("/")) {
            end--;
        }

        return url.substring(start, end);
    }

    public static String stripCommonSubdomains(String host) {
        if (host == null) {
            return host;
        }

        
        
        int start = 0;

        if (host.startsWith("www.")) {
            start = 4;
        } else if (host.startsWith("mobile.")) {
            start = 7;
        } else if (host.startsWith("m.")) {
            start = 2;
        }

        return host.substring(start);
    }

    


    public static String getQueryParameter(String url, String desiredKey) {
        if (TextUtils.isEmpty(url) || TextUtils.isEmpty(desiredKey)) {
            return null;
        }

        final String[] urlParts = url.split("\\?");
        if (urlParts.length < 2) {
            return null;
        }

        final String query = urlParts[1];
        for (final String param : query.split("&")) {
            final String pair[] = param.split("=");
            final String key = Uri.decode(pair[0]);

            
            if (TextUtils.isEmpty(key) || !key.equals(desiredKey)) {
                continue;
            }
            
            if (pair.length < 2) {
                continue;
            }
            final String value = Uri.decode(pair[1]);
            if (TextUtils.isEmpty(value)) {
                return null;
            }
            return value;
        }

        return null;
    }

    public static boolean isFilterUrl(String url) {
        if (TextUtils.isEmpty(url)) {
            return false;
        }

        return url.startsWith(FILTER_URL_PREFIX);
    }

    public static String getFilterFromUrl(String url) {
        if (TextUtils.isEmpty(url)) {
            return null;
        }

        return url.substring(FILTER_URL_PREFIX.length());
    }

    public static boolean isShareableUrl(final String url) {
        final String scheme = Uri.parse(url).getScheme();
        return !("about".equals(scheme) || "chrome".equals(scheme) ||
                "file".equals(scheme) || "resource".equals(scheme));
    }

    public static boolean isUserEnteredUrl(String url) {
        return (url != null && url.startsWith(USER_ENTERED_URL_PREFIX));
    }

    








    public static String decodeUserEnteredUrl(String url) {
        Uri uri = Uri.parse(url);
        if ("user-entered".equals(uri.getScheme())) {
            return uri.getSchemeSpecificPart();
        }
        return url;
    }

    public static String encodeUserEnteredUrl(String url) {
        return Uri.fromParts("user-entered", url, null).toString();
    }
}
