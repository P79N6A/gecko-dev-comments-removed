




package org.mozilla.gecko;

import org.mozilla.gecko.mozglue.RobocopTarget;









@RobocopTarget
public interface TelemetryContract {

    





    public enum Event {
        
        ACTION("action.1"),

        
        CANCEL("cancel.1"),

        
        
        CAST("cast.1"),

        
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

        
        SEARCH("search.1"),

        
        SEARCH_REMOVE("search.remove.1"),

        
        SEARCH_RESTORE_DEFAULTS("search.restoredefaults.1"),

        
        SEARCH_SET_DEFAULT("search.setdefault.1"),

        
        SHARE("share.1"),

        
        
        UNDO("undo.1"),

        
        UNPIN("unpin.1"),

        
        UNSAVE("unsave.1"),

        
        _TEST1("_test_event_1.1"),
        _TEST2("_test_event_2.1"),
        _TEST3("_test_event_3.1"),
        _TEST4("_test_event_4.1"),
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

        
        CONTENT("content"),

        
        CONTEXT_MENU("contextmenu"),

        
        DIALOG("dialog"),

        
        GRID_ITEM("griditem"),

        
        INTENT("intent"),

        
        HOMESCREEN("homescreen"),

        
        LIST("list"),

        
        LIST_ITEM("listitem"),

        
        MENU("menu"),

        
        NONE(null),

        
        
        PAGEACTION("pageaction"),

        
        SETTINGS("settings"),

        
        SUGGESTION("suggestion"),

        
        
        TOAST("toast"),

        
        WIDGET("widget"),

        
        _TEST1("_test_method_1"),
        _TEST2("_test_method_2"),
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

        
        
        HOME_PANEL("homepanel.1"),

        
        
        READER("reader.1"),

        
        SEARCH_ACTIVITY("searchactivity.1"),

        
        SETTINGS("settings.1"),

        
        _TEST_STARTED_TWICE("_test_session_started_twice.1"),
        _TEST_STOPPED_TWICE("_test_session_stopped_twice.1"),
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

        
        _TEST1("_test_reason_1"),
        _TEST2("_test_reason_2"),
        _TEST_IGNORED("_test_reason_ignored"),
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
