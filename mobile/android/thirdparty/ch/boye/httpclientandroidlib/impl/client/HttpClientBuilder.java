


























package ch.boye.httpclientandroidlib.impl.client;

import java.io.Closeable;
import java.net.ProxySelector;
import java.util.ArrayList;
import java.util.Collection;
import java.util.LinkedList;
import java.util.List;

import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocketFactory;

import ch.boye.httpclientandroidlib.ConnectionReuseStrategy;
import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequestInterceptor;
import ch.boye.httpclientandroidlib.HttpResponseInterceptor;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.auth.AuthSchemeProvider;
import ch.boye.httpclientandroidlib.client.AuthenticationStrategy;
import ch.boye.httpclientandroidlib.client.BackoffManager;
import ch.boye.httpclientandroidlib.client.ConnectionBackoffStrategy;
import ch.boye.httpclientandroidlib.client.CookieStore;
import ch.boye.httpclientandroidlib.client.CredentialsProvider;
import ch.boye.httpclientandroidlib.client.HttpRequestRetryHandler;
import ch.boye.httpclientandroidlib.client.RedirectStrategy;
import ch.boye.httpclientandroidlib.client.ServiceUnavailableRetryStrategy;
import ch.boye.httpclientandroidlib.client.UserTokenHandler;
import ch.boye.httpclientandroidlib.client.config.AuthSchemes;
import ch.boye.httpclientandroidlib.client.config.CookieSpecs;
import ch.boye.httpclientandroidlib.client.config.RequestConfig;
import ch.boye.httpclientandroidlib.client.protocol.RequestAcceptEncoding;
import ch.boye.httpclientandroidlib.client.protocol.RequestAddCookies;
import ch.boye.httpclientandroidlib.client.protocol.RequestAuthCache;
import ch.boye.httpclientandroidlib.client.protocol.RequestClientConnControl;
import ch.boye.httpclientandroidlib.client.protocol.RequestDefaultHeaders;
import ch.boye.httpclientandroidlib.client.protocol.RequestExpectContinue;
import ch.boye.httpclientandroidlib.client.protocol.ResponseContentEncoding;
import ch.boye.httpclientandroidlib.client.protocol.ResponseProcessCookies;
import ch.boye.httpclientandroidlib.config.ConnectionConfig;
import ch.boye.httpclientandroidlib.config.Lookup;
import ch.boye.httpclientandroidlib.config.RegistryBuilder;
import ch.boye.httpclientandroidlib.config.SocketConfig;
import ch.boye.httpclientandroidlib.conn.ConnectionKeepAliveStrategy;
import ch.boye.httpclientandroidlib.conn.HttpClientConnectionManager;
import ch.boye.httpclientandroidlib.conn.SchemePortResolver;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoutePlanner;
import ch.boye.httpclientandroidlib.conn.socket.ConnectionSocketFactory;
import ch.boye.httpclientandroidlib.conn.socket.LayeredConnectionSocketFactory;
import ch.boye.httpclientandroidlib.conn.socket.PlainConnectionSocketFactory;
import ch.boye.httpclientandroidlib.conn.ssl.SSLConnectionSocketFactory;
import ch.boye.httpclientandroidlib.conn.ssl.SSLContexts;
import ch.boye.httpclientandroidlib.conn.ssl.X509HostnameVerifier;
import ch.boye.httpclientandroidlib.cookie.CookieSpecProvider;
import ch.boye.httpclientandroidlib.impl.DefaultConnectionReuseStrategy;
import ch.boye.httpclientandroidlib.impl.NoConnectionReuseStrategy;
import ch.boye.httpclientandroidlib.impl.auth.BasicSchemeFactory;
import ch.boye.httpclientandroidlib.impl.auth.DigestSchemeFactory;

import ch.boye.httpclientandroidlib.impl.auth.NTLMSchemeFactory;

