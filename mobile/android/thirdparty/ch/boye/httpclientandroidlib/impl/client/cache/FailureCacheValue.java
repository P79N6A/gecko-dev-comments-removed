

























package ch.boye.httpclientandroidlib.impl.client.cache;

import ch.boye.httpclientandroidlib.annotation.Immutable;






@Immutable
public class FailureCacheValue {

    private final long creationTimeInNanos;
    private final String key;
    private final int errorCount;

    public FailureCacheValue(final String key, final int errorCount) {
        this.creationTimeInNanos = System.nanoTime();
        this.key = key;
        this.errorCount = errorCount;
    }

    public long getCreationTimeInNanos() {
        return creationTimeInNanos;
    }

    public String getKey()
    {
        return key;
    }

    public int getErrorCount() {
        return errorCount;
    }

    @Override
    public String toString() {
        return "[entry creationTimeInNanos=" + creationTimeInNanos + "; " +
                "key=" + key + "; errorCount=" + errorCount + ']';
    }
}
