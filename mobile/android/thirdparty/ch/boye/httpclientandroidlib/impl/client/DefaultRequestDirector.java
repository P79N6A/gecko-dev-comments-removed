


























package ch.boye.httpclientandroidlib.impl.client;

import java.io.IOException;
import java.io.InterruptedIOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.concurrent.TimeUnit;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.ConnectionReuseStrategy;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpEntityEnclosingRequest;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.NoHttpResponseException;
import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.auth.AuthProtocolState;
import ch.boye.httpclientandroidlib.auth.AuthScheme;
import ch.boye.httpclientandroidlib.auth.AuthState;
import ch.boye.httpclientandroidlib.auth.UsernamePasswordCredentials;
import ch.boye.httpclientandroidlib.client.AuthenticationHandler;
import ch.boye.httpclientandroidlib.client.AuthenticationStrategy;
import ch.boye.httpclientandroidlib.client.HttpRequestRetryHandler;
import ch.boye.httpclientandroidlib.client.NonRepeatableRequestException;
import ch.boye.httpclientandroidlib.client.RedirectException;
import ch.boye.httpclientandroidlib.client.RedirectHandler;
import ch.boye.httpclientandroidlib.client.RedirectStrategy;
import ch.boye.httpclientandroidlib.client.RequestDirector;
import ch.boye.httpclientandroidlib.client.UserTokenHandler;
import ch.boye.httpclientandroidlib.client.methods.AbortableHttpRequest;
import ch.boye.httpclientandroidlib.client.methods.HttpUriRequest;
import ch.boye.httpclientandroidlib.client.params.ClientPNames;
import ch.boye.httpclientandroidlib.client.params.HttpClientParams;
import ch.boye.httpclientandroidlib.client.protocol.ClientContext;
import ch.boye.httpclientandroidlib.client.utils.URIUtils;
import ch.boye.httpclientandroidlib.conn.BasicManagedEntity;
import ch.boye.httpclientandroidlib.conn.ClientConnectionManager;
import ch.boye.httpclientandroidlib.conn.ClientConnectionRequest;
import ch.boye.httpclientandroidlib.conn.ConnectionKeepAliveStrategy;
import ch.boye.httpclientandroidlib.conn.ManagedClientConnection;
import ch.boye.httpclientandroidlib.conn.routing.BasicRouteDirector;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.conn.routing.HttpRouteDirector;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoutePlanner;
import ch.boye.httpclientandroidlib.conn.scheme.Scheme;
import ch.boye.httpclientandroidlib.entity.BufferedHttpEntity;
import ch.boye.httpclientandroidlib.impl.auth.BasicScheme;
import ch.boye.httpclientandroidlib.impl.conn.ConnectionShutdownException;
import ch.boye.httpclientandroidlib.message.BasicHttpRequest;
import ch.boye.httpclientandroidlib.params.HttpConnectionParams;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.params.HttpProtocolParams;
import ch.boye.httpclientandroidlib.protocol.ExecutionContext;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.protocol.HttpProcessor;
import ch.boye.httpclientandroidlib.protocol.HttpRequestExecutor;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.EntityUtils;











































@Deprecated
@NotThreadSafe 
public class DefaultRequestDirector implements RequestDirector {

    public HttpClientAndroidLog log;

    
    protected final ClientConnectionManager connManager;

    
    protected final HttpRoutePlanner routePlanner;

    
    protected final ConnectionReuseStrategy reuseStrategy;

    
    protected final ConnectionKeepAliveStrategy keepAliveStrategy;

    
    protected final HttpRequestExecutor requestExec;

    
    protected final HttpProcessor httpProcessor;

    
    protected final HttpRequestRetryHandler retryHandler;

    
    @Deprecated
    protected final RedirectHandler redirectHandler;

    
    protected final RedirectStrategy redirectStrategy;

    
    @Deprecated
    protected final AuthenticationHandler targetAuthHandler;

    
    protected final AuthenticationStrategy targetAuthStrategy;

    
    @Deprecated
    protected final AuthenticationHandler proxyAuthHandler;

    
    protected final AuthenticationStrategy proxyAuthStrategy;

    
    protected final UserTokenHandler userTokenHandler;

    
    protected final HttpParams params;

    
    protected ManagedClientConnection managedConn;