import ch.boye.httpclientandroidlib.impl.conn.DefaultProxyRoutePlanner;
import ch.boye.httpclientandroidlib.impl.conn.DefaultRoutePlanner;
import ch.boye.httpclientandroidlib.impl.conn.DefaultSchemePortResolver;
import ch.boye.httpclientandroidlib.impl.conn.PoolingHttpClientConnectionManager;
import ch.boye.httpclientandroidlib.impl.conn.SystemDefaultRoutePlanner;
import ch.boye.httpclientandroidlib.impl.cookie.BestMatchSpecFactory;
import ch.boye.httpclientandroidlib.impl.cookie.BrowserCompatSpecFactory;
import ch.boye.httpclientandroidlib.impl.cookie.IgnoreSpecFactory;
import ch.boye.httpclientandroidlib.impl.cookie.NetscapeDraftSpecFactory;
import ch.boye.httpclientandroidlib.impl.cookie.RFC2109SpecFactory;
import ch.boye.httpclientandroidlib.impl.cookie.RFC2965SpecFactory;
import ch.boye.httpclientandroidlib.impl.execchain.BackoffStrategyExec;
import ch.boye.httpclientandroidlib.impl.execchain.ClientExecChain;
import ch.boye.httpclientandroidlib.impl.execchain.MainClientExec;
import ch.boye.httpclientandroidlib.impl.execchain.ProtocolExec;
import ch.boye.httpclientandroidlib.impl.execchain.RedirectExec;
import ch.boye.httpclientandroidlib.impl.execchain.RetryExec;
import ch.boye.httpclientandroidlib.impl.execchain.ServiceUnavailableRetryExec;
import ch.boye.httpclientandroidlib.protocol.HttpProcessor;
import ch.boye.httpclientandroidlib.protocol.HttpProcessorBuilder;
import ch.boye.httpclientandroidlib.protocol.HttpRequestExecutor;
import ch.boye.httpclientandroidlib.protocol.RequestContent;
import ch.boye.httpclientandroidlib.protocol.RequestTargetHost;
import ch.boye.httpclientandroidlib.protocol.RequestUserAgent;
import ch.boye.httpclientandroidlib.util.TextUtils;
import ch.boye.httpclientandroidlib.util.VersionInfo;




































@NotThreadSafe
public class HttpClientBuilder {

    private HttpRequestExecutor requestExec;
    private X509HostnameVerifier hostnameVerifier;
    private LayeredConnectionSocketFactory sslSocketFactory;
    private SSLContext sslcontext;
    private HttpClientConnectionManager connManager;
    private SchemePortResolver schemePortResolver;
    private ConnectionReuseStrategy reuseStrategy;
    private ConnectionKeepAliveStrategy keepAliveStrategy;
    private AuthenticationStrategy targetAuthStrategy;
    private AuthenticationStrategy proxyAuthStrategy;
    private UserTokenHandler userTokenHandler;
    private HttpProcessor httpprocessor;

    private LinkedList<HttpRequestInterceptor> requestFirst;
    private LinkedList<HttpRequestInterceptor> requestLast;
    private LinkedList<HttpResponseInterceptor> responseFirst;
    private LinkedList<HttpResponseInterceptor> responseLast;

    private HttpRequestRetryHandler retryHandler;
    private HttpRoutePlanner routePlanner;
    private RedirectStrategy redirectStrategy;
    private ConnectionBackoffStrategy connectionBackoffStrategy;
    private BackoffManager backoffManager;
    private ServiceUnavailableRetryStrategy serviceUnavailStrategy;
    private Lookup<AuthSchemeProvider> authSchemeRegistry;
    private Lookup<CookieSpecProvider> cookieSpecRegistry;
    private CookieStore cookieStore;
    private CredentialsProvider credentialsProvider;
    private String userAgent;
    private HttpHost proxy;
    private Collection<? extends Header> defaultHeaders;
    private SocketConfig defaultSocketConfig;
    private ConnectionConfig defaultConnectionConfig;
    private RequestConfig defaultRequestConfig;

