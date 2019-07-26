




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
        if (url == null)
            return url;

        if (url.startsWith("http://")) {
            return url.substring(7);
        }
        return url;
    }

    public static String stripCommonSubdomains(String host) {
        if (host == null)
            return host;
        
        
        if (host.startsWith("www.")) return host.substring(4);
        else if (host.startsWith("mobile.")) return host.substring(7);
        else if (host.startsWith("m.")) return host.substring(2);
        return host;
    }

}
