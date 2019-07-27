


























package ch.boye.httpclientandroidlib.impl.execchain;

import java.io.IOException;
import java.io.InterruptedIOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.ConnectionReuseStrategy;
import ch.boye.httpclientandroidlib.HttpClientConnection;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.config.RequestConfig;
import ch.boye.httpclientandroidlib.client.methods.CloseableHttpResponse;
import ch.boye.httpclientandroidlib.client.methods.HttpExecutionAware;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestWrapper;
import ch.boye.httpclientandroidlib.client.methods.HttpUriRequest;
import ch.boye.httpclientandroidlib.client.protocol.HttpClientContext;
import ch.boye.httpclientandroidlib.client.protocol.RequestClientConnControl;
import ch.boye.httpclientandroidlib.client.utils.URIUtils;
import ch.boye.httpclientandroidlib.conn.ConnectionKeepAliveStrategy;
import ch.boye.httpclientandroidlib.conn.ConnectionRequest;
import ch.boye.httpclientandroidlib.conn.HttpClientConnectionManager;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.impl.conn.ConnectionShutdownException;
import ch.boye.httpclientandroidlib.protocol.HttpCoreContext;
import ch.boye.httpclientandroidlib.protocol.HttpProcessor;
import ch.boye.httpclientandroidlib.protocol.HttpRequestExecutor;
import ch.boye.httpclientandroidlib.protocol.ImmutableHttpProcessor;
import ch.boye.httpclientandroidlib.protocol.RequestContent;
import ch.boye.httpclientandroidlib.protocol.RequestTargetHost;
import ch.boye.httpclientandroidlib.protocol.RequestUserAgent;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.VersionInfo;










@Immutable
public class MinimalClientExec implements ClientExecChain {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    private final HttpRequestExecutor requestExecutor;
    private final HttpClientConnectionManager connManager;
    private final ConnectionReuseStrategy reuseStrategy;
    private final ConnectionKeepAliveStrategy keepAliveStrategy;
    private final HttpProcessor httpProcessor;

    public MinimalClientExec(
            final HttpRequestExecutor requestExecutor,
            final HttpClientConnectionManager connManager,
            final ConnectionReuseStrategy reuseStrategy,
            final ConnectionKeepAliveStrategy keepAliveStrategy) {
        Args.notNull(requestExecutor, "HTTP request executor");
        Args.notNull(connManager, "Client connection manager");
        Args.notNull(reuseStrategy, "Connection reuse strategy");
        Args.notNull(keepAliveStrategy, "Connection keep alive strategy");
        this.httpProcessor = new ImmutableHttpProcessor(
                new RequestContent(),
                new RequestTargetHost(),
                new RequestClientConnControl(),
                new RequestUserAgent(VersionInfo.getUserAgent(
                        "Apache-HttpClient", "ch.boye.httpclientandroidlib.client", getClass())));
        this.requestExecutor    = requestExecutor;
        this.connManager        = connManager;
        this.reuseStrategy      = reuseStrategy;
        this.keepAliveStrategy  = keepAliveStrategy;
    }

    static void rewriteRequestURI(
            final HttpRequestWrapper request,
            final HttpRoute route) throws ProtocolException {
        try {
            URI uri = request.getURI();
            if (uri != null) {
                
                if (uri.isAbsolute()) {
                    uri = URIUtils.rewriteURI(uri, null, true);
                } else {
                    uri = URIUtils.rewriteURI(uri);
                }
                request.setURI(uri);
            }
        } catch (final URISyntaxException ex) {
            throw new ProtocolException("Invalid URI: " + request.getRequestLine().getUri(), ex);
        }
    }

    public CloseableHttpResponse execute(
            final HttpRoute route,
            final HttpRequestWrapper request,
            final HttpClientContext context,
            final HttpExecutionAware execAware) throws IOException, HttpException {
        Args.notNull(route, "HTTP route");
        Args.notNull(request, "HTTP request");
        Args.notNull(context, "HTTP context");

        rewriteRequestURI(request, route);

        final ConnectionRequest connRequest = connManager.requestConnection(route, null);
        if (execAware != null) {
            if (execAware.isAborted()) {
                connRequest.cancel();
                throw new RequestAbortedException("Request aborted");
            } else {
                execAware.setCancellable(connRequest);
            }
        }

        final RequestConfig config = context.getRequestConfig();

        final HttpClientConnection managedConn;
        try {
            final int timeout = config.getConnectionRequestTimeout();
            managedConn = connRequest.get(timeout > 0 ? timeout : 0, TimeUnit.MILLISECONDS);
        } catch(final InterruptedException interrupted) {
            Thread.currentThread().interrupt();
            throw new RequestAbortedException("Request aborted", interrupted);
        } catch(final ExecutionException ex) {
            Throwable cause = ex.getCause();
            if (cause == null) {
                cause = ex;
            }
            throw new RequestAbortedException("Request execution failed", cause);
        }

        final ConnectionHolder releaseTrigger = new ConnectionHolder(log, connManager, managedConn);
        try {
            if (execAware != null) {
                if (execAware.isAborted()) {
                    releaseTrigger.close();
                    throw new RequestAbortedException("Request aborted");
                } else {
                    execAware.setCancellable(releaseTrigger);
                }
            }

            if (!managedConn.isOpen()) {
                final int timeout = config.getConnectTimeout();
                this.connManager.connect(
                    managedConn,
                    route,
                    timeout > 0 ? timeout : 0,
                    context);
                this.connManager.routeComplete(managedConn, route, context);
            }
            final int timeout = config.getSocketTimeout();
            if (timeout >= 0) {
                managedConn.setSocketTimeout(timeout);
            }

            HttpHost target = null;
            final HttpRequest original = request.getOriginal();
            if (original instanceof HttpUriRequest) {
                final URI uri = ((HttpUriRequest) original).getURI();
                if (uri.isAbsolute()) {
                    target = new HttpHost(uri.getHost(), uri.getPort(), uri.getScheme());
                }
            }
            if (target == null) {
                target = route.getTargetHost();
            }

            context.setAttribute(HttpCoreContext.HTTP_TARGET_HOST, target);
            context.setAttribute(HttpCoreContext.HTTP_REQUEST, request);
            context.setAttribute(HttpCoreContext.HTTP_CONNECTION, managedConn);
            context.setAttribute(HttpClientContext.HTTP_ROUTE, route);

            httpProcessor.process(request, context);
            final HttpResponse response = requestExecutor.execute(request, managedConn, context);
            httpProcessor.process(response, context);

            
            if (reuseStrategy.keepAlive(response, context)) {
                
                final long duration = keepAliveStrategy.getKeepAliveDuration(response, context);
                releaseTrigger.setValidFor(duration, TimeUnit.MILLISECONDS);
                releaseTrigger.markReusable();
            } else {
                releaseTrigger.markNonReusable();
            }

            
            final HttpEntity entity = response.getEntity();
            if (entity == null || !entity.isStreaming()) {
                
                releaseTrigger.releaseConnection();
                return new HttpResponseProxy(response, null);
            } else {
                return new HttpResponseProxy(response, releaseTrigger);
            }
        } catch (final ConnectionShutdownException ex) {
            final InterruptedIOException ioex = new InterruptedIOException(
                    "Connection has been shut down");
            ioex.initCause(ex);
            throw ioex;
        } catch (final HttpException ex) {
            releaseTrigger.abortConnection();
            throw ex;
        } catch (final IOException ex) {
            releaseTrigger.abortConnection();
            throw ex;
        } catch (final RuntimeException ex) {
            releaseTrigger.abortConnection();
            throw ex;
        }
    }

}
