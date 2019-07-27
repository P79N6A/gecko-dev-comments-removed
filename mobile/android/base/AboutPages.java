




package org.mozilla.gecko;

import org.mozilla.gecko.util.StringUtils;

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

    public static final String PANEL_PARAM = "panel";

    public static final boolean isAboutPage(final String url) {
        return url != null && url.startsWith("about:");
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

    public static final String getPanelIdFromAboutHomeUrl(String aboutHomeUrl) {
        return StringUtils.getQueryParameter(aboutHomeUrl, PANEL_PARAM);
    }

    public static final boolean isAboutReader(final String url) {
        if (url == null) {
            return false;
        }
        return url.startsWith(READER);
    }

    private static final String[] DEFAULT_ICON_PAGES = new String[] {
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

    public static boolean isBuiltinIconPage(final String url) {
        if (url == null ||
            !url.startsWith("about:")) {
            return false;
        }

        
        if (isAboutHome(url)) {
            return true;
        }

        
        for (int i = 0; i < DEFAULT_ICON_PAGES.length; ++i) {
            if (DEFAULT_ICON_PAGES[i].equals(url)) {
                return true;
            }
        }
        return false;
    }
}

