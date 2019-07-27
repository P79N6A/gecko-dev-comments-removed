

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.io.Closeable;
import java.io.IOException;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.RejectedExecutionException;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.client.cache.HttpCacheEntry;
import ch.boye.httpclientandroidlib.client.methods.HttpExecutionAware;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestWrapper;
import ch.boye.httpclientandroidlib.client.protocol.HttpClientContext;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;





class AsynchronousValidator implements Closeable {
    private final SchedulingStrategy schedulingStrategy;
    private final Set<String> queued;
    private final CacheKeyGenerator cacheKeyGenerator;
    private final FailureCache failureCache;

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    









    public AsynchronousValidator(final CacheConfig config) {
        this(new ImmediateSchedulingStrategy(config));
    }

    






    AsynchronousValidator(final SchedulingStrategy schedulingStrategy) {
        this.schedulingStrategy = schedulingStrategy;
        this.queued = new HashSet<String>();
        this.cacheKeyGenerator = new CacheKeyGenerator();
        this.failureCache = new DefaultFailureCache();
    }

    public void close() throws IOException {
        schedulingStrategy.close();
    }

    


    public synchronized void revalidateCacheEntry(
            final CachingExec cachingExec,
            final HttpRoute route,
            final HttpRequestWrapper request,
            final HttpClientContext context,
            final HttpExecutionAware execAware,
            final HttpCacheEntry entry) {
        
        final String uri = cacheKeyGenerator.getVariantURI(context.getTargetHost(), request, entry);

        if (!queued.contains(uri)) {
            final int consecutiveFailedAttempts = failureCache.getErrorCount(uri);
            final AsynchronousValidationRequest revalidationRequest =
                new AsynchronousValidationRequest(
                        this, cachingExec, route, request, context, execAware, entry, uri, consecutiveFailedAttempts);

            try {
                schedulingStrategy.schedule(revalidationRequest);
                queued.add(uri);
            } catch (final RejectedExecutionException ree) {
                log.debug("Revalidation for [" + uri + "] not scheduled: " + ree);
            }
        }
    }

    






    synchronized void markComplete(final String identifier) {
        queued.remove(identifier);
    }

    





    void jobSuccessful(final String identifier) {
        failureCache.resetErrorCount(identifier);
    }

    





    void jobFailed(final String identifier) {
        failureCache.increaseErrorCount(identifier);
    }

    Set<String> getScheduledIdentifiers() {
        return Collections.unmodifiableSet(queued);
    }
}
