




package org.mozilla.gecko;

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
        return HOME.equals(url) ||
               PRIVATEBROWSING.equals(url);
    }

    public static final boolean isAboutHome(final String url) {
        return HOME.equals(url);
    }

    public static final boolean isAboutReader(final String url) {
        if (url == null) {
            return false;
        }
        return url.startsWith(READER);
    }
}

