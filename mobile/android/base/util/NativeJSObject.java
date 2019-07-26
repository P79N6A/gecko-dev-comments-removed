




package org.mozilla.gecko.util;

import org.mozilla.gecko.mozglue.JNITarget;





@JNITarget
public class NativeJSObject
{
    private final NativeJSContainer mContainer;
    private final int mObjectIndex;

    protected NativeJSObject() {
        mContainer = (NativeJSContainer)this;
        mObjectIndex = -1;
    }

    private NativeJSObject(NativeJSContainer container, int index) {
        mContainer = container;
        mObjectIndex = index;
    }

    













    public native boolean getBoolean(String name);

    













    public native double getDouble(String name);

    













    public native int getInt(String name);

    













    public native NativeJSObject getObject(String name);

    













    public native String getString(String name);

    











    public native boolean has(String name);

    









    @Override
    public native String toString();
}
