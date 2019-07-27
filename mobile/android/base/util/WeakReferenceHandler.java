



package org.mozilla.gecko.util;

import android.os.Handler;

import java.lang.ref.WeakReference;










public class WeakReferenceHandler<T> extends Handler {
    public final WeakReference<T> mTarget;

    public WeakReferenceHandler(final T that) {
        super();
        mTarget = new WeakReference<>(that);
    }
}
