




package org.mozilla.gecko;




public interface TelemetryContract {

    



    public interface Event {
        
        public static final String POLICY_NOTIFICATION_SUCCESS = "policynotification.success.1:";

        
        public static final String TOP_SITES_PIN = "pin.1";

        
        public static final String TOP_SITES_UNPIN = "unpin.1";

        
        public static final String TOP_SITES_EDIT = "edit.1";

        
        public static final String PANEL_SET_DEFAULT = "setdefault.1";

        
        public static final String SHARE = "share.1";

        
        public static final String SANITIZE = "sanitize.1";
    }

    



    public interface Method {
        
        public static final String LIST = "list";

        
        public static final String BUTTON = "button";

        
        public static final String DIALOG = "dialog";
    }

    



    public interface Session {
        
        public static final String HOME = "home.1";

        
        
        public static final String HOME_PANEL = "homepanel.1:";
    }

    



    public interface Reason {}
}
