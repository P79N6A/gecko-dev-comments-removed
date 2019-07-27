package org.mozilla.gecko.tests;


public class StringHelper {
    private StringHelper() {}

    public static final String OK = "OK";

    
    public static final String[] DEFAULT_BOOKMARKS_TITLES = new String[] {
        "Firefox: About your browser",
        "Firefox: Support",
        "Firefox: Customize with add-ons"
    };
    public static final String[] DEFAULT_BOOKMARKS_URLS = new String[] {
        "about:firefox",
        "https://support.mozilla.org/products/mobile",
        "https://addons.mozilla.org/android/"
    };
    public static final int DEFAULT_BOOKMARKS_COUNT = DEFAULT_BOOKMARKS_TITLES.length;

    
    public static final String ABOUT_BLANK_URL = "about:blank";
    public static final String ABOUT_FIREFOX_URL = "about:firefox";
    public static final String ABOUT_RIGHTS_URL = "about:rights";
    public static final String ABOUT_BUILDCONFIG_URL = "about:buildconfig";
    public static final String ABOUT_FEEDBACK_URL = "about:feedback";
    public static final String ABOUT_HEALTHREPORT_URL = "about:healthreport";
    public static final String ABOUT_DOWNLOADS_URL = "about:downloads";
    public static final String ABOUT_HOME_URL = "about:home";
    public static final String ABOUT_ADDONS_URL = "about:addons";
    public static final String ABOUT_APPS_URL = "about:apps";
    public static final String ABOUT_ABOUT_URL = "about:about";
    public static final String ABOUT_SCHEME = "about:";

    
    public static final String ABOUT_HOME_TITLE = "";

    
    public static final String CONTEXT_MENU_BOOKMARK_LINK = "Bookmark Link";
    public static final String CONTEXT_MENU_OPEN_LINK_IN_NEW_TAB = "Open Link in New Tab";
    public static final String CONTEXT_MENU_OPEN_IN_NEW_TAB = "Open in New Tab";
    public static final String CONTEXT_MENU_OPEN_LINK_IN_PRIVATE_TAB = "Open Link in Private Tab";
    public static final String CONTEXT_MENU_OPEN_IN_PRIVATE_TAB = "Open in Private Tab";
    public static final String CONTEXT_MENU_COPY_LINK = "Copy Link";
    public static final String CONTEXT_MENU_SHARE_LINK = "Share Link";
    public static final String CONTEXT_MENU_EDIT = "Edit";
    public static final String CONTEXT_MENU_SHARE = "Share";
    public static final String CONTEXT_MENU_REMOVE = "Remove";
    public static final String CONTEXT_MENU_COPY_ADDRESS = "Copy Address";
    public static final String CONTEXT_MENU_EDIT_SITE_SETTINGS = "Edit Site Settings";
    public static final String CONTEXT_MENU_ADD_TO_HOME_SCREEN = "Add to Home Screen";
    public static final String CONTEXT_MENU_PIN_SITE = "Pin Site";
    public static final String CONTEXT_MENU_UNPIN_SITE = "Unpin Site";

    
    public static final String[] CONTEXT_MENU_ITEMS_IN_PRIVATE_TAB = new String[] {
        CONTEXT_MENU_OPEN_LINK_IN_PRIVATE_TAB,
        CONTEXT_MENU_COPY_LINK,
        CONTEXT_MENU_SHARE_LINK,
        CONTEXT_MENU_BOOKMARK_LINK
    };

    public static final String[] CONTEXT_MENU_ITEMS_IN_NORMAL_TAB = new String[] {
        CONTEXT_MENU_OPEN_LINK_IN_NEW_TAB,
        CONTEXT_MENU_OPEN_LINK_IN_PRIVATE_TAB,
        CONTEXT_MENU_COPY_LINK,
        CONTEXT_MENU_SHARE_LINK,
        CONTEXT_MENU_BOOKMARK_LINK
    };

    public static final String[] BOOKMARK_CONTEXT_MENU_ITEMS = new String[] {
        CONTEXT_MENU_OPEN_IN_NEW_TAB,
        CONTEXT_MENU_OPEN_IN_PRIVATE_TAB,
        CONTEXT_MENU_COPY_ADDRESS,
        CONTEXT_MENU_SHARE,
        CONTEXT_MENU_EDIT,
        CONTEXT_MENU_REMOVE,
        CONTEXT_MENU_ADD_TO_HOME_SCREEN
    };

    public static final String[] CONTEXT_MENU_ITEMS_IN_URL_BAR = new String[] {
        CONTEXT_MENU_SHARE,
        CONTEXT_MENU_COPY_ADDRESS,
        CONTEXT_MENU_EDIT_SITE_SETTINGS,
        CONTEXT_MENU_ADD_TO_HOME_SCREEN
    };