    protected final AuthState targetAuthState;

    protected final AuthState proxyAuthState;

    private final HttpAuthenticator authenticator;

    private int execCount;

    private int redirectCount;

    private final int maxRedirects;

    private HttpHost virtualHost;

    @Deprecated
    public DefaultRequestDirector(
            final HttpRequestExecutor requestExec,
            final ClientConnectionManager conman,
            final ConnectionReuseStrategy reustrat,
            final ConnectionKeepAliveStrategy kastrat,
            final HttpRoutePlanner rouplan,
            final HttpProcessor httpProcessor,
            final HttpRequestRetryHandler retryHandler,
            final RedirectHandler redirectHandler,
            final AuthenticationHandler targetAuthHandler,
            final AuthenticationHandler proxyAuthHandler,
            final UserTokenHandler userTokenHandler,
            final HttpParams params) {
        this(new HttpClientAndroidLog(DefaultRequestDirector.class),
                requestExec, conman, reustrat, kastrat, rouplan, httpProcessor, retryHandler,
                new DefaultRedirectStrategyAdaptor(redirectHandler),
                new AuthenticationStrategyAdaptor(targetAuthHandler),
                new AuthenticationStrategyAdaptor(proxyAuthHandler),
                userTokenHandler,
                params);
    }


    @Deprecated
    public DefaultRequestDirector(
            final HttpClientAndroidLog log,
            final HttpRequestExecutor requestExec,
            final ClientConnectionManager conman,
            final ConnectionReuseStrategy reustrat,
            final ConnectionKeepAliveStrategy kastrat,
            final HttpRoutePlanner rouplan,
            final HttpProcessor httpProcessor,
            final HttpRequestRetryHandler retryHandler,
            final RedirectStrategy redirectStrategy,
            final AuthenticationHandler targetAuthHandler,
            final AuthenticationHandler proxyAuthHandler,
            final UserTokenHandler userTokenHandler,
            final HttpParams params) {
        this(new HttpClientAndroidLog(DefaultRequestDirector.class),
                requestExec, conman, reustrat, kastrat, rouplan, httpProcessor, retryHandler,
                redirectStrategy,
                new AuthenticationStrategyAdaptor(targetAuthHandler),
                new AuthenticationStrategyAdaptor(proxyAuthHandler),
                userTokenHandler,
                params);
    }

    


    public DefaultRequestDirector(
            final HttpClientAndroidLog log,
            final HttpRequestExecutor requestExec,
            final ClientConnectionManager conman,
            final ConnectionReuseStrategy reustrat,
            final ConnectionKeepAliveStrategy kastrat,
            final HttpRoutePlanner rouplan,
            final HttpProcessor httpProcessor,
            final HttpRequestRetryHandler retryHandler,
            final RedirectStrategy redirectStrategy,
            final AuthenticationStrategy targetAuthStrategy,
            final AuthenticationStrategy proxyAuthStrategy,
            final UserTokenHandler userTokenHandler,
            final HttpParams params) {

        Args.notNull(log, "Log");
        Args.notNull(requestExec, "Request executor");
        Args.notNull(conman, "Client connection manager");
        Args.notNull(reustrat, "Connection reuse strategy");
        Args.notNull(kastrat, "Connection keep alive strategy");
        Args.notNull(rouplan, "Route planner");
        Args.notNull(httpProcessor, "HTTP protocol processor");
        Args.notNull(retryHandler, "HTTP request retry handler");
        Args.notNull(redirectStrategy, "Redirect strategy");
        Args.notNull(targetAuthStrategy, "Target authentication strategy");
        Args.notNull(proxyAuthStrategy, "Proxy authentication strategy");
        Args.notNull(userTokenHandler, "User token handler");
        Args.notNull(params, "HTTP parameters");
        this.log               = log;
        this.authenticator     = new HttpAuthenticator(log);
        this.requestExec        = requestExec;
        this.connManager        = conman;
        this.reuseStrategy      = reustrat;
        this.keepAliveStrategy  = kastrat;
        this.routePlanner       = rouplan;
        this.httpProcessor      = httpProcessor;
        this.retryHandler       = retryHandler;
        this.redirectStrategy   = redirectStrategy;
        this.targetAuthStrategy = targetAuthStrategy;
        this.proxyAuthStrategy  = proxyAuthStrategy;
        this.userTokenHandler   = userTokenHandler;
        this.params             = params;

        if (redirectStrategy instanceof DefaultRedirectStrategyAdaptor) {
            this.redirectHandler = ((DefaultRedirectStrategyAdaptor) redirectStrategy).getHandler();
        } else {
            this.redirectHandler = null;
        }
        if (targetAuthStrategy instanceof AuthenticationStrategyAdaptor) {
            this.targetAuthHandler = ((AuthenticationStrategyAdaptor) targetAuthStrategy).getHandler();
        } else {
            this.targetAuthHandler = null;
        }
        if (proxyAuthStrategy instanceof AuthenticationStrategyAdaptor) {
            this.proxyAuthHandler = ((AuthenticationStrategyAdaptor) proxyAuthStrategy).getHandler();
        } else {
            this.proxyAuthHandler = null;
        }

        this.managedConn = null;

        this.execCount = 0;
        this.redirectCount = 0;
        this.targetAuthState = new AuthState();
        this.proxyAuthState = new AuthState();
        this.maxRedirects = this.params.getIntParameter(ClientPNames.MAX_REDIRECTS, 100);
    }


