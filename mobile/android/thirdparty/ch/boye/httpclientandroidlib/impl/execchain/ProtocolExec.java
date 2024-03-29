


























package ch.boye.httpclientandroidlib.impl.execchain;

import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.auth.AuthScope;
import ch.boye.httpclientandroidlib.auth.UsernamePasswordCredentials;
import ch.boye.httpclientandroidlib.client.CredentialsProvider;
import ch.boye.httpclientandroidlib.client.methods.CloseableHttpResponse;
import ch.boye.httpclientandroidlib.client.methods.HttpExecutionAware;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestWrapper;
import ch.boye.httpclientandroidlib.client.methods.HttpUriRequest;
import ch.boye.httpclientandroidlib.client.params.ClientPNames;
import ch.boye.httpclientandroidlib.client.protocol.HttpClientContext;
import ch.boye.httpclientandroidlib.client.utils.URIUtils;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.impl.client.BasicCredentialsProvider;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.HttpCoreContext;
import ch.boye.httpclientandroidlib.protocol.HttpProcessor;
import ch.boye.httpclientandroidlib.util.Args;














@Immutable
@SuppressWarnings("deprecation")
public class ProtocolExec implements ClientExecChain {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    private final ClientExecChain requestExecutor;
    private final HttpProcessor httpProcessor;

    public ProtocolExec(final ClientExecChain requestExecutor, final HttpProcessor httpProcessor) {
        Args.notNull(requestExecutor, "HTTP client request executor");
        Args.notNull(httpProcessor, "HTTP protocol processor");
        this.requestExecutor = requestExecutor;
        this.httpProcessor = httpProcessor;
    }

    void rewriteRequestURI(
            final HttpRequestWrapper request,
            final HttpRoute route) throws ProtocolException {
        try {
            URI uri = request.getURI();
            if (uri != null) {
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
            }
        } catch (final URISyntaxException ex) {
            throw new ProtocolException("Invalid URI: " + request.getRequestLine().getUri(), ex);
        }
    }

    public CloseableHttpResponse execute(
            final HttpRoute route,
            final HttpRequestWrapper request,
            final HttpClientContext context,
            final HttpExecutionAware execAware) throws IOException,
        HttpException {
        Args.notNull(route, "HTTP route");
        Args.notNull(request, "HTTP request");
        Args.notNull(context, "HTTP context");

        final HttpRequest original = request.getOriginal();
        URI uri = null;
        if (original instanceof HttpUriRequest) {
            uri = ((HttpUriRequest) original).getURI();
        } else {
            final String uriString = original.getRequestLine().getUri();
            try {
                uri = URI.create(uriString);
            } catch (final IllegalArgumentException ex) {
                if (this.log.isDebugEnabled()) {
                    this.log.debug("Unable to parse '" + uriString + "' as a valid URI; " +
                        "request URI and Host header may be inconsistent", ex);
                }
            }

        }
        request.setURI(uri);

        
        rewriteRequestURI(request, route);

        final HttpParams params = request.getParams();
        HttpHost virtualHost = (HttpHost) params.getParameter(ClientPNames.VIRTUAL_HOST);
        
        if (virtualHost != null && virtualHost.getPort() == -1) {
            final int port = route.getTargetHost().getPort();
            if (port != -1) {
                virtualHost = new HttpHost(virtualHost.getHostName(), port,
                    virtualHost.getSchemeName());
            }
            if (this.log.isDebugEnabled()) {
                this.log.debug("Using virtual host" + virtualHost);
            }
        }

        HttpHost target = null;
        if (virtualHost != null) {
            target = virtualHost;
        } else {
            if (uri != null && uri.isAbsolute() && uri.getHost() != null) {
                target = new HttpHost(uri.getHost(), uri.getPort(), uri.getScheme());
            }
        }
        if (target == null) {
            target = route.getTargetHost();
        }

        
        if (uri != null) {
            final String userinfo = uri.getUserInfo();
            if (userinfo != null) {
                CredentialsProvider credsProvider = context.getCredentialsProvider();
                if (credsProvider == null) {
                    credsProvider = new BasicCredentialsProvider();
                    context.setCredentialsProvider(credsProvider);
                }
                credsProvider.setCredentials(
                        new AuthScope(target),
                        new UsernamePasswordCredentials(userinfo));
            }
        }

        
        context.setAttribute(HttpCoreContext.HTTP_TARGET_HOST, target);
        context.setAttribute(HttpClientContext.HTTP_ROUTE, route);
        context.setAttribute(HttpCoreContext.HTTP_REQUEST, request);

        this.httpProcessor.process(request, context);

        final CloseableHttpResponse response = this.requestExecutor.execute(route, request,
            context, execAware);
        try {
            
            context.setAttribute(HttpCoreContext.HTTP_RESPONSE, response);
            this.httpProcessor.process(response, context);
            return response;
        } catch (final RuntimeException ex) {
            response.close();
            throw ex;
        } catch (final IOException ex) {
            response.close();
            throw ex;
        } catch (final HttpException ex) {
            response.close();
            throw ex;
        }
    }

}
