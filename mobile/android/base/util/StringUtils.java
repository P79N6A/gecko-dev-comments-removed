




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
}
