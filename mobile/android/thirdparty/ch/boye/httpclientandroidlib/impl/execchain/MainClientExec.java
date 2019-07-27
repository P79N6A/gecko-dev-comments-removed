


























package ch.boye.httpclientandroidlib.impl.execchain;

import java.io.IOException;
import java.io.InterruptedIOException;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.ConnectionReuseStrategy;
import ch.boye.httpclientandroidlib.HttpClientConnection;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpEntityEnclosingRequest;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.auth.AUTH;
import ch.boye.httpclientandroidlib.auth.AuthProtocolState;
import ch.boye.httpclientandroidlib.auth.AuthState;
import ch.boye.httpclientandroidlib.client.AuthenticationStrategy;
import ch.boye.httpclientandroidlib.client.NonRepeatableRequestException;
import ch.boye.httpclientandroidlib.client.UserTokenHandler;
import ch.boye.httpclientandroidlib.client.config.RequestConfig;
import ch.boye.httpclientandroidlib.client.methods.CloseableHttpResponse;
import ch.boye.httpclientandroidlib.client.methods.HttpExecutionAware;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestWrapper;
import ch.boye.httpclientandroidlib.client.protocol.HttpClientContext;
import ch.boye.httpclientandroidlib.client.protocol.RequestClientConnControl;
import ch.boye.httpclientandroidlib.conn.ConnectionKeepAliveStrategy;
import ch.boye.httpclientandroidlib.conn.ConnectionRequest;
import ch.boye.httpclientandroidlib.conn.HttpClientConnectionManager;
import ch.boye.httpclientandroidlib.conn.routing.BasicRouteDirector;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.conn.routing.HttpRouteDirector;
import ch.boye.httpclientandroidlib.conn.routing.RouteTracker;
import ch.boye.httpclientandroidlib.entity.BufferedHttpEntity;
import ch.boye.httpclientandroidlib.impl.auth.HttpAuthenticator;
import ch.boye.httpclientandroidlib.impl.conn.ConnectionShutdownException;
import ch.boye.httpclientandroidlib.message.BasicHttpRequest;
import ch.boye.httpclientandroidlib.protocol.HttpCoreContext;
import ch.boye.httpclientandroidlib.protocol.HttpProcessor;
import ch.boye.httpclientandroidlib.protocol.HttpRequestExecutor;
import ch.boye.httpclientandroidlib.protocol.ImmutableHttpProcessor;
import ch.boye.httpclientandroidlib.protocol.RequestTargetHost;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.EntityUtils;











@Immutable
public class MainClientExec implements ClientExecChain {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    private final HttpRequestExecutor requestExecutor;
    private final HttpClientConnectionManager connManager;
    private final ConnectionReuseStrategy reuseStrategy;
    private final ConnectionKeepAliveStrategy keepAliveStrategy;
    private final HttpProcessor proxyHttpProcessor;
    private final AuthenticationStrategy targetAuthStrategy;
    private final AuthenticationStrategy proxyAuthStrategy;
    private final HttpAuthenticator authenticator;
    private final UserTokenHandler userTokenHandler;
    private final HttpRouteDirector routeDirector;


    public MainClientExec(
            final HttpRequestExecutor requestExecutor,
            final HttpClientConnectionManager connManager,
            final ConnectionReuseStrategy reuseStrategy,
            final ConnectionKeepAliveStrategy keepAliveStrategy,
            final AuthenticationStrategy targetAuthStrategy,
            final AuthenticationStrategy proxyAuthStrategy,
            final UserTokenHandler userTokenHandler) {
        Args.notNull(requestExecutor, "HTTP request executor");
        Args.notNull(connManager, "Client connection manager");
        Args.notNull(reuseStrategy, "Connection reuse strategy");
        Args.notNull(keepAliveStrategy, "Connection keep alive strategy");
        Args.notNull(targetAuthStrategy, "Target authentication strategy");
        Args.notNull(proxyAuthStrategy, "Proxy authentication strategy");
        Args.notNull(userTokenHandler, "User token handler");
        this.authenticator      = new HttpAuthenticator();
        this.proxyHttpProcessor = new ImmutableHttpProcessor(
                new RequestTargetHost(), new RequestClientConnControl());
        this.routeDirector      = new BasicRouteDirector();
        this.requestExecutor    = requestExecutor;
        this.connManager        = connManager;
        this.reuseStrategy      = reuseStrategy;
        this.keepAliveStrategy  = keepAliveStrategy;
        this.targetAuthStrategy = targetAuthStrategy;
        this.proxyAuthStrategy  = proxyAuthStrategy;
        this.userTokenHandler   = userTokenHandler;
    }

