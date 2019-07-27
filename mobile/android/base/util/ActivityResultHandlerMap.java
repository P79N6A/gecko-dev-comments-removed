



package org.mozilla.gecko.util;

import android.util.SparseArray;

public final class ActivityResultHandlerMap {
    private SparseArray<ActivityResultHandler> mMap = new SparseArray<ActivityResultHandler>();
    private int mCounter;

    public synchronized int put(ActivityResultHandler handler) {
        mMap.put(mCounter, handler);
        return mCounter++;
    }

    public synchronized ActivityResultHandler getAndRemove(int i) {
        ActivityResultHandler handler = mMap.get(i);
        mMap.delete(i);

        return handler;
    }
}
