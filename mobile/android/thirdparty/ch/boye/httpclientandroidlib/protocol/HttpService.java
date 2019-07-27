


























package ch.boye.httpclientandroidlib.protocol;

import java.io.IOException;

import ch.boye.httpclientandroidlib.ConnectionReuseStrategy;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpEntityEnclosingRequest;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpResponseFactory;
import ch.boye.httpclientandroidlib.HttpServerConnection;
import ch.boye.httpclientandroidlib.HttpStatus;
import ch.boye.httpclientandroidlib.HttpVersion;
import ch.boye.httpclientandroidlib.MethodNotSupportedException;
import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.UnsupportedHttpVersionException;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.entity.ByteArrayEntity;
import ch.boye.httpclientandroidlib.impl.DefaultConnectionReuseStrategy;
import ch.boye.httpclientandroidlib.impl.DefaultHttpResponseFactory;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.EncodingUtils;
import ch.boye.httpclientandroidlib.util.EntityUtils;




















@SuppressWarnings("deprecation")
@Immutable 
public class HttpService {

    


    private volatile HttpParams params = null;
    private volatile HttpProcessor processor = null;
    private volatile HttpRequestHandlerMapper handlerMapper = null;
    private volatile ConnectionReuseStrategy connStrategy = null;
    private volatile HttpResponseFactory responseFactory = null;
    private volatile HttpExpectationVerifier expectationVerifier = null;

    













    @Deprecated
    public HttpService(
            final HttpProcessor processor,
            final ConnectionReuseStrategy connStrategy,
            final HttpResponseFactory responseFactory,
            final HttpRequestHandlerResolver handlerResolver,
            final HttpExpectationVerifier expectationVerifier,
            final HttpParams params) {
        this(processor,
             connStrategy,
             responseFactory,
             new HttpRequestHandlerResolverAdapter(handlerResolver),
             expectationVerifier);
        this.params = params;
    }

    












    @Deprecated
    public HttpService(
            final HttpProcessor processor,
            final ConnectionReuseStrategy connStrategy,
            final HttpResponseFactory responseFactory,
            final HttpRequestHandlerResolver handlerResolver,
            final HttpParams params) {
        this(processor,
             connStrategy,
             responseFactory,
             new HttpRequestHandlerResolverAdapter(handlerResolver),
             null);
        this.params = params;
    }

    









    @Deprecated
    public HttpService(
            final HttpProcessor proc,
            final ConnectionReuseStrategy connStrategy,
            final HttpResponseFactory responseFactory) {
        super();
        setHttpProcessor(proc);
        setConnReuseStrategy(connStrategy);
        setResponseFactory(responseFactory);
    }

    












    public HttpService(
            final HttpProcessor processor,
            final ConnectionReuseStrategy connStrategy,
            final HttpResponseFactory responseFactory,
            final HttpRequestHandlerMapper handlerMapper,
            final HttpExpectationVerifier expectationVerifier) {
        super();
        this.processor =  Args.notNull(processor, "HTTP processor");
        this.connStrategy = connStrategy != null ? connStrategy :
            DefaultConnectionReuseStrategy.INSTANCE;
        this.responseFactory = responseFactory != null ? responseFactory :
            DefaultHttpResponseFactory.INSTANCE;
        this.handlerMapper = handlerMapper;
        this.expectationVerifier = expectationVerifier;
    }

    











    public HttpService(
            final HttpProcessor processor,
            final ConnectionReuseStrategy connStrategy,
            final HttpResponseFactory responseFactory,
            final HttpRequestHandlerMapper handlerMapper) {
        this(processor, connStrategy, responseFactory, handlerMapper, null);
    }

    







    public HttpService(
            final HttpProcessor processor, final HttpRequestHandlerMapper handlerMapper) {
        this(processor, null, null, handlerMapper, null);
    }

    


    @Deprecated
    public void setHttpProcessor(final HttpProcessor processor) {
        Args.notNull(processor, "HTTP processor");
        this.processor = processor;
    }

    


    @Deprecated
    public void setConnReuseStrategy(final ConnectionReuseStrategy connStrategy) {
        Args.notNull(connStrategy, "Connection reuse strategy");
        this.connStrategy = connStrategy;
    }

    


    @Deprecated
    public void setResponseFactory(final HttpResponseFactory responseFactory) {
        Args.notNull(responseFactory, "Response factory");
        this.responseFactory = responseFactory;
    }

    


    @Deprecated
    public void setParams(final HttpParams params) {
        this.params = params;
    }

    


    @Deprecated
    public void setHandlerResolver(final HttpRequestHandlerResolver handlerResolver) {
        this.handlerMapper = new HttpRequestHandlerResolverAdapter(handlerResolver);
    }

    


