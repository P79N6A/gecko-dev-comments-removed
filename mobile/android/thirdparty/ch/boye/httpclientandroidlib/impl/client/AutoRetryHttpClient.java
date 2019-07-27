


























package ch.boye.httpclientandroidlib.impl.client;

import java.io.IOException;
import java.io.InterruptedIOException;
import java.net.URI;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.client.HttpClient;
import ch.boye.httpclientandroidlib.client.ResponseHandler;
import ch.boye.httpclientandroidlib.client.ServiceUnavailableRetryStrategy;
import ch.boye.httpclientandroidlib.client.methods.HttpUriRequest;
import ch.boye.httpclientandroidlib.conn.ClientConnectionManager;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.EntityUtils;









@Deprecated
@ThreadSafe
public class AutoRetryHttpClient implements HttpClient {

    private final HttpClient backend;

    private final ServiceUnavailableRetryStrategy retryStrategy;

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    public AutoRetryHttpClient(
            final HttpClient client, final ServiceUnavailableRetryStrategy retryStrategy) {
        super();
        Args.notNull(client, "HttpClient");
        Args.notNull(retryStrategy, "ServiceUnavailableRetryStrategy");
        this.backend = client;
        this.retryStrategy = retryStrategy;
    }

    




    public AutoRetryHttpClient() {
        this(new DefaultHttpClient(), new DefaultServiceUnavailableRetryStrategy());
    }

    







    public AutoRetryHttpClient(final ServiceUnavailableRetryStrategy config) {
        this(new DefaultHttpClient(), config);
    }

    







    public AutoRetryHttpClient(final HttpClient client) {
        this(client, new DefaultServiceUnavailableRetryStrategy());
    }

    public HttpResponse execute(final HttpHost target, final HttpRequest request)
            throws IOException {
        final HttpContext defaultContext = null;
        return execute(target, request, defaultContext);
    }

    public <T> T execute(final HttpHost target, final HttpRequest request,
            final ResponseHandler<? extends T> responseHandler) throws IOException {
        return execute(target, request, responseHandler, null);
    }

    public <T> T execute(final HttpHost target, final HttpRequest request,
            final ResponseHandler<? extends T> responseHandler, final HttpContext context)
            throws IOException {
        final HttpResponse resp = execute(target, request, context);
        return responseHandler.handleResponse(resp);
    }

    public HttpResponse execute(final HttpUriRequest request) throws IOException {
        final HttpContext context = null;
        return execute(request, context);
    }

    public HttpResponse execute(final HttpUriRequest request, final HttpContext context)
            throws IOException {
        final URI uri = request.getURI();
        final HttpHost httpHost = new HttpHost(uri.getHost(), uri.getPort(),
                uri.getScheme());
        return execute(httpHost, request, context);
    }

    public <T> T execute(final HttpUriRequest request,
            final ResponseHandler<? extends T> responseHandler) throws IOException {
        return execute(request, responseHandler, null);
    }

    public <T> T execute(final HttpUriRequest request,
            final ResponseHandler<? extends T> responseHandler, final HttpContext context)
            throws IOException {
        final HttpResponse resp = execute(request, context);
        return responseHandler.handleResponse(resp);
    }

    public HttpResponse execute(final HttpHost target, final HttpRequest request,
            final HttpContext context) throws IOException {
        for (int c = 1;; c++) {
            final HttpResponse response = backend.execute(target, request, context);
            try {
                if (retryStrategy.retryRequest(response, c, context)) {
                    EntityUtils.consume(response.getEntity());
                    final long nextInterval = retryStrategy.getRetryInterval();
                    try {
                        log.trace("Wait for " + nextInterval);
                        Thread.sleep(nextInterval);
                    } catch (final InterruptedException e) {
                        Thread.currentThread().interrupt();
                        throw new InterruptedIOException();
                    }
                } else {
                    return response;
                }
            } catch (final RuntimeException ex) {
                try {
                    EntityUtils.consume(response.getEntity());
                } catch (final IOException ioex) {
                    log.warn("I/O error consuming response content", ioex);
                }
                throw ex;
            }
        }
    }

    public ClientConnectionManager getConnectionManager() {
        return backend.getConnectionManager();
    }

    public HttpParams getParams() {
        return backend.getParams();
    }

}
