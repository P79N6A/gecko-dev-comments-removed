



package org.mozilla.search.stream;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;







class PreloadAgent {

    public static final List<TmpItem> ITEMS = new ArrayList<TmpItem>();

    private static final Map<String, TmpItem> ITEM_MAP = new HashMap<String, TmpItem>();

    static {
        addItem(new TmpItem("1", "Pre-load item1"));
        addItem(new TmpItem("2", "Pre-load item2"));
    }

    private static void addItem(TmpItem item) {
        ITEMS.add(item);
        ITEM_MAP.put(item.id, item);
    }

    public static class TmpItem {
        public final String id;
        public final String content;

        public TmpItem(String id, String content) {
            this.id = id;
            this.content = content;
        }

        @Override
        public String toString() {
            return content;
        }
    }
}
