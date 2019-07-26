




package org.mozilla.gecko;






public interface TelemetryContract {

    





    public interface Event {
        
        public static final String ACTION = "action.1";

        
        public static final String CANCEL = "cancel.1";

        
        
        public static final String LAUNCH = "launch.1";

        
        public static final String LOAD_URL = "loadurl.1";

        public static final String LOCALE_BROWSER_RESET = "locale.browser.reset.1";
        public static final String LOCALE_BROWSER_SELECTED = "locale.browser.selected.1";
        public static final String LOCALE_BROWSER_UNSELECTED = "locale.browser.unselected.1";

        
        public static final String PANEL_SET_DEFAULT = "setdefault.1";

        
        public static final String POLICY_NOTIFICATION_SUCCESS = "policynotification.success.1:";

        
        public static final String SANITIZE = "sanitize.1";

        
        
        public static final String SAVE = "save.1";

        
        public static final String SHARE = "share.1";

        
        public static final String TOP_SITES_EDIT = "edit.1";

        
        public static final String TOP_SITES_PIN = "pin.1";

        
        public static final String TOP_SITES_UNPIN = "unpin.1";

        
        
        public static final String UNSAVE = "unsave.1";
    }

    





    public interface Method {
        
        public static final String ACTIONBAR = "actionbar";

        
        public static final String BACK = "back";

        
        public static final String BUTTON = "button";

        
        public static final String CONTEXT_MENU = "contextmenu";

        
        public static final String DIALOG = "dialog";

        
        public static final String GRID_ITEM = "griditem";

        
        public static final String INTENT = "intent";

        
        public static final String LIST = "list";

        
        public static final String LIST_ITEM = "listitem";

        
        public static final String MENU = "menu";

        
        
        public static final String PAGEACTION = "pageaction";

        
        public static final String SUGGESTION = "suggestion";
    }

    





    public interface Session {
        
        public static final String AWESOMESCREEN = "awesomescreen.1";

        
        public static final String FIRSTRUN = "firstrun.1";

        
        public static final String FRECENCY = "frecency.1";

        
        public static final String HOME = "home.1";

        
        
        public static final String HOME_PANEL = "homepanel.1:";

        
        
        public static final String READER = "reader.1";
    }

    





    public interface Reason {
        
        public static final String COMMIT = "commit";
    }
}