    private boolean systemProperties;
    private boolean redirectHandlingDisabled;
    private boolean automaticRetriesDisabled;
    private boolean contentCompressionDisabled;
    private boolean cookieManagementDisabled;
    private boolean authCachingDisabled;
    private boolean connectionStateDisabled;

    private int maxConnTotal = 0;
    private int maxConnPerRoute = 0;

    private List<Closeable> closeables;

    static final String DEFAULT_USER_AGENT;
    static {
        final VersionInfo vi = VersionInfo.loadVersionInfo
                ("ch.boye.httpclientandroidlib.client", HttpClientBuilder.class.getClassLoader());
        final String release = (vi != null) ?
                vi.getRelease() : VersionInfo.UNAVAILABLE;
        DEFAULT_USER_AGENT = "Apache-HttpClient/" + release + " (java 1.5)";
    }

    public static HttpClientBuilder create() {
        return new HttpClientBuilder();
    }

    protected HttpClientBuilder() {
        super();
    }

    


    public final HttpClientBuilder setRequestExecutor(final HttpRequestExecutor requestExec) {
        this.requestExec = requestExec;
        return this;
    }

    






    public final HttpClientBuilder setHostnameVerifier(final X509HostnameVerifier hostnameVerifier) {
        this.hostnameVerifier = hostnameVerifier;
        return this;
    }

    







    public final HttpClientBuilder setSslcontext(final SSLContext sslcontext) {
        this.sslcontext = sslcontext;
        return this;
    }

    





    public final HttpClientBuilder setSSLSocketFactory(
            final LayeredConnectionSocketFactory sslSocketFactory) {
        this.sslSocketFactory = sslSocketFactory;
        return this;
    }

    





    public final HttpClientBuilder setMaxConnTotal(final int maxConnTotal) {
        this.maxConnTotal = maxConnTotal;
        return this;
    }

    





    public final HttpClientBuilder setMaxConnPerRoute(final int maxConnPerRoute) {
        this.maxConnPerRoute = maxConnPerRoute;
        return this;
    }

    





    public final HttpClientBuilder setDefaultSocketConfig(final SocketConfig config) {
        this.defaultSocketConfig = config;
        return this;
    }

    





    public final HttpClientBuilder setDefaultConnectionConfig(final ConnectionConfig config) {
        this.defaultConnectionConfig = config;
        return this;
    }

    


    public final HttpClientBuilder setConnectionManager(
            final HttpClientConnectionManager connManager) {
        this.connManager = connManager;
        return this;
    }

    


    public final HttpClientBuilder setConnectionReuseStrategy(
            final ConnectionReuseStrategy reuseStrategy) {
        this.reuseStrategy = reuseStrategy;
        return this;
    }

    


    public final HttpClientBuilder setKeepAliveStrategy(
            final ConnectionKeepAliveStrategy keepAliveStrategy) {
        this.keepAliveStrategy = keepAliveStrategy;
        return this;
    }

    



    public final HttpClientBuilder setTargetAuthenticationStrategy(
            final AuthenticationStrategy targetAuthStrategy) {
        this.targetAuthStrategy = targetAuthStrategy;
        return this;
    }

    



    public final HttpClientBuilder setProxyAuthenticationStrategy(
            final AuthenticationStrategy proxyAuthStrategy) {
        this.proxyAuthStrategy = proxyAuthStrategy;
        return this;
    }

    





    public final HttpClientBuilder setUserTokenHandler(final UserTokenHandler userTokenHandler) {
        this.userTokenHandler = userTokenHandler;
        return this;
    }

    


    public final HttpClientBuilder disableConnectionState() {
        connectionStateDisabled = true;
        return this;
    }

    


    public final HttpClientBuilder setSchemePortResolver(
            final SchemePortResolver schemePortResolver) {
        this.schemePortResolver = schemePortResolver;
        return this;
    }

    





    public final HttpClientBuilder setUserAgent(final String userAgent) {
        this.userAgent = userAgent;
        return this;
    }

    