    private RequestWrapper wrapRequest(
            final HttpRequest request) throws ProtocolException {
        if (request instanceof HttpEntityEnclosingRequest) {
            return new EntityEnclosingRequestWrapper(
                    (HttpEntityEnclosingRequest) request);
        } else {
            return new RequestWrapper(
                    request);
        }
    }


    protected void rewriteRequestURI(
            final RequestWrapper request,
            final HttpRoute route) throws ProtocolException {
        try {

            URI uri = request.getURI();
            if (route.getProxyHost() != null && !route.isTunnelled()) {
                
                if (!uri.isAbsolute()) {
                    final HttpHost target = route.getTargetHost();
                    uri = URIUtils.rewriteURI(uri, target, true);
                } else {
                    uri = URIUtils.rewriteURI(uri);
                }
            } else {
                
                if (uri.isAbsolute()) {
                    uri = URIUtils.rewriteURI(uri, null, true);
                } else {
                    uri = URIUtils.rewriteURI(uri);
                }
            }
            request.setURI(uri);

        } catch (final URISyntaxException ex) {
            throw new ProtocolException("Invalid URI: " +
                    request.getRequestLine().getUri(), ex);
        }
    }


    
    public HttpResponse execute(final HttpHost targetHost, final HttpRequest request,
                                final HttpContext context)
        throws HttpException, IOException {

        context.setAttribute(ClientContext.TARGET_AUTH_STATE, targetAuthState);
        context.setAttribute(ClientContext.PROXY_AUTH_STATE, proxyAuthState);

        HttpHost target = targetHost;

        final HttpRequest orig = request;
        final RequestWrapper origWrapper = wrapRequest(orig);
        origWrapper.setParams(params);
        final HttpRoute origRoute = determineRoute(target, origWrapper, context);

        virtualHost = (HttpHost) origWrapper.getParams().getParameter(ClientPNames.VIRTUAL_HOST);

        
        if (virtualHost != null && virtualHost.getPort() == -1) {
            final HttpHost host = (target != null) ? target : origRoute.getTargetHost();
            final int port = host.getPort();
            if (port != -1){
                virtualHost = new HttpHost(virtualHost.getHostName(), port, virtualHost.getSchemeName());
            }
        }

        RoutedRequest roureq = new RoutedRequest(origWrapper, origRoute);

        boolean reuse = false;
        boolean done = false;
        try {
            HttpResponse response = null;
            while (!done) {
                
                
                
                

                final RequestWrapper wrapper = roureq.getRequest();
                final HttpRoute route = roureq.getRoute();
                response = null;

                
                Object userToken = context.getAttribute(ClientContext.USER_TOKEN);

                
                if (managedConn == null) {
                    final ClientConnectionRequest connRequest = connManager.requestConnection(
                            route, userToken);
                    if (orig instanceof AbortableHttpRequest) {
                        ((AbortableHttpRequest) orig).setConnectionRequest(connRequest);
                    }

                    final long timeout = HttpClientParams.getConnectionManagerTimeout(params);
                    try {
                        managedConn = connRequest.getConnection(timeout, TimeUnit.MILLISECONDS);
                    } catch(final InterruptedException interrupted) {
                        Thread.currentThread().interrupt();
                        throw new InterruptedIOException();
                    }

                    if (HttpConnectionParams.isStaleCheckingEnabled(params)) {
                        
                        if (managedConn.isOpen()) {
                            this.log.debug("Stale connection check");
                            if (managedConn.isStale()) {
                                this.log.debug("Stale connection detected");
                                managedConn.close();
                            }
                        }
                    }
                }

                if (orig instanceof AbortableHttpRequest) {
                    ((AbortableHttpRequest) orig).setReleaseTrigger(managedConn);
                }

                try {
                    tryConnect(roureq, context);
                } catch (final TunnelRefusedException ex) {
                    if (this.log.isDebugEnabled()) {
                        this.log.debug(ex.getMessage());
                    }
                    response = ex.getResponse();
                    break;
                }

                final String userinfo = wrapper.getURI().getUserInfo();
                if (userinfo != null) {
                    targetAuthState.update(
                            new BasicScheme(), new UsernamePasswordCredentials(userinfo));
                }

                
                if (virtualHost != null) {
                    target = virtualHost;
                } else {
                    final URI requestURI = wrapper.getURI();
                    if (requestURI.isAbsolute()) {
                        target = URIUtils.extractHost(requestURI);
                    }
                }
                if (target == null) {
                    target = route.getTargetHost();
                }

                
                wrapper.resetHeaders();
                
                rewriteRequestURI(wrapper, route);

                
                context.setAttribute(ExecutionContext.HTTP_TARGET_HOST, target);
                context.setAttribute(ClientContext.ROUTE, route);
                context.setAttribute(ExecutionContext.HTTP_CONNECTION, managedConn);

                
                requestExec.preProcess(wrapper, httpProcessor, context);

                response = tryExecute(roureq, context);
                if (response == null) {
                    
                    continue;
                }

                
                response.setParams(params);
                requestExec.postProcess(response, httpProcessor, context);


                
                reuse = reuseStrategy.keepAlive(response, context);
                if (reuse) {
                    
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
                    managedConn.setIdleDuration(duration, TimeUnit.MILLISECONDS);
                }

                final RoutedRequest followup = handleResponse(roureq, response, context);
                if (followup == null) {
                    done = true;
                } else {
                    if (reuse) {
                        
                        final HttpEntity entity = response.getEntity();
                        EntityUtils.consume(entity);
                        
                        
                        managedConn.markReusable();
                    } else {
                        managedConn.close();
                        if (proxyAuthState.getState().compareTo(AuthProtocolState.CHALLENGED) > 0
                                && proxyAuthState.getAuthScheme() != null
                                && proxyAuthState.getAuthScheme().isConnectionBased()) {
                            this.log.debug("Resetting proxy auth state");
                            proxyAuthState.reset();
                        }
                        if (targetAuthState.getState().compareTo(AuthProtocolState.CHALLENGED) > 0
                                && targetAuthState.getAuthScheme() != null
                                && targetAuthState.getAuthScheme().isConnectionBased()) {
                            this.log.debug("Resetting target auth state");
                            targetAuthState.reset();
                        }
                    }
                    
                    if (!followup.getRoute().equals(roureq.getRoute())) {
                        releaseConnection();
                    }
                    roureq = followup;
                }

                if (managedConn != null) {
                    if (userToken == null) {
                        userToken = userTokenHandler.getUserToken(context);
                        context.setAttribute(ClientContext.USER_TOKEN, userToken);
                    }
                    if (userToken != null) {
                        managedConn.setState(userToken);
                    }
                }

            } 


            
            if ((response == null) || (response.getEntity() == null) ||
                !response.getEntity().isStreaming()) {
                
                if (reuse) {
                    managedConn.markReusable();
                }
                releaseConnection();
            } else {
                
                HttpEntity entity = response.getEntity();
                entity = new BasicManagedEntity(entity, managedConn, reuse);
                response.setEntity(entity);
            }

            return response;

        } catch (final ConnectionShutdownException ex) {
            final InterruptedIOException ioex = new InterruptedIOException(
                    "Connection has been shut down");
            ioex.initCause(ex);
            throw ioex;
        } catch (final HttpException ex) {
            abortConnection();
            throw ex;
        } catch (final IOException ex) {
            abortConnection();
            throw ex;
        } catch (final RuntimeException ex) {
            abortConnection();
            throw ex;
        }
    } 

    



