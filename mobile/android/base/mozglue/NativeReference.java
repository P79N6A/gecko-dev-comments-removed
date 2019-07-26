




package org.mozilla.gecko.mozglue;

public interface NativeReference
{
    public void release();

    public boolean isReleased();
}
