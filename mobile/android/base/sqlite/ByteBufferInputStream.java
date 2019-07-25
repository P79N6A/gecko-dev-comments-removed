




































package org.mozilla.gecko.sqlite;

import java.io.BufferedInputStream;
import java.io.InputStream;
import java.io.IOException;
import java.nio.ByteBuffer;





public class ByteBufferInputStream extends InputStream {
    private ByteBuffer mByteBuffer;

    public ByteBufferInputStream(ByteBuffer aByteBuffer) {
        mByteBuffer = aByteBuffer;
    }

    @Override
    public synchronized int read() throws IOException {
        if (!mByteBuffer.hasRemaining()) {
            return -1;
        }
        return mByteBuffer.get();
    }

    @Override
    public synchronized int read(byte[] aBytes, int aOffset, int aLen)
        throws IOException {
        int toRead = Math.min(aLen, mByteBuffer.remaining());
        mByteBuffer.get(aBytes, aOffset, toRead);
        return toRead;
    }
}
