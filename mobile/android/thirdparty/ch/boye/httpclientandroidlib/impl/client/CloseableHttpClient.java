


























package ch.boye.httpclientandroidlib.impl.client;

import java.io.Closeable;
import java.io.IOException;
import java.lang.reflect.UndeclaredThrowableException;
import java.net.URI;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.client.ClientProtocolException;
import ch.boye.httpclientandroidlib.client.HttpClient;
import ch.boye.httpclientandroidlib.client.ResponseHandler;
import ch.boye.httpclientandroidlib.client.methods.CloseableHttpResponse;
import ch.boye.httpclientandroidlib.client.methods.HttpUriRequest;
import ch.boye.httpclientandroidlib.client.utils.URIUtils;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.EntityUtils;






@ThreadSafe
public abstract class CloseableHttpClient implements HttpClient, Closeable {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    protected abstract CloseableHttpResponse doExecute(HttpHost target, HttpRequest request,
            HttpContext context) throws IOException, ClientProtocolException;

    


    public CloseableHttpResponse execute(
            final HttpHost target,
            final HttpRequest request,
            final HttpContext context) throws IOException, ClientProtocolException {
        return doExecute(target, request, context);
    }

    


    public CloseableHttpResponse execute(
            final HttpUriRequest request,
            final HttpContext context) throws IOException, ClientProtocolException {
        Args.notNull(request, "HTTP request");
        return doExecute(determineTarget(request), request, context);
    }

    private static HttpHost determineTarget(final HttpUriRequest request) throws ClientProtocolException {
        
        
        HttpHost target = null;

        final URI requestURI = request.getURI();
        if (requestURI.isAbsolute()) {
            target = URIUtils.extractHost(requestURI);
            if (target == null) {
                throw new ClientProtocolException("URI does not specify a valid host name: "
                        + requestURI);
            }
        }
        return target;
    }

    


    public CloseableHttpResponse execute(
            final HttpUriRequest request) throws IOException, ClientProtocolException {
        return execute(request, (HttpContext) null);
    }

    


    public CloseableHttpResponse execute(
            final HttpHost target,
            final HttpRequest request) throws IOException, ClientProtocolException {
        return doExecute(target, request, (HttpContext) null);
    }

    














    public <T> T execute(final HttpUriRequest request,
            final ResponseHandler<? extends T> responseHandler) throws IOException,
            ClientProtocolException {
        return execute(request, responseHandler, null);
    }

    
















    public <T> T execute(final HttpUriRequest request,
            final ResponseHandler<? extends T> responseHandler, final HttpContext context)
            throws IOException, ClientProtocolException {
        final HttpHost target = determineTarget(request);
        return execute(target, request, responseHandler, context);
    }

    


















    public <T> T execute(final HttpHost target, final HttpRequest request,
            final ResponseHandler<? extends T> responseHandler) throws IOException,
            ClientProtocolException {
        return execute(target, request, responseHandler, null);
    }

    




















    public <T> T execute(final HttpHost target, final HttpRequest request,
            final ResponseHandler<? extends T> responseHandler, final HttpContext context)
            throws IOException, ClientProtocolException {
        Args.notNull(responseHandler, "Response handler");

        final HttpResponse response = execute(target, request, context);

        final T result;
        try {
            result = responseHandler.handleResponse(response);
        } catch (final Exception t) {
            final HttpEntity entity = response.getEntity();
            try {
                EntityUtils.consume(entity);
            } catch (final Exception t2) {
                
                
                this.log.warn("Error consuming content after an exception.", t2);
            }
            if (t instanceof RuntimeException) {
                throw (RuntimeException) t;
            }
            if (t instanceof IOException) {
                throw (IOException) t;
            }
            throw new UndeclaredThrowableException(t);
        }

        
        
        final HttpEntity entity = response.getEntity();
        EntityUtils.consume(entity);
        return result;
    }

}