    private void tryConnect(
            final RoutedRequest req, final HttpContext context) throws HttpException, IOException {
        final HttpRoute route = req.getRoute();
        final HttpRequest wrapper = req.getRequest();

        int connectCount = 0;
        for (;;) {
            context.setAttribute(ExecutionContext.HTTP_REQUEST, wrapper);
            
            connectCount++;
            try {
                if (!managedConn.isOpen()) {
                    managedConn.open(route, context, params);
                } else {
                    managedConn.setSocketTimeout(HttpConnectionParams.getSoTimeout(params));
                }
                establishRoute(route, context);
                break;
            } catch (final IOException ex) {
                try {
                    managedConn.close();
                } catch (final IOException ignore) {
                }
                if (retryHandler.retryRequest(ex, connectCount, context)) {
                    if (this.log.isInfoEnabled()) {
                        this.log.info("I/O exception ("+ ex.getClass().getName() +
                                ") caught when connecting to "
                                + route +
                                ": "
                                + ex.getMessage());
                        if (this.log.isDebugEnabled()) {
                            this.log.debug(ex.getMessage(), ex);
                        }
                        this.log.info("Retrying connect to " + route);
                    }
                } else {
                    throw ex;
                }
            }
        }
    }

    


    private HttpResponse tryExecute(
            final RoutedRequest req, final HttpContext context) throws HttpException, IOException {
        final RequestWrapper wrapper = req.getRequest();
        final HttpRoute route = req.getRoute();
        HttpResponse response = null;

        Exception retryReason = null;
        for (;;) {
            
            execCount++;
            
            wrapper.incrementExecCount();
            if (!wrapper.isRepeatable()) {
                this.log.debug("Cannot retry non-repeatable request");
                if (retryReason != null) {
                    throw new NonRepeatableRequestException("Cannot retry request " +
                        "with a non-repeatable request entity.  The cause lists the " +
                        "reason the original request failed.", retryReason);
                } else {
                    throw new NonRepeatableRequestException("Cannot retry request " +
                            "with a non-repeatable request entity.");
                }
            }

            try {
                if (!managedConn.isOpen()) {
                    
                    
                    if (!route.isTunnelled()) {
                        this.log.debug("Reopening the direct connection.");
                        managedConn.open(route, context, params);
                    } else {
                        
                        this.log.debug("Proxied connection. Need to start over.");
                        break;
                    }
                }

                if (this.log.isDebugEnabled()) {
                    this.log.debug("Attempt " + execCount + " to execute request");
                }
                response = requestExec.execute(wrapper, managedConn, context);
                break;

            } catch (final IOException ex) {
                this.log.debug("Closing the connection.");
                try {
                    managedConn.close();
                } catch (final IOException ignore) {
                }
                if (retryHandler.retryRequest(ex, wrapper.getExecCount(), context)) {
                    if (this.log.isInfoEnabled()) {
                        this.log.info("I/O exception ("+ ex.getClass().getName() +
                                ") caught when processing request to "
                                + route +
                                ": "
                                + ex.getMessage());
                    }
                    if (this.log.isDebugEnabled()) {
                        this.log.debug(ex.getMessage(), ex);
                    }
                    if (this.log.isInfoEnabled()) {
                        this.log.info("Retrying request to " + route);
                    }
                    retryReason = ex;
                } else {
                    if (ex instanceof NoHttpResponseException) {
                        final NoHttpResponseException updatedex = new NoHttpResponseException(
                                route.getTargetHost().toHostString() + " failed to respond");
                        updatedex.setStackTrace(ex.getStackTrace());
                        throw updatedex;
                    } else {
                        throw ex;
                    }
                }
            }
        }
        return response;
    }

    




