




package org.mozilla.gecko;






public interface TelemetryContract {

    





    public enum Event {
        
        ACTION("action.1"),

        
        CANCEL("cancel.1"),

        
        EDIT("edit.1"),

        
        
        LAUNCH("launch.1"),

        
        LOAD_URL("loadurl.1"),

        LOCALE_BROWSER_RESET("locale.browser.reset.1"),
        LOCALE_BROWSER_SELECTED("locale.browser.selected.1"),
        LOCALE_BROWSER_UNSELECTED("locale.browser.unselected.1"),

        
        PANEL_SET_DEFAULT("setdefault.1"),

        
        PIN("pin.1"),

        
        POLICY_NOTIFICATION_SUCCESS("policynotification.success.1"),

        
        SANITIZE("sanitize.1"),

        
        
        SAVE("save.1"),

        
        SHARE("share.1"),

        
        UNPIN("unpin.1"),

        
        
        UNSAVE("unsave.1"),
        ;

        private final String string;

        Event(final String string) {
            this.string = string;
        }

        @Override
        public String toString() {
            return string;
        }
    }

    





    public enum Method {
        
        ACTIONBAR("actionbar"),

        
        BACK("back"),

        
        BUTTON("button"),

        
        CONTEXT_MENU("contextmenu"),

        
        DIALOG("dialog"),

        
        GRID_ITEM("griditem"),

        
        INTENT("intent"),

        
        LIST("list"),

        
        LIST_ITEM("listitem"),

        
        MENU("menu"),

        
        NONE(null),

        
        
        PAGEACTION("pageaction"),

        
        SUGGESTION("suggestion"),
        ;

        private final String string;

        Method(final String string) {
            this.string = string;
        }

        @Override
        public String toString() {
            return string;
        }
    }

    





    public enum Session {
        
        AWESOMESCREEN("awesomescreen.1"),

        
        FIRSTRUN("firstrun.1"),

        
        FRECENCY("frecency.1"),

        
        HOME("home.1"),

        
        
        HOME_PANEL("homepanel.1"),

        
        
        READER("reader.1"),
        ;

        private final String string;

        Session(final String string) {
            this.string = string;
        }

        @Override
        public String toString() {
            return string;
        }
    }

    





    public enum Reason {
        
        COMMIT("commit"),

        
        NONE(null),
        ;

        private final String string;

        Reason(final String string) {
            this.string = string;
        }

        @Override
        public String toString() {
            return string;
        }
    }
}
