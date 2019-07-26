




package org.mozilla.gecko.util;

import org.mozilla.gecko.mozglue.JNITarget;

import android.os.Bundle;





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

    















    public native boolean optBoolean(String name, boolean fallback);

    













    public native Bundle getBundle(String name);

    















    public native Bundle optBundle(String name, Bundle fallback);

    













    public native double getDouble(String name);

    















    public native double optDouble(String name, double fallback);

    













    public native int getInt(String name);

    















    public native int optInt(String name, int fallback);

    













    public native NativeJSObject getObject(String name);

    















    public native NativeJSObject optObject(String name, NativeJSObject fallback);

    













    public native String getString(String name);

    















    public native String optString(String name, String fallback);

    











    public native boolean has(String name);

    









    public native Bundle toBundle();

    









    @Override
    public native String toString();
}