    public CloseableHttpResponse execute(
            final HttpRoute route,
            final HttpRequestWrapper request,
            final HttpClientContext context,
            final HttpExecutionAware execAware) throws IOException, HttpException {
        Args.notNull(route, "HTTP route");
        Args.notNull(request, "HTTP request");
        Args.notNull(context, "HTTP context");

        AuthState targetAuthState = context.getTargetAuthState();
        if (targetAuthState == null) {
            targetAuthState = new AuthState();
            context.setAttribute(HttpClientContext.TARGET_AUTH_STATE, targetAuthState);
        }
        AuthState proxyAuthState = context.getProxyAuthState();
        if (proxyAuthState == null) {
            proxyAuthState = new AuthState();
            context.setAttribute(HttpClientContext.PROXY_AUTH_STATE, proxyAuthState);
        }

        if (request instanceof HttpEntityEnclosingRequest) {
            RequestEntityProxy.enhance((HttpEntityEnclosingRequest) request);
        }

        Object userToken = context.getUserToken();

        final ConnectionRequest connRequest = connManager.requestConnection(route, userToken);
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

        context.setAttribute(HttpCoreContext.HTTP_CONNECTION, managedConn);

        if (config.isStaleConnectionCheckEnabled()) {
            
            if (managedConn.isOpen()) {
                this.log.debug("Stale connection check");
                if (managedConn.isStale()) {
                    this.log.debug("Stale connection detected");
                    managedConn.close();
                }
            }
        }

        final ConnectionHolder connHolder = new ConnectionHolder(this.log, this.connManager, managedConn);
        try {
            if (execAware != null) {
                execAware.setCancellable(connHolder);
            }

            HttpResponse response;
            for (int execCount = 1;; execCount++) {

                if (execCount > 1 && !RequestEntityProxy.isRepeatable(request)) {
                    throw new NonRepeatableRequestException("Cannot retry request " +
                            "with a non-repeatable request entity.");
                }

                if (execAware != null && execAware.isAborted()) {
                    throw new RequestAbortedException("Request aborted");
                }

                if (!managedConn.isOpen()) {
                    this.log.debug("Opening connection " + route);
                    try {
                        establishRoute(proxyAuthState, managedConn, route, request, context);
                    } catch (final TunnelRefusedException ex) {
                        if (this.log.isDebugEnabled()) {
                            this.log.debug(ex.getMessage());
                        }
                        response = ex.getResponse();
                        break;
                    }
                }
                final int timeout = config.getSocketTimeout();
                if (timeout >= 0) {
                    managedConn.setSocketTimeout(timeout);
                }

                if (execAware != null && execAware.isAborted()) {
                    throw new RequestAbortedException("Request aborted");
                }

                if (this.log.isDebugEnabled()) {
                    this.log.debug("Executing request " + request.getRequestLine());
                }

                if (!request.containsHeader(AUTH.WWW_AUTH_RESP)) {
                    if (this.log.isDebugEnabled()) {
                        this.log.debug("Target auth state: " + targetAuthState.getState());
                    }
                    this.authenticator.generateAuthResponse(request, targetAuthState, context);
                }
                if (!request.containsHeader(AUTH.PROXY_AUTH_RESP) && !route.isTunnelled()) {
                    if (this.log.isDebugEnabled()) {
                        this.log.debug("Proxy auth state: " + proxyAuthState.getState());
                    }
                    this.authenticator.generateAuthResponse(request, proxyAuthState, context);
                }

                response = requestExecutor.execute(request, managedConn, context);

                
                if (reuseStrategy.keepAlive(response, context)) {
                    
                    final long duration = keepAliveStrategy.getKeepAliveDuration(response, context);
                    if (this.log.isDebugEnabled()) {
                        final String s;
                        if (duration > 0) {
                            s = "for " + duration + " " + TimeUnit.MILLISECONDS;
                        } else {
                            s = "indefinitely";
                        }
                        this.log.debug("Connection can be kept alive " + s);
                    }
                    connHolder.setValidFor(duration, TimeUnit.MILLISECONDS);
                    connHolder.markReusable();
                } else {
                    connHolder.markNonReusable();
                }

                if (needAuthentication(
                        targetAuthState, proxyAuthState, route, response, context)) {
                    
                    final HttpEntity entity = response.getEntity();
                    if (connHolder.isReusable()) {
                        EntityUtils.consume(entity);
                    } else {
                        managedConn.close();
                        if (proxyAuthState.getState() == AuthProtocolState.SUCCESS
                                && proxyAuthState.getAuthScheme() != null
                                && proxyAuthState.getAuthScheme().isConnectionBased()) {
                            this.log.debug("Resetting proxy auth state");
                            proxyAuthState.reset();
                        }
                        if (targetAuthState.getState() == AuthProtocolState.SUCCESS
                                && targetAuthState.getAuthScheme() != null
                                && targetAuthState.getAuthScheme().isConnectionBased()) {
                            this.log.debug("Resetting target auth state");
                            targetAuthState.reset();
                        }
                    }
                    
                    final HttpRequest original = request.getOriginal();
                    if (!original.containsHeader(AUTH.WWW_AUTH_RESP)) {
                        request.removeHeaders(AUTH.WWW_AUTH_RESP);
                    }
                    if (!original.containsHeader(AUTH.PROXY_AUTH_RESP)) {
                        request.removeHeaders(AUTH.PROXY_AUTH_RESP);
                    }
                } else {
                    break;
                }
            }

            if (userToken == null) {
                userToken = userTokenHandler.getUserToken(context);
                context.setAttribute(HttpClientContext.USER_TOKEN, userToken);
            }
            if (userToken != null) {
                connHolder.setState(userToken);
            }

            
            final HttpEntity entity = response.getEntity();
            if (entity == null || !entity.isStreaming()) {
                
                connHolder.releaseConnection();
                return new HttpResponseProxy(response, null);
            } else {
                return new HttpResponseProxy(response, connHolder);
            }
        } catch (final ConnectionShutdownException ex) {
            final InterruptedIOException ioex = new InterruptedIOException(
                    "Connection has been shut down");
            ioex.initCause(ex);
            throw ioex;
        } catch (final HttpException ex) {
            connHolder.abortConnection();
            throw ex;
        } catch (final IOException ex) {
            connHolder.abortConnection();
            throw ex;
        } catch (final RuntimeException ex) {
            connHolder.abortConnection();
            throw ex;
        }
    }

    