    protected void releaseConnection() {
        
        
        
        try {
            managedConn.releaseConnection();
        } catch(final IOException ignored) {
            this.log.debug("IOException releasing connection", ignored);
        }
        managedConn = null;
    }

    
















    protected HttpRoute determineRoute(final HttpHost targetHost,
                                           final HttpRequest request,
                                           final HttpContext context)
        throws HttpException {
        return this.routePlanner.determineRoute(
                targetHost != null ? targetHost : (HttpHost) request.getParams()
                        .getParameter(ClientPNames.DEFAULT_HOST),
                request, context);
    }


    








    protected void establishRoute(final HttpRoute route, final HttpContext context)
        throws HttpException, IOException {

        final HttpRouteDirector rowdy = new BasicRouteDirector();
        int step;
        do {
            final HttpRoute fact = managedConn.getRoute();
            step = rowdy.nextStep(route, fact);

            switch (step) {

            case HttpRouteDirector.CONNECT_TARGET:
            case HttpRouteDirector.CONNECT_PROXY:
                managedConn.open(route, context, this.params);
                break;

            case HttpRouteDirector.TUNNEL_TARGET: {
                final boolean secure = createTunnelToTarget(route, context);
                this.log.debug("Tunnel to target created.");
                managedConn.tunnelTarget(secure, this.params);
            }   break;

            case HttpRouteDirector.TUNNEL_PROXY: {
                
                
                
                
                final int hop = fact.getHopCount()-1; 
                final boolean secure = createTunnelToProxy(route, hop, context);
                this.log.debug("Tunnel to proxy created.");
                managedConn.tunnelProxy(route.getHopTarget(hop),
                                        secure, this.params);
            }   break;


            case HttpRouteDirector.LAYER_PROTOCOL:
                managedConn.layerProtocol(context, this.params);
                break;

            case HttpRouteDirector.UNREACHABLE:
                throw new HttpException("Unable to establish route: " +
                        "planned = " + route + "; current = " + fact);
            case HttpRouteDirector.COMPLETE:
                
                break;
            default:
                throw new IllegalStateException("Unknown step indicator "
                        + step + " from RouteDirector.");
            }

        } while (step > HttpRouteDirector.COMPLETE);

    } 


    


















