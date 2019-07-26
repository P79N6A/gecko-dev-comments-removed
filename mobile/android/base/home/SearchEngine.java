




package org.mozilla.gecko.home;

import android.graphics.Bitmap;

import java.util.ArrayList;

class SearchEngine {
    public String name;
    public String identifier;
    public Bitmap icon;
    public ArrayList<String> suggestions;

    public SearchEngine(String name, String identifier) {
        this(name, identifier, null);
    }

    public SearchEngine(String name, String identifier, Bitmap icon) {
        this.name = name;
        this.identifier = identifier;
        this.icon = icon;
        this.suggestions = new ArrayList<String>();
    }
}

