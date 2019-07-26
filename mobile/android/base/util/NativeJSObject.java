




package org.mozilla.gecko.util;

import org.mozilla.gecko.mozglue.JNITarget;





@JNITarget
public class NativeJSObject
{
    private final NativeJSContainer mContainer;

    protected NativeJSObject() {
        mContainer = (NativeJSContainer)this;
    }

    









    @Override
    public native String toString();
}
