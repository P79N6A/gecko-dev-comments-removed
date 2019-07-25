



package org.mozilla.gecko;

import java.util.HashMap;
import java.util.Map;

class ActivityResultHandlerMap {
    private Map<Integer, ActivityResultHandler> mMap = new HashMap<Integer, ActivityResultHandler>();
    private int mCounter = 0;

    synchronized int put(ActivityResultHandler handler) {
        mMap.put(mCounter, handler);
        return mCounter++;
    }

    synchronized ActivityResultHandler getAndRemove(int i) {
        return mMap.remove(i);
    }
}