    @Deprecated
    public void setExpectationVerifier(final HttpExpectationVerifier expectationVerifier) {
        this.expectationVerifier = expectationVerifier;
    }

    


    @Deprecated
    public HttpParams getParams() {
        return this.params;
    }

    









    public void handleRequest(
            final HttpServerConnection conn,
            final HttpContext context) throws IOException, HttpException {

        context.setAttribute(HttpCoreContext.HTTP_CONNECTION, conn);

        HttpResponse response = null;

        try {

            final HttpRequest request = conn.receiveRequestHeader();
            if (request instanceof HttpEntityEnclosingRequest) {

                if (((HttpEntityEnclosingRequest) request).expectContinue()) {
                    response = this.responseFactory.newHttpResponse(HttpVersion.HTTP_1_1,
                            HttpStatus.SC_CONTINUE, context);
                    if (this.expectationVerifier != null) {
                        try {
                            this.expectationVerifier.verify(request, response, context);
                        } catch (final HttpException ex) {
                            response = this.responseFactory.newHttpResponse(HttpVersion.HTTP_1_0,
                                    HttpStatus.SC_INTERNAL_SERVER_ERROR, context);
                            handleException(ex, response);
                        }
                    }
                    if (response.getStatusLine().getStatusCode() < 200) {
                        
                        
                        conn.sendResponseHeader(response);
                        conn.flush();
                        response = null;
                        conn.receiveRequestEntity((HttpEntityEnclosingRequest) request);
                    }
                } else {
                    conn.receiveRequestEntity((HttpEntityEnclosingRequest) request);
                }
            }

            context.setAttribute(HttpCoreContext.HTTP_REQUEST, request);

            if (response == null) {
                response = this.responseFactory.newHttpResponse(HttpVersion.HTTP_1_1,
                        HttpStatus.SC_OK, context);
                this.processor.process(request, context);
                doService(request, response, context);
            }

            
            if (request instanceof HttpEntityEnclosingRequest) {
                final HttpEntity entity = ((HttpEntityEnclosingRequest)request).getEntity();
                EntityUtils.consume(entity);
            }

        } catch (final HttpException ex) {
            response = this.responseFactory.newHttpResponse
                (HttpVersion.HTTP_1_0, HttpStatus.SC_INTERNAL_SERVER_ERROR,
                 context);
            handleException(ex, response);
        }

        context.setAttribute(HttpCoreContext.HTTP_RESPONSE, response);

        this.processor.process(response, context);
        conn.sendResponseHeader(response);
        conn.sendResponseEntity(response);
        conn.flush();

        if (!this.connStrategy.keepAlive(response, context)) {
            conn.close();
        }
    }

    







    protected void handleException(final HttpException ex, final HttpResponse response) {
        if (ex instanceof MethodNotSupportedException) {
            response.setStatusCode(HttpStatus.SC_NOT_IMPLEMENTED);
        } else if (ex instanceof UnsupportedHttpVersionException) {
            response.setStatusCode(HttpStatus.SC_HTTP_VERSION_NOT_SUPPORTED);
        } else if (ex instanceof ProtocolException) {
            response.setStatusCode(HttpStatus.SC_BAD_REQUEST);
        } else {
            response.setStatusCode(HttpStatus.SC_INTERNAL_SERVER_ERROR);
        }
        String message = ex.getMessage();
        if (message == null) {
            message = ex.toString();
        }
        final byte[] msg = EncodingUtils.getAsciiBytes(message);
        final ByteArrayEntity entity = new ByteArrayEntity(msg);
        entity.setContentType("text/plain; charset=US-ASCII");
        response.setEntity(entity);
    }

    
















    protected void doService(
            final HttpRequest request,
            final HttpResponse response,
            final HttpContext context) throws HttpException, IOException {
        HttpRequestHandler handler = null;
        if (this.handlerMapper != null) {
            handler = this.handlerMapper.lookup(request);
        }
        if (handler != null) {
            handler.handle(request, response, context);
        } else {
            response.setStatusCode(HttpStatus.SC_NOT_IMPLEMENTED);
        }
    }

    


    @Deprecated
    private static class HttpRequestHandlerResolverAdapter implements HttpRequestHandlerMapper {

        private final HttpRequestHandlerResolver resolver;

        public HttpRequestHandlerResolverAdapter(final HttpRequestHandlerResolver resolver) {
            this.resolver = resolver;
        }

        public HttpRequestHandler lookup(final HttpRequest request) {
            return resolver.lookup(request.getRequestLine().getUri());
        }

    }

}
