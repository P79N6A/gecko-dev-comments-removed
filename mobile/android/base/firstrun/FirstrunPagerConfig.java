




package org.mozilla.gecko.firstrun;

import java.util.LinkedList;
import java.util.List;

public class FirstrunPagerConfig {
    public static List<FirstrunPanel> getDefault() {
        final List<FirstrunPanel> panels = new LinkedList<>();
        panels.add(new FirstrunPanel(WelcomePanel.class.getName(), "Welcome"));
        return panels;
    }

    public static class FirstrunPanel {
        private String resource;
        private String title;

        public FirstrunPanel(String resource, String title) {
            this.resource = resource;
            this.title = title;
        }

        public String getResource() {
            return this.resource;
        }

        public String getTitle() {
            return this.title;
        }

    }
}
