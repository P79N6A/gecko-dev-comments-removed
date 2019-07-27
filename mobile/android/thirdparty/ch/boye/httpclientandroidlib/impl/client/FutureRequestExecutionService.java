

























package ch.boye.httpclientandroidlib.impl.client;

import java.io.Closeable;
import java.io.IOException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.atomic.AtomicBoolean;

import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.client.HttpClient;
import ch.boye.httpclientandroidlib.client.ResponseHandler;
import ch.boye.httpclientandroidlib.client.methods.HttpUriRequest;
import ch.boye.httpclientandroidlib.concurrent.FutureCallback;
import ch.boye.httpclientandroidlib.protocol.HttpContext;





@ThreadSafe
public class FutureRequestExecutionService implements Closeable {

    private final HttpClient httpclient;
    private final ExecutorService executorService;
    private final FutureRequestExecutionMetrics metrics = new FutureRequestExecutionMetrics();
    private final AtomicBoolean closed = new AtomicBoolean(false);

    












    public FutureRequestExecutionService(
            final HttpClient httpclient,
            final ExecutorService executorService) {
        this.httpclient = httpclient;
        this.executorService = executorService;
    }

    











    public <T> HttpRequestFutureTask<T> execute(
            final HttpUriRequest request,
            final HttpContext context,
            final ResponseHandler<T> responseHandler) {
        return execute(request, context, responseHandler, null);
    }

    
















    public <T> HttpRequestFutureTask<T> execute(
            final HttpUriRequest request,
            final HttpContext context,
            final ResponseHandler<T> responseHandler,
            final FutureCallback<T> callback) {
        if(closed.get()) {
            throw new IllegalStateException("Close has been called on this httpclient instance.");
        }
        metrics.getScheduledConnections().incrementAndGet();
        final HttpRequestTaskCallable<T> callable = new HttpRequestTaskCallable<T>(
            httpclient, request, context, responseHandler, callback, metrics);
        final HttpRequestFutureTask<T> httpRequestFutureTask = new HttpRequestFutureTask<T>(
            request, callable);
        executorService.execute(httpRequestFutureTask);

        return httpRequestFutureTask;
    }

    



    public FutureRequestExecutionMetrics metrics() {
        return metrics;
    }

    public void close() throws IOException {
        closed.set(true);
        executorService.shutdownNow();
        if (httpclient instanceof Closeable) {
            ((Closeable) httpclient).close();
        }
    }
}
