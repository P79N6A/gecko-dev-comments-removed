




package org.mozilla.gecko.util;

public class StringUtils {
    












    public static boolean isSearchQuery(String text) {
        text = text.trim();
        if (text.length() == 0)
            return false;

        int colon = text.indexOf(':');
        int dot = text.indexOf('.');
        int space = text.indexOf(' ');

        
        boolean spacedOut = space > -1 && (space < colon || space < dot);

        return spacedOut || (dot == -1 && colon == -1);
    }
}
