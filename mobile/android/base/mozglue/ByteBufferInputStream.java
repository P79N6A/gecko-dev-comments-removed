




package org.mozilla.gecko.mozglue;

import org.mozilla.gecko.mozglue.NativeReference;

import java.io.InputStream;
import java.nio.ByteBuffer;

class ByteBufferInputStream extends InputStream {

    protected ByteBuffer mBuf;
    
    private NativeReference mNativeRef;

    protected ByteBufferInputStream(ByteBuffer buffer, NativeReference ref) {
        mBuf = buffer;
        mNativeRef = ref;
    }

    @Override
    public int available() {
        return mBuf.remaining();
    }

    @Override
    public void close() {
        mBuf = null;
        mNativeRef.release();
    }

    @Override
    public int read() {
        if (!mBuf.hasRemaining() || mNativeRef.isReleased()) {
            return -1;
        }

        return mBuf.get() & 0xff; 
    }

    @Override
    public int read(byte[] buffer, int offset, int length) {
        if (!mBuf.hasRemaining() || mNativeRef.isReleased()) {
            return -1;
        }

        length = Math.min(length, mBuf.remaining());
        mBuf.get(buffer, offset, length);
        return length;
    }

    @Override
    public long skip(long byteCount) {
        if (byteCount < 0 || mNativeRef.isReleased()) {
            return 0;
        }

        byteCount = Math.min(byteCount, mBuf.remaining());
        mBuf.position(mBuf.position() + (int)byteCount);
        return byteCount;
    }

}
