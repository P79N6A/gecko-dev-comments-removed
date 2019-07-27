

























package ch.boye.httpclientandroidlib.impl.client.cache;

import ch.boye.httpclientandroidlib.annotation.ThreadSafe;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;







@ThreadSafe
public class DefaultFailureCache implements FailureCache {

    static final int DEFAULT_MAX_SIZE = 1000;
    static final int MAX_UPDATE_TRIES = 10;

    private final int maxSize;
    private final ConcurrentMap<String, FailureCacheValue> storage;

    



    public DefaultFailureCache() {
        this(DEFAULT_MAX_SIZE);
    }

    



    public DefaultFailureCache(final int maxSize) {
        this.maxSize = maxSize;
        this.storage = new ConcurrentHashMap<String, FailureCacheValue>();
    }

    public int getErrorCount(final String identifier) {
        if (identifier == null) {
            throw new IllegalArgumentException("identifier may not be null");
        }
        final FailureCacheValue storedErrorCode = storage.get(identifier);
        return storedErrorCode != null ? storedErrorCode.getErrorCount() : 0;
    }

    public void resetErrorCount(final String identifier) {
        if (identifier == null) {
            throw new IllegalArgumentException("identifier may not be null");
        }
        storage.remove(identifier);
    }

    public void increaseErrorCount(final String identifier) {
        if (identifier == null) {
            throw new IllegalArgumentException("identifier may not be null");
        }
        updateValue(identifier);
        removeOldestEntryIfMapSizeExceeded();
    }

    private void updateValue(final String identifier) {
        








        for (int i = 0; i < MAX_UPDATE_TRIES; i++) {
            final FailureCacheValue oldValue = storage.get(identifier);
            if (oldValue == null) {
                final FailureCacheValue newValue = new FailureCacheValue(identifier, 1);
                if (storage.putIfAbsent(identifier, newValue) == null) {
                    return;
                }
            }
            else {
                final int errorCount = oldValue.getErrorCount();
                if (errorCount == Integer.MAX_VALUE) {
                    return;
                }
                final FailureCacheValue newValue = new FailureCacheValue(identifier, errorCount + 1);
                if (storage.replace(identifier, oldValue, newValue)) {
                    return;
                }
            }
        }
    }

    private void removeOldestEntryIfMapSizeExceeded() {
        if (storage.size() > maxSize) {
            final FailureCacheValue valueWithOldestTimestamp = findValueWithOldestTimestamp();
            if (valueWithOldestTimestamp != null) {
                storage.remove(valueWithOldestTimestamp.getKey(), valueWithOldestTimestamp);
            }
        }
    }

    private FailureCacheValue findValueWithOldestTimestamp() {
        long oldestTimestamp = Long.MAX_VALUE;
        FailureCacheValue oldestValue = null;
        for (final Map.Entry<String, FailureCacheValue> storageEntry : storage.entrySet()) {
            final FailureCacheValue value = storageEntry.getValue();
            final long creationTimeInNanos = value.getCreationTimeInNanos();
            if (creationTimeInNanos < oldestTimestamp) {
                oldestTimestamp = creationTimeInNanos;
                oldestValue = storageEntry.getValue();
            }
        }
        return oldestValue;
    }
}