    void establishRoute(
            final AuthState proxyAuthState,
            final HttpClientConnection managedConn,
            final HttpRoute route,
            final HttpRequest request,
            final HttpClientContext context) throws HttpException, IOException {
        final RequestConfig config = context.getRequestConfig();
        final int timeout = config.getConnectTimeout();
        final RouteTracker tracker = new RouteTracker(route);
        int step;
        do {
            final HttpRoute fact = tracker.toRoute();
            step = this.routeDirector.nextStep(route, fact);

            switch (step) {

            case HttpRouteDirector.CONNECT_TARGET:
                this.connManager.connect(
                        managedConn,
                        route,
                        timeout > 0 ? timeout : 0,
                        context);
                tracker.connectTarget(route.isSecure());
                break;
            case HttpRouteDirector.CONNECT_PROXY:
                this.connManager.connect(
                        managedConn,
                        route,
                        timeout > 0 ? timeout : 0,
                        context);
                final HttpHost proxy  = route.getProxyHost();
                tracker.connectProxy(proxy, false);
                break;
            case HttpRouteDirector.TUNNEL_TARGET: {
                final boolean secure = createTunnelToTarget(
                        proxyAuthState, managedConn, route, request, context);
                this.log.debug("Tunnel to target created.");
                tracker.tunnelTarget(secure);
            }   break;

            case HttpRouteDirector.TUNNEL_PROXY: {
                
                
                
                
                final int hop = fact.getHopCount()-1; 
                final boolean secure = createTunnelToProxy(route, hop, context);
                this.log.debug("Tunnel to proxy created.");
                tracker.tunnelProxy(route.getHopTarget(hop), secure);
            }   break;

            case HttpRouteDirector.LAYER_PROTOCOL:
                this.connManager.upgrade(managedConn, route, context);
                tracker.layerProtocol(route.isSecure());
                break;

            case HttpRouteDirector.UNREACHABLE:
                throw new HttpException("Unable to establish route: " +
                        "planned = " + route + "; current = " + fact);
            case HttpRouteDirector.COMPLETE:
                this.connManager.routeComplete(managedConn, route, context);
                break;
            default:
                throw new IllegalStateException("Unknown step indicator "
                        + step + " from RouteDirector.");
            }

        } while (step > HttpRouteDirector.COMPLETE);
    }

    







