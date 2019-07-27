

























package ch.boye.httpclientandroidlib.impl.client;

import java.util.concurrent.Callable;
import java.util.concurrent.atomic.AtomicBoolean;

import ch.boye.httpclientandroidlib.client.HttpClient;
import ch.boye.httpclientandroidlib.client.ResponseHandler;
import ch.boye.httpclientandroidlib.client.methods.HttpUriRequest;
import ch.boye.httpclientandroidlib.concurrent.FutureCallback;
import ch.boye.httpclientandroidlib.protocol.HttpContext;

class HttpRequestTaskCallable<V> implements Callable<V> {

    private final HttpUriRequest request;
    private final HttpClient httpclient;
    private final AtomicBoolean cancelled = new AtomicBoolean(false);

    private final long scheduled = System.currentTimeMillis();
    private long started = -1;
    private long ended = -1;

    private final HttpContext context;
    private final ResponseHandler<V> responseHandler;
    private final FutureCallback<V> callback;

    private final FutureRequestExecutionMetrics metrics;

    HttpRequestTaskCallable(
            final HttpClient httpClient,
            final HttpUriRequest request,
            final HttpContext context,
            final ResponseHandler<V> responseHandler,
            final FutureCallback<V> callback,
            final FutureRequestExecutionMetrics metrics) {
        this.httpclient = httpClient;
        this.responseHandler = responseHandler;
        this.request = request;
        this.context = context;
        this.callback = callback;
        this.metrics = metrics;
    }

    public long getScheduled() {
        return scheduled;
    }

    public long getStarted() {
        return started;
    }

    public long getEnded() {
        return ended;
    }

    public V call() throws Exception {
        if (!cancelled.get()) {
            try {
                metrics.getActiveConnections().incrementAndGet();
                started = System.currentTimeMillis();
                try {
                    metrics.getScheduledConnections().decrementAndGet();
                    final V result = httpclient.execute(request, responseHandler, context);
                    ended = System.currentTimeMillis();
                    metrics.getSuccessfulConnections().increment(started);
                    if (callback != null) {
                        callback.completed(result);
                    }
                    return result;
                } catch (final Exception e) {
                    metrics.getFailedConnections().increment(started);
                    ended = System.currentTimeMillis();
                    if (callback != null) {
                        callback.failed(e);
                    }
                    throw e;
                }
            } finally {
                metrics.getRequests().increment(started);
                metrics.getTasks().increment(started);
                metrics.getActiveConnections().decrementAndGet();
            }
        } else {
            throw new IllegalStateException("call has been cancelled for request " + request.getURI());
        }
    }

    public void cancel() {
        cancelled.set(true);
        if (callback != null) {
            callback.cancelled();
        }
    }
}
