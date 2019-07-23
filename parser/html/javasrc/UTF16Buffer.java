





















package nu.validator.htmlparser.impl;

import nu.validator.htmlparser.annotation.NoLength;

public final class UTF16Buffer {
    private final @NoLength char[] buffer;

    private int start;

    private int end;

    




    public UTF16Buffer(@NoLength char[] buffer, int start, int end) {
        this.buffer = buffer;
        this.start = start;
        this.end = end;
    }

    




    public int getStart() {
        return start;
    }

    




    public void setStart(int start) {
        this.start = start;
    }

    




    public @NoLength char[] getBuffer() {
        return buffer;
    }

    




    public int getEnd() {
        return end;
    }
    
    public boolean hasMore() {
        return start < end;
    }
    
    public void adjust(boolean lastWasCR) {
        if (lastWasCR && buffer[start] == '\n') {
            start++;
        }
    }

    




    public void setEnd(int end) {
        this.end = end;
    }
}