    public static final String TITLE_PLACE_HOLDER = "Search or enter address";

    
    
    public static final String ROBOCOP_BIG_LINK_URL = "/robocop/robocop_big_link.html";
    public static final String ROBOCOP_BIG_MAILTO_URL = "/robocop/robocop_big_mailto.html";
    public static final String ROBOCOP_BLANK_PAGE_01_URL = "/robocop/robocop_blank_01.html";
    public static final String ROBOCOP_BLANK_PAGE_02_URL = "/robocop/robocop_blank_02.html";
    public static final String ROBOCOP_BLANK_PAGE_03_URL = "/robocop/robocop_blank_03.html";
    public static final String ROBOCOP_BLANK_PAGE_04_URL = "/robocop/robocop_blank_04.html";
    public static final String ROBOCOP_BLANK_PAGE_05_URL = "/robocop/robocop_blank_05.html";
    public static final String ROBOCOP_BOXES_URL = "/robocop/robocop_boxes.html";
    public static final String ROBOCOP_GEOLOCATION_URL = "/robocop/robocop_geolocation.html";
    public static final String ROBOCOP_LOGIN_URL = "/robocop/robocop_login.html";
    public static final String ROBOCOP_POPUP_URL = "/robocop/robocop_popup.html";
    public static final String ROBOCOP_OFFLINE_STORAGE_URL = "/robocop/robocop_offline_storage.html";
    public static final String ROBOCOP_PICTURE_LINK_URL = "/robocop/robocop_picture_link.html";
    public static final String ROBOCOP_SEARCH_URL = "/robocop/robocop_search.html";
    public static final String ROBOCOP_TEXT_PAGE_URL = "/robocop/robocop_text_page.html";
    public static final String ROBOCOP_ADOBE_FLASH_URL = "/robocop/robocop_adobe_flash.html";
    public static final String ROBOCOP_INPUT_URL = "/robocop/robocop_input.html";

    private static final String ROBOCOP_JS_HARNESS_URL = "/robocop/robocop_javascript.html";

    









    public static String getHarnessUrlForJavascript(String javascriptUrl) {
        
        return ROBOCOP_JS_HARNESS_URL +
                "?slug=" + System.currentTimeMillis() +
                "&path=" + javascriptUrl;
    }

    
    public static final String ROBOCOP_BIG_LINK_TITLE = "Big Link";
    public static final String ROBOCOP_BIG_MAILTO_TITLE = "Big Mailto";
    public static final String ROBOCOP_BLANK_PAGE_01_TITLE = "Browser Blank Page 01";
    public static final String ROBOCOP_BLANK_PAGE_02_TITLE = "Browser Blank Page 02";
    public static final String ROBOCOP_BLANK_PAGE_03_TITLE = "Browser Blank Page 03";
    public static final String ROBOCOP_BLANK_PAGE_04_TITLE = "Browser Blank Page 04";
    public static final String ROBOCOP_BLANK_PAGE_05_TITLE = "Browser Blank Page 05";
    public static final String ROBOCOP_BOXES_TITLE = "Browser Box test";
    public static final String ROBOCOP_GEOLOCATION_TITLE = "Geolocation Test Page";
    public static final String ROBOCOP_LOGIN_TITLE = "Robocop Login";
    public static final String ROBOCOP_OFFLINE_STORAGE_TITLE = "Robocop offline storage";
    public static final String ROBOCOP_PICTURE_LINK_TITLE = "Picture Link";
    public static final String ROBOCOP_SEARCH_TITLE = "Robocop Search Engine";
    public static final String ROBOCOP_TEXT_PAGE_TITLE = "Robocop Text Page";
    public static final String ROBOCOP_INPUT_TITLE = "Robocop Input";
    public static final String ROBOCOP_SELECTION_HANDLER_TITLE = "Automated Text Selection tests for Mobile";

    
    public static final String DISTRIBUTION1_LABEL = "Distribution 1";
    public static final String DISTRIBUTION2_LABEL = "Distribution 2";

    
    
    public static final String CUSTOMIZE_SECTION_LABEL = "Customize";
    public static final String DISPLAY_SECTION_LABEL = "Display";
    public static final String PRIVACY_SECTION_LABEL = "Privacy";
    public static final String MOZILLA_SECTION_LABEL = "Mozilla";
    public static final String DEVELOPER_TOOLS_SECTION_LABEL = "Developer tools";

    
    
