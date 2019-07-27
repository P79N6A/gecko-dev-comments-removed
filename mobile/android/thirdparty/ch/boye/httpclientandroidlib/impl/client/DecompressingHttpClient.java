

























package ch.boye.httpclientandroidlib.impl.client;

import java.io.IOException;
import java.net.URI;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpEntityEnclosingRequest;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpRequestInterceptor;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpResponseInterceptor;
import ch.boye.httpclientandroidlib.client.ClientProtocolException;
import ch.boye.httpclientandroidlib.client.HttpClient;
import ch.boye.httpclientandroidlib.client.ResponseHandler;
import ch.boye.httpclientandroidlib.client.methods.HttpUriRequest;
import ch.boye.httpclientandroidlib.client.protocol.RequestAcceptEncoding;
import ch.boye.httpclientandroidlib.client.protocol.ResponseContentEncoding;
import ch.boye.httpclientandroidlib.client.utils.URIUtils;
import ch.boye.httpclientandroidlib.conn.ClientConnectionManager;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.BasicHttpContext;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.EntityUtils;

























@Deprecated
public class DecompressingHttpClient implements HttpClient {

    private final HttpClient backend;
    private final HttpRequestInterceptor acceptEncodingInterceptor;
    private final HttpResponseInterceptor contentEncodingInterceptor;

    



    public DecompressingHttpClient() {
        this(new DefaultHttpClient());
    }

    





    public DecompressingHttpClient(final HttpClient backend) {
        this(backend, new RequestAcceptEncoding(), new ResponseContentEncoding());
    }

    DecompressingHttpClient(final HttpClient backend,
            final HttpRequestInterceptor requestInterceptor,
            final HttpResponseInterceptor responseInterceptor) {
        this.backend = backend;
        this.acceptEncodingInterceptor = requestInterceptor;
        this.contentEncodingInterceptor = responseInterceptor;
    }

    public HttpParams getParams() {
        return backend.getParams();
    }

    public ClientConnectionManager getConnectionManager() {
        return backend.getConnectionManager();
    }

    public HttpResponse execute(final HttpUriRequest request) throws IOException,
            ClientProtocolException {
        return execute(getHttpHost(request), request, (HttpContext)null);
    }

    




    public HttpClient getHttpClient() {
        return this.backend;
    }

    HttpHost getHttpHost(final HttpUriRequest request) {
        final URI uri = request.getURI();
        return URIUtils.extractHost(uri);
    }

    public HttpResponse execute(final HttpUriRequest request, final HttpContext context)
            throws IOException, ClientProtocolException {
        return execute(getHttpHost(request), request, context);
    }

    public HttpResponse execute(final HttpHost target, final HttpRequest request)
            throws IOException, ClientProtocolException {
        return execute(target, request, (HttpContext)null);
    }

    public HttpResponse execute(final HttpHost target, final HttpRequest request,
            final HttpContext context) throws IOException, ClientProtocolException {
        try {
            final HttpContext localContext = context != null ? context : new BasicHttpContext();
            final HttpRequest wrapped;
            if (request instanceof HttpEntityEnclosingRequest) {
                wrapped = new EntityEnclosingRequestWrapper((HttpEntityEnclosingRequest) request);
            } else {
                wrapped = new RequestWrapper(request);
            }
            acceptEncodingInterceptor.process(wrapped, localContext);
            final HttpResponse response = backend.execute(target, wrapped, localContext);
            try {
                contentEncodingInterceptor.process(response, localContext);
                if (Boolean.TRUE.equals(localContext.getAttribute(ResponseContentEncoding.UNCOMPRESSED))) {
                    response.removeHeaders("Content-Length");
                    response.removeHeaders("Content-Encoding");
                    response.removeHeaders("Content-MD5");
                }
                return response;
            } catch (final HttpException ex) {
                EntityUtils.consume(response.getEntity());
                throw ex;
            } catch (final IOException ex) {
                EntityUtils.consume(response.getEntity());
                throw ex;
            } catch (final RuntimeException ex) {
                EntityUtils.consume(response.getEntity());
                throw ex;
            }
        } catch (final HttpException e) {
            throw new ClientProtocolException(e);
        }
    }

    public <T> T execute(final HttpUriRequest request,
            final ResponseHandler<? extends T> responseHandler) throws IOException,
            ClientProtocolException {
        return execute(getHttpHost(request), request, responseHandler);
    }

    public <T> T execute(final HttpUriRequest request,
            final ResponseHandler<? extends T> responseHandler, final HttpContext context)
            throws IOException, ClientProtocolException {
        return execute(getHttpHost(request), request, responseHandler, context);
    }

    public <T> T execute(final HttpHost target, final HttpRequest request,
            final ResponseHandler<? extends T> responseHandler) throws IOException,
            ClientProtocolException {
        return execute(target, request, responseHandler, null);
    }

    public <T> T execute(final HttpHost target, final HttpRequest request,
            final ResponseHandler<? extends T> responseHandler, final HttpContext context)
            throws IOException, ClientProtocolException {
        final HttpResponse response = execute(target, request, context);
        try {
            return responseHandler.handleResponse(response);
        } finally {
            final HttpEntity entity = response.getEntity();
            if (entity != null) {
                EntityUtils.consume(entity);
            }
        }
    }

}
