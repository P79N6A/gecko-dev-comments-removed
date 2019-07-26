




package org.mozilla.gecko.util;

import org.mozilla.gecko.mozglue.JNITarget;









@JNITarget
public final class NativeJSContainer extends NativeJSObject
{
    private long mNativeObject;

    private NativeJSContainer(long nativeObject) {
        mNativeObject = nativeObject;
    }

    



    @Override
    public native NativeJSContainer clone();

    



    public native void dispose();
}
