

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.io.Closeable;
import java.io.IOException;
import java.lang.ref.ReferenceQueue;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;

import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.client.cache.HttpCacheEntry;
import ch.boye.httpclientandroidlib.client.cache.HttpCacheStorage;
import ch.boye.httpclientandroidlib.client.cache.HttpCacheUpdateCallback;
import ch.boye.httpclientandroidlib.client.cache.Resource;
import ch.boye.httpclientandroidlib.util.Args;
















@ThreadSafe
public class ManagedHttpCacheStorage implements HttpCacheStorage, Closeable {

    private final CacheMap entries;
    private final ReferenceQueue<HttpCacheEntry> morque;
    private final Set<ResourceReference> resources;
    private final AtomicBoolean active;

    public ManagedHttpCacheStorage(final CacheConfig config) {
        super();
        this.entries = new CacheMap(config.getMaxCacheEntries());
        this.morque = new ReferenceQueue<HttpCacheEntry>();
        this.resources = new HashSet<ResourceReference>();
        this.active = new AtomicBoolean(true);
    }

    private void ensureValidState() throws IllegalStateException {
        if (!this.active.get()) {
            throw new IllegalStateException("Cache has been shut down");
        }
    }

    private void keepResourceReference(final HttpCacheEntry entry) {
        final Resource resource = entry.getResource();
        if (resource != null) {
            
            final ResourceReference ref = new ResourceReference(entry, this.morque);
            this.resources.add(ref);
        }
    }

    public void putEntry(final String url, final HttpCacheEntry entry) throws IOException {
        Args.notNull(url, "URL");
        Args.notNull(entry, "Cache entry");
        ensureValidState();
        synchronized (this) {
            this.entries.put(url, entry);
            keepResourceReference(entry);
        }
    }

    public HttpCacheEntry getEntry(final String url) throws IOException {
        Args.notNull(url, "URL");
        ensureValidState();
        synchronized (this) {
            return this.entries.get(url);
        }
    }

    public void removeEntry(final String url) throws IOException {
        Args.notNull(url, "URL");
        ensureValidState();
        synchronized (this) {
            
            
            this.entries.remove(url);
        }
    }

    public void updateEntry(
            final String url,
            final HttpCacheUpdateCallback callback) throws IOException {
        Args.notNull(url, "URL");
        Args.notNull(callback, "Callback");
        ensureValidState();
        synchronized (this) {
            final HttpCacheEntry existing = this.entries.get(url);
            final HttpCacheEntry updated = callback.update(existing);
            this.entries.put(url, updated);
            if (existing != updated) {
                keepResourceReference(updated);
            }
        }
    }

    public void cleanResources() {
        if (this.active.get()) {
            ResourceReference ref;
            while ((ref = (ResourceReference) this.morque.poll()) != null) {
                synchronized (this) {
                    this.resources.remove(ref);
                }
                ref.getResource().dispose();
            }
        }
    }

    public void shutdown() {
        if (this.active.compareAndSet(true, false)) {
            synchronized (this) {
                this.entries.clear();
                for (final ResourceReference ref: this.resources) {
                    ref.getResource().dispose();
                }
                this.resources.clear();
                while (this.morque.poll() != null) {
                }
            }
        }
    }

    public void close() {
        shutdown();
    }

}