    protected boolean createTunnelToTarget(final HttpRoute route,
                                           final HttpContext context)
        throws HttpException, IOException {

        final HttpHost proxy = route.getProxyHost();
        final HttpHost target = route.getTargetHost();
        HttpResponse response = null;

        for (;;) {
            if (!this.managedConn.isOpen()) {
                this.managedConn.open(route, context, this.params);
            }

            final HttpRequest connect = createConnectRequest(route, context);
            connect.setParams(this.params);

            
            context.setAttribute(ExecutionContext.HTTP_TARGET_HOST, target);
            context.setAttribute(ClientContext.ROUTE, route);
            context.setAttribute(ExecutionContext.HTTP_PROXY_HOST, proxy);
            context.setAttribute(ExecutionContext.HTTP_CONNECTION, managedConn);
            context.setAttribute(ExecutionContext.HTTP_REQUEST, connect);

            this.requestExec.preProcess(connect, this.httpProcessor, context);

            response = this.requestExec.execute(connect, this.managedConn, context);

            response.setParams(this.params);
            this.requestExec.postProcess(response, this.httpProcessor, context);

            final int status = response.getStatusLine().getStatusCode();
            if (status < 200) {
                throw new HttpException("Unexpected response to CONNECT request: " +
                        response.getStatusLine());
            }

            if (HttpClientParams.isAuthenticating(this.params)) {
                if (this.authenticator.isAuthenticationRequested(proxy, response,
                        this.proxyAuthStrategy, this.proxyAuthState, context)) {
                    if (this.authenticator.authenticate(proxy, response,
                            this.proxyAuthStrategy, this.proxyAuthState, context)) {
                        
                        if (this.reuseStrategy.keepAlive(response, context)) {
                            this.log.debug("Connection kept alive");
                            
                            final HttpEntity entity = response.getEntity();
                            EntityUtils.consume(entity);
                        } else {
                            this.managedConn.close();
                        }
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            }
        }

        final int status = response.getStatusLine().getStatusCode();

        if (status > 299) {

            
            final HttpEntity entity = response.getEntity();
            if (entity != null) {
                response.setEntity(new BufferedHttpEntity(entity));
            }

            this.managedConn.close();
            throw new TunnelRefusedException("CONNECT refused by proxy: " +
                    response.getStatusLine(), response);
        }

        this.managedConn.markReusable();

        
        
        
        
        return false;

    } 



    
















    protected boolean createTunnelToProxy(final HttpRoute route, final int hop,
                                          final HttpContext context)
        throws HttpException, IOException {

        
        
        
        
        
        
        
        

        throw new HttpException("Proxy chains are not supported.");
    }



    








    protected HttpRequest createConnectRequest(final HttpRoute route,
                                               final HttpContext context) {
        
        
        

        final HttpHost target = route.getTargetHost();

        final String host = target.getHostName();
        int port = target.getPort();
        if (port < 0) {
            final Scheme scheme = connManager.getSchemeRegistry().
                getScheme(target.getSchemeName());
            port = scheme.getDefaultPort();
        }

        final StringBuilder buffer = new StringBuilder(host.length() + 6);
        buffer.append(host);
        buffer.append(':');
        buffer.append(Integer.toString(port));

        final String authority = buffer.toString();
        final ProtocolVersion ver = HttpProtocolParams.getVersion(params);
        final HttpRequest req = new BasicHttpRequest
            ("CONNECT", authority, ver);

        return req;
    }


    












    protected RoutedRequest handleResponse(final RoutedRequest roureq,
                                           final HttpResponse response,
                                           final HttpContext context)
        throws HttpException, IOException {

        final HttpRoute route = roureq.getRoute();
        final RequestWrapper request = roureq.getRequest();

        final HttpParams params = request.getParams();

        if (HttpClientParams.isAuthenticating(params)) {
            HttpHost target = (HttpHost) context.getAttribute(ExecutionContext.HTTP_TARGET_HOST);
            if (target == null) {
                target = route.getTargetHost();
            }
            if (target.getPort() < 0) {
                final Scheme scheme = connManager.getSchemeRegistry().getScheme(target);
                target = new HttpHost(target.getHostName(), scheme.getDefaultPort(), target.getSchemeName());
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
                if (this.authenticator.authenticate(target, response,
                        this.targetAuthStrategy, this.targetAuthState, context)) {
                    
                    return roureq;
                }
            }
            if (proxyAuthRequested) {
                if (this.authenticator.authenticate(proxy, response,
                        this.proxyAuthStrategy, this.proxyAuthState, context)) {
                    
                    return roureq;
                }
            }
        }

        if (HttpClientParams.isRedirecting(params) &&
                this.redirectStrategy.isRedirected(request, response, context)) {

            if (redirectCount >= maxRedirects) {
                throw new RedirectException("Maximum redirects ("
                        + maxRedirects + ") exceeded");
            }
            redirectCount++;

            
            virtualHost = null;

            final HttpUriRequest redirect = redirectStrategy.getRedirect(request, response, context);
            final HttpRequest orig = request.getOriginal();
            redirect.setHeaders(orig.getAllHeaders());

            final URI uri = redirect.getURI();
            final HttpHost newTarget = URIUtils.extractHost(uri);
            if (newTarget == null) {
                throw new ProtocolException("Redirect URI does not specify a valid host name: " + uri);
            }

            
            if (!route.getTargetHost().equals(newTarget)) {
                this.log.debug("Resetting target auth state");
                targetAuthState.reset();
                final AuthScheme authScheme = proxyAuthState.getAuthScheme();
                if (authScheme != null && authScheme.isConnectionBased()) {
                    this.log.debug("Resetting proxy auth state");
                    proxyAuthState.reset();
                }
            }

            final RequestWrapper wrapper = wrapRequest(redirect);
            wrapper.setParams(params);

            final HttpRoute newRoute = determineRoute(newTarget, wrapper, context);
            final RoutedRequest newRequest = new RoutedRequest(wrapper, newRoute);

            if (this.log.isDebugEnabled()) {
                this.log.debug("Redirecting to '" + uri + "' via " + newRoute);
            }

            return newRequest;
        }

        return null;
    } 


    




    private void abortConnection() {
        final ManagedClientConnection mcc = managedConn;
        if (mcc != null) {
            
            
            managedConn = null;
            try {
                mcc.abortConnection();
            } catch (final IOException ex) {
                if (this.log.isDebugEnabled()) {
                    this.log.debug(ex.getMessage(), ex);
                }
            }
            
            try {
                mcc.releaseConnection();
            } catch(final IOException ignored) {
                this.log.debug("Error releasing connection", ignored);
            }
        }
    } 


} 