    public final HttpClientBuilder setDefaultHeaders(final Collection<? extends Header> defaultHeaders) {
        this.defaultHeaders = defaultHeaders;
        return this;
    }

    





    public final HttpClientBuilder addInterceptorFirst(final HttpResponseInterceptor itcp) {
        if (itcp == null) {
            return this;
        }
        if (responseFirst == null) {
            responseFirst = new LinkedList<HttpResponseInterceptor>();
        }
        responseFirst.addFirst(itcp);
        return this;
    }

    





    public final HttpClientBuilder addInterceptorLast(final HttpResponseInterceptor itcp) {
        if (itcp == null) {
            return this;
        }
        if (responseLast == null) {
            responseLast = new LinkedList<HttpResponseInterceptor>();
        }
        responseLast.addLast(itcp);
        return this;
    }

    





    public final HttpClientBuilder addInterceptorFirst(final HttpRequestInterceptor itcp) {
        if (itcp == null) {
            return this;
        }
        if (requestFirst == null) {
            requestFirst = new LinkedList<HttpRequestInterceptor>();
        }
        requestFirst.addFirst(itcp);
        return this;
    }

    





    public final HttpClientBuilder addInterceptorLast(final HttpRequestInterceptor itcp) {
        if (itcp == null) {
            return this;
        }
        if (requestLast == null) {
            requestLast = new LinkedList<HttpRequestInterceptor>();
        }
        requestLast.addLast(itcp);
        return this;
    }

    





    public final HttpClientBuilder disableCookieManagement() {
        this.cookieManagementDisabled = true;
        return this;
    }

    





    public final HttpClientBuilder disableContentCompression() {
        contentCompressionDisabled = true;
        return this;
    }

    





    public final HttpClientBuilder disableAuthCaching() {
        this.authCachingDisabled = true;
        return this;
    }

    


    public final HttpClientBuilder setHttpProcessor(final HttpProcessor httpprocessor) {
        this.httpprocessor = httpprocessor;
        return this;
    }

    





    public final HttpClientBuilder setRetryHandler(final HttpRequestRetryHandler retryHandler) {
        this.retryHandler = retryHandler;
        return this;
    }

    


    public final HttpClientBuilder disableAutomaticRetries() {
        automaticRetriesDisabled = true;
        return this;
    }

    





    public final HttpClientBuilder setProxy(final HttpHost proxy) {
        this.proxy = proxy;
        return this;
    }

    


    public final HttpClientBuilder setRoutePlanner(final HttpRoutePlanner routePlanner) {
        this.routePlanner = routePlanner;
        return this;
    }

    





    public final HttpClientBuilder setRedirectStrategy(final RedirectStrategy redirectStrategy) {
        this.redirectStrategy = redirectStrategy;
        return this;
    }

    


    public final HttpClientBuilder disableRedirectHandling() {
        redirectHandlingDisabled = true;
        return this;
    }

    


    public final HttpClientBuilder setConnectionBackoffStrategy(
            final ConnectionBackoffStrategy connectionBackoffStrategy) {
        this.connectionBackoffStrategy = connectionBackoffStrategy;
        return this;
    }

    


    public final HttpClientBuilder setBackoffManager(final BackoffManager backoffManager) {
        this.backoffManager = backoffManager;
        return this;
    }

    


    public final HttpClientBuilder setServiceUnavailableRetryStrategy(
            final ServiceUnavailableRetryStrategy serviceUnavailStrategy) {
        this.serviceUnavailStrategy = serviceUnavailStrategy;
        return this;
    }

    



    public final HttpClientBuilder setDefaultCookieStore(final CookieStore cookieStore) {
        this.cookieStore = cookieStore;
        return this;
    }

    




    public final HttpClientBuilder setDefaultCredentialsProvider(
            final CredentialsProvider credentialsProvider) {
        this.credentialsProvider = credentialsProvider;
        return this;
    }

    




