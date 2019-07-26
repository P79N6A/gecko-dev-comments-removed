




package org.mozilla.gecko;

import android.text.TextUtils;

public class AboutPages {
    
    public static final String ADDONS          = "about:addons";
    public static final String APPS            = "about:apps";
    public static final String CONFIG          = "about:config";
    public static final String DOWNLOADS       = "about:downloads";
    public static final String FIREFOX         = "about:firefox";
    public static final String HEALTHREPORT    = "about:healthreport";
    public static final String HOME            = "about:home";
    public static final String PRIVATEBROWSING = "about:privatebrowsing";
    public static final String READER          = "about:reader";
    public static final String UPDATER         = "about:";

    public static final String URL_FILTER = "about:%";

    public static final boolean isAboutPage(final String url) {
        return url.startsWith("about:");
    }

    public static final boolean isTitlelessAboutPage(final String url) {
        return isAboutHome(url) ||
               PRIVATEBROWSING.equals(url);
    }

    public static final boolean isAboutHome(final String url) {
        if (url == null || !url.startsWith(HOME)) {
            return false;
        }
        
        
        
        return HOME.equals(url.split("\\?")[0]);
    }

    public static final String getPageIdFromAboutHomeUrl(final String aboutHomeUrl) {
        if (aboutHomeUrl == null) {
            return null;
        }

        final String[] urlParts = aboutHomeUrl.split("\\?");
        if (urlParts.length < 2) {
            return null;
        }

        final String query = urlParts[1];
        for (final String param : query.split("&")) {
            final String pair[] = param.split("=");
            final String key = pair[0];

            
            if (TextUtils.isEmpty(key) || !key.equals("page")) {
                continue;
            }
            
            if (pair.length < 2) {
                continue;
            }
            return pair[1];
        }

        return null;
    }

    public static final boolean isAboutReader(final String url) {
        if (url == null) {
            return false;
        }
        return url.startsWith(READER);
    }

    private static final String[] DEFAULT_ICON_PAGES = new String[] {
        HOME,

        ADDONS,
        CONFIG,
        DOWNLOADS,
        FIREFOX,
        HEALTHREPORT,
        UPDATER
    };

    


    public static String[] getDefaultIconPages() {
        return DEFAULT_ICON_PAGES;
    }

    public static boolean isDefaultIconPage(final String url) {
        if (url == null ||
            !url.startsWith("about:")) {
            return false;
        }

        
        for (int i = 0; i < DEFAULT_ICON_PAGES.length; ++i) {
            if (DEFAULT_ICON_PAGES[i].equals(url)) {
                return true;
            }
        }
        return false;
    }
}

