




package org.mozilla.gecko.mozglue;

import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.zip.Inflater;
import java.util.zip.InflaterInputStream;

public class NativeZip implements NativeReference {
    private static final int DEFLATE = 8;
    private static final int STORE = 0;

    private volatile long mObj;
    private InputStream mInput;

    public NativeZip(String path) {
        mObj = getZip(path);
        mInput = null;
    }

    public NativeZip(InputStream input) {
        if (input instanceof ByteBufferInputStream) {
            ByteBufferInputStream bbinput = (ByteBufferInputStream)input;
            mObj = getZipFromByteBuffer(bbinput.mBuf);
        } else {
            throw new RuntimeException("Only ByteBufferInputStream is supported");
        }
        mInput = input;
    }

    @Override
    public void finalize() {
        release();
    }

    public void close() {
        release();
    }

    @Override
    public void release() {
        if (mObj != 0) {
            _release(mObj);
            mObj = 0;
        }
        mInput = null;
    }

    @Override
    public boolean isReleased() {
        return (mObj == 0);
    }

    public InputStream getInputStream(String path) {
        if (mObj == 0) {
            throw new RuntimeException("NativeZip is closed");
        }
        return _getInputStream(mObj, path);
    }

    private static native long getZip(String path);
    private static native long getZipFromByteBuffer(ByteBuffer buffer);
    private static native void _release(long obj);
    private native InputStream _getInputStream(long obj, String path);

    private InputStream createInputStream(ByteBuffer buffer, int compression) {
        InputStream input = new ByteBufferInputStream(buffer, this);
        if (compression == DEFLATE) {
            Inflater inflater = new Inflater(true);
            input = new InflaterInputStream(input, inflater);
        }
        return input;
    }
}