    public final HttpClientBuilder setDefaultAuthSchemeRegistry(
            final Lookup<AuthSchemeProvider> authSchemeRegistry) {
        this.authSchemeRegistry = authSchemeRegistry;
        return this;
    }

    




    public final HttpClientBuilder setDefaultCookieSpecRegistry(
            final Lookup<CookieSpecProvider> cookieSpecRegistry) {
        this.cookieSpecRegistry = cookieSpecRegistry;
        return this;
    }

    




    public final HttpClientBuilder setDefaultRequestConfig(final RequestConfig config) {
        this.defaultRequestConfig = config;
        return this;
    }

    



    public final HttpClientBuilder useSystemProperties() {
        systemProperties = true;
        return this;
    }

    


    protected ClientExecChain decorateMainExec(final ClientExecChain mainExec) {
        return mainExec;
    }

    


    protected ClientExecChain decorateProtocolExec(final ClientExecChain protocolExec) {
        return protocolExec;
    }

    


    protected void addCloseable(final Closeable closeable) {
        if (closeable == null) {
            return;
        }
        if (closeables == null) {
            closeables = new ArrayList<Closeable>();
        }
        closeables.add(closeable);
    }

    private static String[] split(final String s) {
        if (TextUtils.isBlank(s)) {
            return null;
        }
        return s.split(" *, *");
    }

