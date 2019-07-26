




package org.mozilla.gecko.util;

public class StringUtils {
    


















    public static boolean isSearchQuery(String text, boolean wasSearchQuery) {
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

    public static String stripScheme(String url) {
        if (url == null) {
            return url;
        }

        int start = 0;
        int end = url.length();

        if (url.startsWith("http://")) {
            start = 7;
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
}
