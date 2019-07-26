




package org.mozilla.gecko.util;

import android.support.v4.util.LruCache;

import java.util.concurrent.ConcurrentHashMap;






public class NonEvictingLruCache<K, V> {
    private final ConcurrentHashMap<K, V> permanent = new ConcurrentHashMap<K, V>();
    private final LruCache<K, V> evictable;

    public NonEvictingLruCache(final int evictableSize) {
        evictable = new LruCache<K, V>(evictableSize);
    }

    public V get(K key) {
        V val = permanent.get(key);
        if (val == null) {
            return evictable.get(key);
        }
        return val;
    }

    public void putWithoutEviction(K key, V value) {
        permanent.put(key, value);
    }

    public void put(K key, V value) {
        evictable.put(key, value);
    }

    public void evictAll() {
        evictable.evictAll();
    }
}
