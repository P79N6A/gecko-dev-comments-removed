




package org.mozilla.gecko.mozglue;

import java.nio.ByteBuffer;







public final class DirectBufferAllocator {
    private DirectBufferAllocator() {}

    public static ByteBuffer allocate(int size) {
        if (size <= 0) {
            throw new IllegalArgumentException("Invalid size " + size);
        }

        ByteBuffer directBuffer = nativeAllocateDirectBuffer(size);
        if (directBuffer == null) {
            throw new OutOfMemoryError("allocateDirectBuffer() returned null");
        } else if (!directBuffer.isDirect()) {
            throw new AssertionError("allocateDirectBuffer() did not return a direct buffer");
        }

        return directBuffer;
    }

    public static ByteBuffer free(ByteBuffer buffer) {
        if (buffer == null) {
            return null;
        }

        if (!buffer.isDirect()) {
            throw new IllegalArgumentException("buffer must be direct");
        }

        nativeFreeDirectBuffer(buffer);
        return null;
    }

    
    private static native ByteBuffer nativeAllocateDirectBuffer(long size);
    private static native void nativeFreeDirectBuffer(ByteBuffer buf);
}
