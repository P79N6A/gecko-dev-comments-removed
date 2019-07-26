


























package ch.boye.httpclientandroidlib.impl.client;

import java.io.IOException;
import java.io.InterruptedIOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.TimeUnit;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.ConnectionReuseStrategy;
import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpEntityEnclosingRequest;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.auth.AuthScheme;
import ch.boye.httpclientandroidlib.auth.AuthScope;
import ch.boye.httpclientandroidlib.auth.AuthState;
import ch.boye.httpclientandroidlib.auth.AuthenticationException;
import ch.boye.httpclientandroidlib.auth.Credentials;
import ch.boye.httpclientandroidlib.auth.MalformedChallengeException;
import ch.boye.httpclientandroidlib.client.AuthenticationHandler;
import ch.boye.httpclientandroidlib.client.RedirectStrategy;
import ch.boye.httpclientandroidlib.client.RequestDirector;
import ch.boye.httpclientandroidlib.client.CredentialsProvider;
import ch.boye.httpclientandroidlib.client.HttpRequestRetryHandler;
import ch.boye.httpclientandroidlib.client.NonRepeatableRequestException;
import ch.boye.httpclientandroidlib.client.RedirectException;
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
import ch.boye.httpclientandroidlib.conn.params.ConnManagerParams;
import ch.boye.httpclientandroidlib.conn.routing.BasicRouteDirector;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.conn.routing.HttpRouteDirector;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoutePlanner;
import ch.boye.httpclientandroidlib.conn.scheme.Scheme;
import ch.boye.httpclientandroidlib.entity.BufferedHttpEntity;
import ch.boye.httpclientandroidlib.impl.conn.ConnectionShutdownException;
import ch.boye.httpclientandroidlib.message.BasicHttpRequest;
import ch.boye.httpclientandroidlib.params.HttpConnectionParams;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.params.HttpProtocolParams;
import ch.boye.httpclientandroidlib.protocol.ExecutionContext;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.protocol.HttpProcessor;
import ch.boye.httpclientandroidlib.protocol.HttpRequestExecutor;
import ch.boye.httpclientandroidlib.util.EntityUtils;








