    public CloseableHttpClient build() {
        
        HttpRequestExecutor requestExec = this.requestExec;
        if (requestExec == null) {
            requestExec = new HttpRequestExecutor();
        }
        HttpClientConnectionManager connManager = this.connManager;
        if (connManager == null) {
            LayeredConnectionSocketFactory sslSocketFactory = this.sslSocketFactory;
            if (sslSocketFactory == null) {
                final String[] supportedProtocols = systemProperties ? split(
                        System.getProperty("https.protocols")) : null;
                final String[] supportedCipherSuites = systemProperties ? split(
                        System.getProperty("https.cipherSuites")) : null;
                X509HostnameVerifier hostnameVerifier = this.hostnameVerifier;
                if (hostnameVerifier == null) {
                    hostnameVerifier = SSLConnectionSocketFactory.BROWSER_COMPATIBLE_HOSTNAME_VERIFIER;
                }
                if (sslcontext != null) {
                    sslSocketFactory = new SSLConnectionSocketFactory(
                            sslcontext, supportedProtocols, supportedCipherSuites, hostnameVerifier);
                } else {
                    if (systemProperties) {
                        sslSocketFactory = new SSLConnectionSocketFactory(
                                (SSLSocketFactory) SSLSocketFactory.getDefault(),
                                supportedProtocols, supportedCipherSuites, hostnameVerifier);
                    } else {
                        sslSocketFactory = new SSLConnectionSocketFactory(
                                SSLContexts.createDefault(),
                                hostnameVerifier);
                    }
                }
            }
            @SuppressWarnings("resource")
            final PoolingHttpClientConnectionManager poolingmgr = new PoolingHttpClientConnectionManager(
                    RegistryBuilder.<ConnectionSocketFactory>create()
                        .register("http", PlainConnectionSocketFactory.getSocketFactory())
                        .register("https", sslSocketFactory)
                        .build());
            if (defaultSocketConfig != null) {
                poolingmgr.setDefaultSocketConfig(defaultSocketConfig);
            }
            if (defaultConnectionConfig != null) {
                poolingmgr.setDefaultConnectionConfig(defaultConnectionConfig);
            }
            if (systemProperties) {
                String s = System.getProperty("http.keepAlive", "true");
                if ("true".equalsIgnoreCase(s)) {
                    s = System.getProperty("http.maxConnections", "5");
                    final int max = Integer.parseInt(s);
                    poolingmgr.setDefaultMaxPerRoute(max);
                    poolingmgr.setMaxTotal(2 * max);
                }
            }
            if (maxConnTotal > 0) {
                poolingmgr.setMaxTotal(maxConnTotal);
            }
            if (maxConnPerRoute > 0) {
                poolingmgr.setDefaultMaxPerRoute(maxConnPerRoute);
            }
            connManager = poolingmgr;
        }
        ConnectionReuseStrategy reuseStrategy = this.reuseStrategy;
        if (reuseStrategy == null) {
            if (systemProperties) {
                final String s = System.getProperty("http.keepAlive", "true");
                if ("true".equalsIgnoreCase(s)) {
                    reuseStrategy = DefaultConnectionReuseStrategy.INSTANCE;
                } else {
                    reuseStrategy = NoConnectionReuseStrategy.INSTANCE;
                }
            } else {
                reuseStrategy = DefaultConnectionReuseStrategy.INSTANCE;
            }
        }
        ConnectionKeepAliveStrategy keepAliveStrategy = this.keepAliveStrategy;
        if (keepAliveStrategy == null) {
            keepAliveStrategy = DefaultConnectionKeepAliveStrategy.INSTANCE;
        }
        AuthenticationStrategy targetAuthStrategy = this.targetAuthStrategy;
        if (targetAuthStrategy == null) {
            targetAuthStrategy = TargetAuthenticationStrategy.INSTANCE;
        }
        AuthenticationStrategy proxyAuthStrategy = this.proxyAuthStrategy;
        if (proxyAuthStrategy == null) {
            proxyAuthStrategy = ProxyAuthenticationStrategy.INSTANCE;
        }
        UserTokenHandler userTokenHandler = this.userTokenHandler;
        if (userTokenHandler == null) {
            if (!connectionStateDisabled) {
                userTokenHandler = DefaultUserTokenHandler.INSTANCE;
            } else {
                userTokenHandler = NoopUserTokenHandler.INSTANCE;
            }
        }
        ClientExecChain execChain = new MainClientExec(
                requestExec,
                connManager,
                reuseStrategy,
                keepAliveStrategy,
                targetAuthStrategy,
                proxyAuthStrategy,
                userTokenHandler);

        execChain = decorateMainExec(execChain);

        HttpProcessor httpprocessor = this.httpprocessor;
        if (httpprocessor == null) {

            String userAgent = this.userAgent;
            if (userAgent == null) {
                if (systemProperties) {
                    userAgent = System.getProperty("http.agent");
                }
                if (userAgent == null) {
                    userAgent = DEFAULT_USER_AGENT;
                }
            }

            final HttpProcessorBuilder b = HttpProcessorBuilder.create();
            if (requestFirst != null) {
                for (final HttpRequestInterceptor i: requestFirst) {
                    b.addFirst(i);
                }
            }
            if (responseFirst != null) {
                for (final HttpResponseInterceptor i: responseFirst) {
                    b.addFirst(i);
                }
            }
            b.addAll(
                    new RequestDefaultHeaders(defaultHeaders),
                    new RequestContent(),
                    new RequestTargetHost(),
                    new RequestClientConnControl(),
                    new RequestUserAgent(userAgent),
                    new RequestExpectContinue());
            if (!cookieManagementDisabled) {
                b.add(new RequestAddCookies());
            }
            if (!contentCompressionDisabled) {
                b.add(new RequestAcceptEncoding());
            }
            if (!authCachingDisabled) {
                b.add(new RequestAuthCache());
            }
            if (!cookieManagementDisabled) {
                b.add(new ResponseProcessCookies());
            }
            if (!contentCompressionDisabled) {
                b.add(new ResponseContentEncoding());
            }
            if (requestLast != null) {
                for (final HttpRequestInterceptor i: requestLast) {
                    b.addLast(i);
                }
            }
            if (responseLast != null) {
                for (final HttpResponseInterceptor i: responseLast) {
                    b.addLast(i);
                }
            }
            httpprocessor = b.build();
        }
        execChain = new ProtocolExec(execChain, httpprocessor);

        execChain = decorateProtocolExec(execChain);

        
        if (!automaticRetriesDisabled) {
            HttpRequestRetryHandler retryHandler = this.retryHandler;
            if (retryHandler == null) {
                retryHandler = DefaultHttpRequestRetryHandler.INSTANCE;
            }
            execChain = new RetryExec(execChain, retryHandler);
        }

        HttpRoutePlanner routePlanner = this.routePlanner;
        if (routePlanner == null) {
            SchemePortResolver schemePortResolver = this.schemePortResolver;
            if (schemePortResolver == null) {
                schemePortResolver = DefaultSchemePortResolver.INSTANCE;
            }
            if (proxy != null) {
                routePlanner = new DefaultProxyRoutePlanner(proxy, schemePortResolver);
            } else if (systemProperties) {
                routePlanner = new SystemDefaultRoutePlanner(
                        schemePortResolver, ProxySelector.getDefault());
            } else {
                routePlanner = new DefaultRoutePlanner(schemePortResolver);
            }
        }
        
        if (!redirectHandlingDisabled) {
            RedirectStrategy redirectStrategy = this.redirectStrategy;
            if (redirectStrategy == null) {
                redirectStrategy = DefaultRedirectStrategy.INSTANCE;
            }
            execChain = new RedirectExec(execChain, routePlanner, redirectStrategy);
        }

        
        final ServiceUnavailableRetryStrategy serviceUnavailStrategy = this.serviceUnavailStrategy;
        if (serviceUnavailStrategy != null) {
            execChain = new ServiceUnavailableRetryExec(execChain, serviceUnavailStrategy);
        }
        
        final BackoffManager backoffManager = this.backoffManager;
        final ConnectionBackoffStrategy connectionBackoffStrategy = this.connectionBackoffStrategy;
        if (backoffManager != null && connectionBackoffStrategy != null) {
            execChain = new BackoffStrategyExec(execChain, connectionBackoffStrategy, backoffManager);
        }

        Lookup<AuthSchemeProvider> authSchemeRegistry = this.authSchemeRegistry;
        if (authSchemeRegistry == null) {
            authSchemeRegistry = RegistryBuilder.<AuthSchemeProvider>create()
                .register(AuthSchemes.BASIC, new BasicSchemeFactory())
                .register(AuthSchemes.DIGEST, new DigestSchemeFactory())
                .register(AuthSchemes.NTLM, new NTLMSchemeFactory())
                
                
                .build();
        }
        Lookup<CookieSpecProvider> cookieSpecRegistry = this.cookieSpecRegistry;
        if (cookieSpecRegistry == null) {
            cookieSpecRegistry = RegistryBuilder.<CookieSpecProvider>create()
                .register(CookieSpecs.BEST_MATCH, new BestMatchSpecFactory())
                .register(CookieSpecs.STANDARD, new RFC2965SpecFactory())
                .register(CookieSpecs.BROWSER_COMPATIBILITY, new BrowserCompatSpecFactory())
                .register(CookieSpecs.NETSCAPE, new NetscapeDraftSpecFactory())
                .register(CookieSpecs.IGNORE_COOKIES, new IgnoreSpecFactory())
                .register("rfc2109", new RFC2109SpecFactory())
                .register("rfc2965", new RFC2965SpecFactory())
                .build();
        }

        CookieStore defaultCookieStore = this.cookieStore;
        if (defaultCookieStore == null) {
            defaultCookieStore = new BasicCookieStore();
        }

        CredentialsProvider defaultCredentialsProvider = this.credentialsProvider;
        if (defaultCredentialsProvider == null) {
            if (systemProperties) {
                defaultCredentialsProvider = new SystemDefaultCredentialsProvider();
            } else {
                defaultCredentialsProvider = new BasicCredentialsProvider();
            }
        }

        return new InternalHttpClient(
                execChain,
                connManager,
                routePlanner,
                cookieSpecRegistry,
                authSchemeRegistry,
                defaultCookieStore,
                defaultCredentialsProvider,
                defaultRequestConfig != null ? defaultRequestConfig : RequestConfig.DEFAULT,
                closeables != null ? new ArrayList<Closeable>(closeables) : null);
    }

}
