




package org.mozilla.gecko;

public final class StringUtils {

    private static final String HTTP_PREFIX = "http://";
    private static final String PROTOCOL_DELIMITER = "://";

    








    public static String prettyURL(String url) {
        if (url == null)
            return url;

        
        if (url.startsWith(HTTP_PREFIX))
            url = url.substring(HTTP_PREFIX.length());

        
        int delimiterIndex = url.indexOf(PROTOCOL_DELIMITER);
        int fromIndex = (delimiterIndex == -1 ? 0 : delimiterIndex + PROTOCOL_DELIMITER.length());
        int firstSlash = url.indexOf('/', fromIndex);
        if (firstSlash == url.length() - 1)
            url = url.substring(0, firstSlash);

        return url;
    }

}