    private boolean createTunnelToTarget(
            final AuthState proxyAuthState,
            final HttpClientConnection managedConn,
            final HttpRoute route,
            final HttpRequest request,
            final HttpClientContext context) throws HttpException, IOException {

        final RequestConfig config = context.getRequestConfig();
        final int timeout = config.getConnectTimeout();

        final HttpHost target = route.getTargetHost();
        final HttpHost proxy = route.getProxyHost();
        HttpResponse response = null;

        final String authority = target.toHostString();
        final HttpRequest connect = new BasicHttpRequest("CONNECT", authority, request.getProtocolVersion());

        this.requestExecutor.preProcess(connect, this.proxyHttpProcessor, context);

        while (response == null) {
            if (!managedConn.isOpen()) {
                this.connManager.connect(
                        managedConn,
                        route,
                        timeout > 0 ? timeout : 0,
                        context);
            }

            connect.removeHeaders(AUTH.PROXY_AUTH_RESP);
            this.authenticator.generateAuthResponse(connect, proxyAuthState, context);

            response = this.requestExecutor.execute(connect, managedConn, context);

            final int status = response.getStatusLine().getStatusCode();
            if (status < 200) {
                throw new HttpException("Unexpected response to CONNECT request: " +
                        response.getStatusLine());
            }

            if (config.isAuthenticationEnabled()) {
                if (this.authenticator.isAuthenticationRequested(proxy, response,
                        this.proxyAuthStrategy, proxyAuthState, context)) {
                    if (this.authenticator.handleAuthChallenge(proxy, response,
                            this.proxyAuthStrategy, proxyAuthState, context)) {
                        
                        if (this.reuseStrategy.keepAlive(response, context)) {
                            this.log.debug("Connection kept alive");
                            
                            final HttpEntity entity = response.getEntity();
                            EntityUtils.consume(entity);
                        } else {
                            managedConn.close();
                        }
                        response = null;
                    }
                }
            }
        }

        final int status = response.getStatusLine().getStatusCode();

        if (status > 299) {

            
            final HttpEntity entity = response.getEntity();
            if (entity != null) {
                response.setEntity(new BufferedHttpEntity(entity));
            }

            managedConn.close();
            throw new TunnelRefusedException("CONNECT refused by proxy: " +
                    response.getStatusLine(), response);
        }

        
        
        
        
        return false;
    }

    




    private boolean createTunnelToProxy(
            final HttpRoute route,
            final int hop,
            final HttpClientContext context) throws HttpException {

        
        
        
        
        
        
        
        

        throw new HttpException("Proxy chains are not supported.");
    }

    private boolean needAuthentication(
            final AuthState targetAuthState,
            final AuthState proxyAuthState,
            final HttpRoute route,
            final HttpResponse response,
            final HttpClientContext context) {
        final RequestConfig config = context.getRequestConfig();
        if (config.isAuthenticationEnabled()) {
            HttpHost target = context.getTargetHost();
            if (target == null) {
                target = route.getTargetHost();
            }
            if (target.getPort() < 0) {
                target = new HttpHost(
                        target.getHostName(),
                        route.getTargetHost().getPort(),
                        target.getSchemeName());
            }
            final boolean targetAuthRequested = this.authenticator.isAuthenticationRequested(
                    target, response, this.targetAuthStrategy, targetAuthState, context);

            HttpHost proxy = route.getProxyHost();
            
            if (proxy == null) {
                proxy = route.getTargetHost();
            }
            final boolean proxyAuthRequested = this.authenticator.isAuthenticationRequested(
                    proxy, response, this.proxyAuthStrategy, proxyAuthState, context);

            if (targetAuthRequested) {
                return this.authenticator.handleAuthChallenge(target, response,
                        this.targetAuthStrategy, targetAuthState, context);
            }
            if (proxyAuthRequested) {
                return this.authenticator.handleAuthChallenge(proxy, response,
                        this.proxyAuthStrategy, proxyAuthState, context);
            }
        }
        return false;
    }

}