@SuppressWarnings("deprecation")
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
    protected final ch.boye.httpclientandroidlib.client.RedirectHandler redirectHandler = null;

    
    protected final RedirectStrategy redirectStrategy;

    
    protected final AuthenticationHandler targetAuthHandler;

    
    protected final AuthenticationHandler proxyAuthHandler;

    
    protected final UserTokenHandler userTokenHandler;

    
    protected final HttpParams params;

    
    protected ManagedClientConnection managedConn;

    protected final AuthState targetAuthState;

    protected final AuthState proxyAuthState;

    private int execCount;

    private int redirectCount;

    private int maxRedirects;

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
            final ch.boye.httpclientandroidlib.client.RedirectHandler redirectHandler,
            final AuthenticationHandler targetAuthHandler,
            final AuthenticationHandler proxyAuthHandler,
            final UserTokenHandler userTokenHandler,
            final HttpParams params) {
        this(new HttpClientAndroidLog(DefaultRequestDirector.class),
                requestExec, conman, reustrat, kastrat, rouplan, httpProcessor, retryHandler,
                new DefaultRedirectStrategyAdaptor(redirectHandler),
                targetAuthHandler, proxyAuthHandler, userTokenHandler, params);
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
            final AuthenticationHandler targetAuthHandler,
            final AuthenticationHandler proxyAuthHandler,
            final UserTokenHandler userTokenHandler,
            final HttpParams params) {

        if (log == null) {
            throw new IllegalArgumentException
                ("Log may not be null.");
        }
        if (requestExec == null) {
            throw new IllegalArgumentException
                ("Request executor may not be null.");
        }
        if (conman == null) {
            throw new IllegalArgumentException
                ("Client connection manager may not be null.");
        }
        if (reustrat == null) {
            throw new IllegalArgumentException
                ("Connection reuse strategy may not be null.");
        }
        if (kastrat == null) {
            throw new IllegalArgumentException
                ("Connection keep alive strategy may not be null.");
        }
        if (rouplan == null) {
            throw new IllegalArgumentException
                ("Route planner may not be null.");
        }
        if (httpProcessor == null) {
            throw new IllegalArgumentException
                ("HTTP protocol processor may not be null.");
        }
        if (retryHandler == null) {
            throw new IllegalArgumentException
                ("HTTP request retry handler may not be null.");
        }
        if (redirectStrategy == null) {
            throw new IllegalArgumentException
                ("Redirect strategy may not be null.");
        }
        if (targetAuthHandler == null) {
            throw new IllegalArgumentException
                ("Target authentication handler may not be null.");
        }
        if (proxyAuthHandler == null) {
            throw new IllegalArgumentException
                ("Proxy authentication handler may not be null.");
        }
        if (userTokenHandler == null) {
            throw new IllegalArgumentException
                ("User token handler may not be null.");
        }
        if (params == null) {
            throw new IllegalArgumentException
                ("HTTP parameters may not be null");
        }
        this.log               = log;
        this.requestExec       = requestExec;
        this.connManager       = conman;
        this.reuseStrategy     = reustrat;
        this.keepAliveStrategy = kastrat;
        this.routePlanner      = rouplan;
        this.httpProcessor     = httpProcessor;
        this.retryHandler      = retryHandler;
        this.redirectStrategy  = redirectStrategy;
        this.targetAuthHandler = targetAuthHandler;
        this.proxyAuthHandler  = proxyAuthHandler;
        this.userTokenHandler  = userTokenHandler;
        this.params            = params;

        this.managedConn       = null;

        this.execCount = 0;
        this.redirectCount = 0;
        this.maxRedirects = this.params.getIntParameter(ClientPNames.MAX_REDIRECTS, 100);
        this.targetAuthState = new AuthState();
        this.proxyAuthState = new AuthState();
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
                    HttpHost target = route.getTargetHost();
                    uri = URIUtils.rewriteURI(uri, target);
                    request.setURI(uri);
                }
            } else {
                
                if (uri.isAbsolute()) {
                    uri = URIUtils.rewriteURI(uri, null);
                    request.setURI(uri);
                }
            }

        } catch (URISyntaxException ex) {
            throw new ProtocolException("Invalid URI: " +
                    request.getRequestLine().getUri(), ex);
        }
    }


    
    public HttpResponse execute(HttpHost target, HttpRequest request,
                                HttpContext context)
        throws HttpException, IOException {

        HttpRequest orig = request;
        RequestWrapper origWrapper = wrapRequest(orig);
        origWrapper.setParams(params);
        HttpRoute origRoute = determineRoute(target, origWrapper, context);

        virtualHost = (HttpHost) orig.getParams().getParameter(
                ClientPNames.VIRTUAL_HOST);
        
        
        if (virtualHost != null && virtualHost.getPort() == -1) 
        {
            int port = target.getPort();
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
                
                
                
                

                RequestWrapper wrapper = roureq.getRequest();
                HttpRoute route = roureq.getRoute();
                response = null;

                
                Object userToken = context.getAttribute(ClientContext.USER_TOKEN);

                
                if (managedConn == null) {
                    ClientConnectionRequest connRequest = connManager.requestConnection(
                            route, userToken);
                    if (orig instanceof AbortableHttpRequest) {
                        ((AbortableHttpRequest) orig).setConnectionRequest(connRequest);
                    }

                    long timeout = ConnManagerParams.getTimeout(params);
                    try {
                        managedConn = connRequest.getConnection(timeout, TimeUnit.MILLISECONDS);
                    } catch(InterruptedException interrupted) {
                        InterruptedIOException iox = new InterruptedIOException();
                        iox.initCause(interrupted);
                        throw iox;
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
                } catch (TunnelRefusedException ex) {
                    if (this.log.isDebugEnabled()) {
                        this.log.debug(ex.getMessage());
                    }
                    response = ex.getResponse();
                    break;
                }

                
                wrapper.resetHeaders();

                
                rewriteRequestURI(wrapper, route);

                
                target = virtualHost;

                if (target == null) {
                    target = route.getTargetHost();
                }

                HttpHost proxy = route.getProxyHost();

                
                context.setAttribute(ExecutionContext.HTTP_TARGET_HOST,
                        target);
                context.setAttribute(ExecutionContext.HTTP_PROXY_HOST,
                        proxy);
                context.setAttribute(ExecutionContext.HTTP_CONNECTION,
                        managedConn);
                context.setAttribute(ClientContext.TARGET_AUTH_STATE,
                        targetAuthState);
                context.setAttribute(ClientContext.PROXY_AUTH_STATE,
                        proxyAuthState);

                
                requestExec.preProcess(wrapper, httpProcessor, context);

                response = tryExecute(roureq, context);
                if (response == null) {
                    
                    continue;
                }

                
                response.setParams(params);
                requestExec.postProcess(response, httpProcessor, context);


                
                reuse = reuseStrategy.keepAlive(response, context);
                if (reuse) {
                    
                    long duration = keepAliveStrategy.getKeepAliveDuration(response, context);
                    if (this.log.isDebugEnabled()) {
                        String s;
                        if (duration > 0) {
                            s = "for " + duration + " " + TimeUnit.MILLISECONDS;
                        } else {
                            s = "indefinitely";
                        }
                        this.log.debug("Connection can be kept alive " + s);
                    }
                    managedConn.setIdleDuration(duration, TimeUnit.MILLISECONDS);
                }

                RoutedRequest followup = handleResponse(roureq, response, context);
                if (followup == null) {
                    done = true;
                } else {
                    if (reuse) {
                        
                        HttpEntity entity = response.getEntity();
                        EntityUtils.consume(entity);
                        
                        
                        managedConn.markReusable();
                    } else {
                        managedConn.close();
                        invalidateAuthIfSuccessful(this.proxyAuthState);                        
                        invalidateAuthIfSuccessful(this.targetAuthState);                        
                    }
                    
                    if (!followup.getRoute().equals(roureq.getRoute())) {
                        releaseConnection();
                    }
                    roureq = followup;
                }

                if (managedConn != null && userToken == null) {
                    userToken = userTokenHandler.getUserToken(context);
                    context.setAttribute(ClientContext.USER_TOKEN, userToken);
                    if (userToken != null) {
                        managedConn.setState(userToken);
                    }
                }

            } 


            
            if ((response == null) || (response.getEntity() == null) ||
                !response.getEntity().isStreaming()) {
                
                if (reuse)
                    managedConn.markReusable();
                releaseConnection();
            } else {
                
                HttpEntity entity = response.getEntity();
                entity = new BasicManagedEntity(entity, managedConn, reuse);
                response.setEntity(entity);
            }

            return response;

        } catch (ConnectionShutdownException ex) {
            InterruptedIOException ioex = new InterruptedIOException(
                    "Connection has been shut down");
            ioex.initCause(ex);
            throw ioex;
        } catch (HttpException ex) {
            abortConnection();
            throw ex;
        } catch (IOException ex) {
            abortConnection();
            throw ex;
        } catch (RuntimeException ex) {
            abortConnection();
            throw ex;
        }
    } 

    



    private void tryConnect(
            final RoutedRequest req, final HttpContext context) throws HttpException, IOException {
        HttpRoute route = req.getRoute();

        int connectCount = 0;
        for (;;) {
            
            connectCount++;
            try {
                if (!managedConn.isOpen()) {
                    managedConn.open(route, context, params);
                } else {
                    managedConn.setSocketTimeout(HttpConnectionParams.getSoTimeout(params));
                }
                establishRoute(route, context);
                break;
            } catch (IOException ex) {
                try {
                    managedConn.close();
                } catch (IOException ignore) {
                }
                if (retryHandler.retryRequest(ex, connectCount, context)) {
                    if (this.log.isInfoEnabled()) {
                        this.log.info("I/O exception ("+ ex.getClass().getName() +
                                ") caught when connecting to the target host: "
                                + ex.getMessage());
                    }
                    if (this.log.isDebugEnabled()) {
                        this.log.debug(ex.getMessage(), ex);
                    }
                    this.log.info("Retrying connect");
                } else {
                    throw ex;
                }
            }
        }
    }

    


    private HttpResponse tryExecute(
            final RoutedRequest req, final HttpContext context) throws HttpException, IOException {
        RequestWrapper wrapper = req.getRequest();
        HttpRoute route = req.getRoute();
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

            } catch (IOException ex) {
                this.log.debug("Closing the connection.");
                try {
                    managedConn.close();
                } catch (IOException ignore) {
                }
                if (retryHandler.retryRequest(ex, wrapper.getExecCount(), context)) {
                    if (this.log.isInfoEnabled()) {
                        this.log.info("I/O exception ("+ ex.getClass().getName() +
                                ") caught when processing request: "
                                + ex.getMessage());
                    }
                    if (this.log.isDebugEnabled()) {
                        this.log.debug(ex.getMessage(), ex);
                    }
                    this.log.info("Retrying request");
                    retryReason = ex;
                } else {
                    throw ex;
                }
            }
        }
        return response;
    }

    




    protected void releaseConnection() {
        
        
        
        try {
            managedConn.releaseConnection();
        } catch(IOException ignored) {
            this.log.debug("IOException releasing connection", ignored);
        }
        managedConn = null;
    }

    
















    protected HttpRoute determineRoute(HttpHost    target,
                                           HttpRequest request,
                                           HttpContext context)
        throws HttpException {

        if (target == null) {
            target = (HttpHost) request.getParams().getParameter(
                ClientPNames.DEFAULT_HOST);
        }
        if (target == null) {
            throw new IllegalStateException
                ("Target host must not be null, or set in parameters.");
        }

        return this.routePlanner.determineRoute(target, request, context);
    }


    








    protected void establishRoute(HttpRoute route, HttpContext context)
        throws HttpException, IOException {

        HttpRouteDirector rowdy = new BasicRouteDirector();
        int step;
        do {
            HttpRoute fact = managedConn.getRoute();
            step = rowdy.nextStep(route, fact);

            switch (step) {

            case HttpRouteDirector.CONNECT_TARGET:
            case HttpRouteDirector.CONNECT_PROXY:
                managedConn.open(route, context, this.params);
                break;

            case HttpRouteDirector.TUNNEL_TARGET: {
                boolean secure = createTunnelToTarget(route, context);
                this.log.debug("Tunnel to target created.");
                managedConn.tunnelTarget(secure, this.params);
            }   break;

            case HttpRouteDirector.TUNNEL_PROXY: {
                
                
                
                
                final int hop = fact.getHopCount()-1; 
                boolean secure = createTunnelToProxy(route, hop, context);
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


    


















    protected boolean createTunnelToTarget(HttpRoute route,
                                           HttpContext context)
        throws HttpException, IOException {

        HttpHost proxy = route.getProxyHost();
        HttpHost target = route.getTargetHost();
        HttpResponse response = null;

        boolean done = false;
        while (!done) {

            done = true;

            if (!this.managedConn.isOpen()) {
                this.managedConn.open(route, context, this.params);
            }

            HttpRequest connect = createConnectRequest(route, context);
            connect.setParams(this.params);

            
            context.setAttribute(ExecutionContext.HTTP_TARGET_HOST,
                    target);
            context.setAttribute(ExecutionContext.HTTP_PROXY_HOST,
                    proxy);
            context.setAttribute(ExecutionContext.HTTP_CONNECTION,
                    managedConn);
            context.setAttribute(ClientContext.TARGET_AUTH_STATE,
                    targetAuthState);
            context.setAttribute(ClientContext.PROXY_AUTH_STATE,
                    proxyAuthState);
            context.setAttribute(ExecutionContext.HTTP_REQUEST,
                    connect);

            this.requestExec.preProcess(connect, this.httpProcessor, context);

            response = this.requestExec.execute(connect, this.managedConn, context);

            response.setParams(this.params);
            this.requestExec.postProcess(response, this.httpProcessor, context);

            int status = response.getStatusLine().getStatusCode();
            if (status < 200) {
                throw new HttpException("Unexpected response to CONNECT request: " +
                        response.getStatusLine());
            }

            CredentialsProvider credsProvider = (CredentialsProvider)
                context.getAttribute(ClientContext.CREDS_PROVIDER);

            if (credsProvider != null && HttpClientParams.isAuthenticating(params)) {
                if (this.proxyAuthHandler.isAuthenticationRequested(response, context)) {

                    this.log.debug("Proxy requested authentication");
                    Map<String, Header> challenges = this.proxyAuthHandler.getChallenges(
                            response, context);
                    try {
                        processChallenges(
                                challenges, this.proxyAuthState, this.proxyAuthHandler,
                                response, context);
                    } catch (AuthenticationException ex) {
                        if (this.log.isWarnEnabled()) {
                            this.log.warn("Authentication error: " +  ex.getMessage());
                            break;
                        }
                    }
                    updateAuthState(this.proxyAuthState, proxy, credsProvider);

                    if (this.proxyAuthState.getCredentials() != null) {
                        done = false;

                        
                        if (this.reuseStrategy.keepAlive(response, context)) {
                            this.log.debug("Connection kept alive");
                            
                            HttpEntity entity = response.getEntity();
                            EntityUtils.consume(entity);
                        } else {
                            this.managedConn.close();
                        }

                    }

                } else {
                    
                    this.proxyAuthState.setAuthScope(null);
                }
            }
        }

        int status = response.getStatusLine().getStatusCode(); 

        if (status > 299) {

            
            HttpEntity entity = response.getEntity();
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



    
















    protected boolean createTunnelToProxy(HttpRoute route, int hop,
                                          HttpContext context)
        throws HttpException, IOException {

        
        
        
        
        
        
        
        

        throw new HttpException("Proxy chains are not supported.");
    }



    








    protected HttpRequest createConnectRequest(HttpRoute route,
                                               HttpContext context) {
        
        
        

        HttpHost target = route.getTargetHost();

        String host = target.getHostName();
        int port = target.getPort();
        if (port < 0) {
            Scheme scheme = connManager.getSchemeRegistry().
                getScheme(target.getSchemeName());
            port = scheme.getDefaultPort();
        }

        StringBuilder buffer = new StringBuilder(host.length() + 6);
        buffer.append(host);
        buffer.append(':');
        buffer.append(Integer.toString(port));

        String authority = buffer.toString();
        ProtocolVersion ver = HttpProtocolParams.getVersion(params);
        HttpRequest req = new BasicHttpRequest
            ("CONNECT", authority, ver);

        return req;
    }


    












    protected RoutedRequest handleResponse(RoutedRequest roureq,
                                           HttpResponse response,
                                           HttpContext context)
        throws HttpException, IOException {

        HttpRoute route = roureq.getRoute();
        RequestWrapper request = roureq.getRequest();

        HttpParams params = request.getParams();
        if (HttpClientParams.isRedirecting(params) &&
                this.redirectStrategy.isRedirected(request, response, context)) {

            if (redirectCount >= maxRedirects) {
                throw new RedirectException("Maximum redirects ("
                        + maxRedirects + ") exceeded");
            }
            redirectCount++;

            
            virtualHost = null;

            HttpUriRequest redirect = redirectStrategy.getRedirect(request, response, context);
            HttpRequest orig = request.getOriginal();
            redirect.setHeaders(orig.getAllHeaders());

            URI uri = redirect.getURI();
            if (uri.getHost() == null) {
                throw new ProtocolException("Redirect URI does not specify a valid host name: " + uri);
            }

            HttpHost newTarget = new HttpHost(
                    uri.getHost(),
                    uri.getPort(),
                    uri.getScheme());

            
            targetAuthState.setAuthScope(null);
            proxyAuthState.setAuthScope(null);

            
            if (!route.getTargetHost().equals(newTarget)) {
                targetAuthState.invalidate();
                AuthScheme authScheme = proxyAuthState.getAuthScheme();
                if (authScheme != null && authScheme.isConnectionBased()) {
                    proxyAuthState.invalidate();
                }
            }

            RequestWrapper wrapper = wrapRequest(redirect);
            wrapper.setParams(params);

            HttpRoute newRoute = determineRoute(newTarget, wrapper, context);
            RoutedRequest newRequest = new RoutedRequest(wrapper, newRoute);

            if (this.log.isDebugEnabled()) {
                this.log.debug("Redirecting to '" + uri + "' via " + newRoute);
            }

            return newRequest;
        }

        CredentialsProvider credsProvider = (CredentialsProvider)
            context.getAttribute(ClientContext.CREDS_PROVIDER);

        if (credsProvider != null && HttpClientParams.isAuthenticating(params)) {

            if (this.targetAuthHandler.isAuthenticationRequested(response, context)) {

                HttpHost target = (HttpHost)
                    context.getAttribute(ExecutionContext.HTTP_TARGET_HOST);
                if (target == null) {
                    target = route.getTargetHost();
                }

                this.log.debug("Target requested authentication");
                Map<String, Header> challenges = this.targetAuthHandler.getChallenges(
                        response, context);
                try {
                    processChallenges(challenges,
                            this.targetAuthState, this.targetAuthHandler,
                            response, context);
                } catch (AuthenticationException ex) {
                    if (this.log.isWarnEnabled()) {
                        this.log.warn("Authentication error: " +  ex.getMessage());
                        return null;
                    }
                }
                updateAuthState(this.targetAuthState, target, credsProvider);

                if (this.targetAuthState.getCredentials() != null) {
                    
                    return roureq;
                } else {
                    return null;
                }
            } else {
                
                this.targetAuthState.setAuthScope(null);
            }

            if (this.proxyAuthHandler.isAuthenticationRequested(response, context)) {

                HttpHost proxy = route.getProxyHost();

                this.log.debug("Proxy requested authentication");
                Map<String, Header> challenges = this.proxyAuthHandler.getChallenges(
                        response, context);
                try {
                    processChallenges(challenges,
                            this.proxyAuthState, this.proxyAuthHandler,
                            response, context);
                } catch (AuthenticationException ex) {
                    if (this.log.isWarnEnabled()) {
                        this.log.warn("Authentication error: " +  ex.getMessage());
                        return null;
                    }
                }
                updateAuthState(this.proxyAuthState, proxy, credsProvider);

                if (this.proxyAuthState.getCredentials() != null) {
                    
                    return roureq;
                } else {
                    return null;
                }
            } else {
                
                this.proxyAuthState.setAuthScope(null);
            }
        }
        return null;
    } 


    




    private void abortConnection() {
        ManagedClientConnection mcc = managedConn;
        if (mcc != null) {
            
            
            managedConn = null;
            try {
                mcc.abortConnection();
            } catch (IOException ex) {
                if (this.log.isDebugEnabled()) {
                    this.log.debug(ex.getMessage(), ex);
                }
            }
            
            try {
                mcc.releaseConnection();
            } catch(IOException ignored) {
                this.log.debug("Error releasing connection", ignored);
            }
        }
    } 


    private void processChallenges(
            final Map<String, Header> challenges,
            final AuthState authState,
            final AuthenticationHandler authHandler,
            final HttpResponse response,
            final HttpContext context)
                throws MalformedChallengeException, AuthenticationException {

        AuthScheme authScheme = authState.getAuthScheme();
        if (authScheme == null) {
            
            authScheme = authHandler.selectScheme(challenges, response, context);
            authState.setAuthScheme(authScheme);
        }
        String id = authScheme.getSchemeName();

        Header challenge = challenges.get(id.toLowerCase(Locale.ENGLISH));
        if (challenge == null) {
            throw new AuthenticationException(id +
                " authorization challenge expected, but not found");
        }
        authScheme.processChallenge(challenge);
        this.log.debug("Authorization challenge processed");
    }


    private void updateAuthState(
            final AuthState authState,
            final HttpHost host,
            final CredentialsProvider credsProvider) {

        if (!authState.isValid()) {
            return;
        }

        String hostname = host.getHostName();
        int port = host.getPort();
        if (port < 0) {
            Scheme scheme = connManager.getSchemeRegistry().getScheme(host);
            port = scheme.getDefaultPort();
        }

        AuthScheme authScheme = authState.getAuthScheme();
        AuthScope authScope = new AuthScope(
                hostname,
                port,
                authScheme.getRealm(),
                authScheme.getSchemeName());

        if (this.log.isDebugEnabled()) {
            this.log.debug("Authentication scope: " + authScope);
        }
        Credentials creds = authState.getCredentials();
        if (creds == null) {
            creds = credsProvider.getCredentials(authScope);
            if (this.log.isDebugEnabled()) {
                if (creds != null) {
                    this.log.debug("Found credentials");
                } else {
                    this.log.debug("Credentials not found");
                }
            }
        } else {
            if (authScheme.isComplete()) {
                this.log.debug("Authentication failed");
                creds = null;
            }
        }
        authState.setAuthScope(authScope);
        authState.setCredentials(creds);
    }

    private void invalidateAuthIfSuccessful(final AuthState authState) {
        AuthScheme authscheme = authState.getAuthScheme();
        if (authscheme != null
                && authscheme.isConnectionBased()
                && authscheme.isComplete()
                && authState.getCredentials() != null) {
            authState.invalidate();
        }
    }

} 