    public static final String SYNC_LABEL = "Sync";
    public static final String IMPORT_FROM_ANDROID_LABEL = "Import from Android";
    public static final String TABS_LABEL = "Tabs";

    
    public static final String TEXT_SIZE_LABEL = "Text size";
    public static final String TITLE_BAR_LABEL = "Title bar";
    public static final String SCROLL_TITLE_BAR_LABEL = "Full-screen browsing";
    public static final String TEXT_REFLOW_LABEL = "Text reflow";
    public static final String CHARACTER_ENCODING_LABEL = "Character encoding";
    public static final String PLUGINS_LABEL = "Plugins";

    
    public static final String SHOW_PAGE_TITLE_LABEL = "Show page title";
    public static final String SHOW_PAGE_ADDRESS_LABEL = "Show page address";

    
    public static final String TRACKING_PROTECTION_LABEL = "Tracking protection";
    public static final String DNT_LABEL = "Do not track";
    public static final String COOKIES_LABEL = "Cookies";
    public static final String REMEMBER_PASSWORDS_LABEL = "Remember passwords";
    public static final String MASTER_PASSWORD_LABEL = "Use master password";
    public static final String CLEAR_PRIVATE_DATA_LABEL = "Clear now";

    
    public static final String BRAND_NAME = "(Fennec|Nightly|Aurora|Firefox Beta|Firefox)";
    public static final String ABOUT_LABEL = "About " + BRAND_NAME;
    public static final String FAQS_LABEL = "FAQs";
    public static final String FEEDBACK_LABEL = "Give feedback";
    public static final String LOCATION_SERVICES_LABEL = "Mozilla Location Service";
    public static final String HEALTH_REPORT_LABEL = BRAND_NAME + " Health Report";
    public static final String MY_HEALTH_REPORT_LABEL = "View my Health Report";

    
    public static final String PAINT_FLASHING_LABEL = "Paint flashing";
    public static final String REMOTE_DEBUGGING_LABEL = "Remote debugging";
    public static final String LEARN_MORE_LABEL = "Learn more";

    
    public static final String HISTORY_LABEL = "HISTORY";
    public static final String TOP_SITES_LABEL = "TOP SITES";
    public static final String BOOKMARKS_LABEL = "BOOKMARKS";
    public static final String READING_LIST_LABEL = "READING LIST";
    public static final String TODAY_LABEL = "Today";
    public static final String TABS_FROM_LAST_TIME_LABEL = "Open all tabs from last time";

    
    public static final String BOOKMARKS_UP_TO = "Up to %s";
    public static final String BOOKMARKS_ROOT_LABEL = "Bookmarks";
    public static final String DESKTOP_FOLDER_LABEL = "Desktop Bookmarks";
    public static final String TOOLBAR_FOLDER_LABEL = "Bookmarks Toolbar";
    public static final String BOOKMARKS_MENU_FOLDER_LABEL = "Bookmarks Menu";
    public static final String UNSORTED_FOLDER_LABEL = "Unsorted Bookmarks";

    
    public static final String NEW_TAB_LABEL = "New Tab";
    public static final String NEW_PRIVATE_TAB_LABEL = "New Private Tab";
    public static final String SHARE_LABEL = "Share";
    public static final String FIND_IN_PAGE_LABEL = "Find in Page";
    public static final String DESKTOP_SITE_LABEL = "Request Desktop Site";
    public static final String PDF_LABEL = "Save as PDF";
    public static final String DOWNLOADS_LABEL = "Downloads";
    public static final String ADDONS_LABEL = "Add-ons";
    public static final String APPS_LABEL = "Apps";
    public static final String SETTINGS_LABEL = "Settings";
    public static final String GUEST_MODE_LABEL = "New Guest Session";

    
    public static final String TOOLS_LABEL = "Tools";
    public static final String PAGE_LABEL = "Page";

    
    public static final String MORE_LABEL = "More";
    public static final String RELOAD_LABEL = "Reload";
    public static final String FORWARD_LABEL = "Forward";
    public static final String BOOKMARK_LABEL = "Bookmark";

    
    public static final String BOOKMARK_ADDED_LABEL = "Bookmark added";
    public static final String BOOKMARK_REMOVED_LABEL = "Bookmark removed";
    public static final String BOOKMARK_UPDATED_LABEL = "Bookmark updated";
    public static final String BOOKMARK_OPTIONS_LABEL = "Options";

    
    public static final String EDIT_BOOKMARK = "Edit Bookmark";

    
    public static final String GEO_MESSAGE = "Share your location with";
    public static final String GEO_ALLOW = "Share";
    public static final String GEO_DENY = "Don't share";

    public static final String OFFLINE_MESSAGE = "to store data on your device for offline use";
    public static final String OFFLINE_ALLOW = "Allow";
    public static final String OFFLINE_DENY = "Don't allow";

    public static final String LOGIN_MESSAGE = "Save password";
    public static final String LOGIN_ALLOW = "Save";
    public static final String LOGIN_DENY = "Don't save";

    public static final String POPUP_MESSAGE = "prevented this site from opening";
    public static final String POPUP_ALLOW = "Show";
    public static final String POPUP_DENY = "Don't show";
}
