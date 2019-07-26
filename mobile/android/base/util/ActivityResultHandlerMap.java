



package org.mozilla.gecko.util;

import java.util.HashMap;
import java.util.Map;

public final class ActivityResultHandlerMap {
    private Map<Integer, ActivityResultHandler> mMap = new HashMap<Integer, ActivityResultHandler>();
    private int mCounter = 0;

    public synchronized int put(ActivityResultHandler handler) {
        mMap.put(mCounter, handler);
        return mCounter++;
    }

    public synchronized ActivityResultHandler getAndRemove(int i) {
        return mMap.remove(i);
    }
}
