


























package ch.boye.httpclientandroidlib.impl.client;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.client.ClientProtocolException;
import ch.boye.httpclientandroidlib.client.config.RequestConfig;
import ch.boye.httpclientandroidlib.client.methods.CloseableHttpResponse;
import ch.boye.httpclientandroidlib.client.methods.Configurable;
import ch.boye.httpclientandroidlib.client.methods.HttpExecutionAware;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestWrapper;
import ch.boye.httpclientandroidlib.client.protocol.HttpClientContext;
import ch.boye.httpclientandroidlib.conn.ClientConnectionManager;
import ch.boye.httpclientandroidlib.conn.ClientConnectionRequest;
import ch.boye.httpclientandroidlib.conn.HttpClientConnectionManager;
import ch.boye.httpclientandroidlib.conn.ManagedClientConnection;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.conn.scheme.SchemeRegistry;
import ch.boye.httpclientandroidlib.impl.DefaultConnectionReuseStrategy;
import ch.boye.httpclientandroidlib.impl.execchain.MinimalClientExec;
import ch.boye.httpclientandroidlib.params.BasicHttpParams;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.BasicHttpContext;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.protocol.HttpRequestExecutor;
import ch.boye.httpclientandroidlib.util.Args;






@ThreadSafe
@SuppressWarnings("deprecation")
class MinimalHttpClient extends CloseableHttpClient {

    private final HttpClientConnectionManager connManager;
    private final MinimalClientExec requestExecutor;
    private final HttpParams params;

    public MinimalHttpClient(
            final HttpClientConnectionManager connManager) {
        super();
        this.connManager = Args.notNull(connManager, "HTTP connection manager");
        this.requestExecutor = new MinimalClientExec(
                new HttpRequestExecutor(),
                connManager,
                DefaultConnectionReuseStrategy.INSTANCE,
                DefaultConnectionKeepAliveStrategy.INSTANCE);
        this.params = new BasicHttpParams();
    }

    @Override
    protected CloseableHttpResponse doExecute(
            final HttpHost target,
            final HttpRequest request,
            final HttpContext context) throws IOException, ClientProtocolException {
        Args.notNull(target, "Target host");
        Args.notNull(request, "HTTP request");
        HttpExecutionAware execAware = null;
        if (request instanceof HttpExecutionAware) {
            execAware = (HttpExecutionAware) request;
        }
        try {
            final HttpRequestWrapper wrapper = HttpRequestWrapper.wrap(request);
            final HttpClientContext localcontext = HttpClientContext.adapt(
                context != null ? context : new BasicHttpContext());
            final HttpRoute route = new HttpRoute(target);
            RequestConfig config = null;
            if (request instanceof Configurable) {
                config = ((Configurable) request).getConfig();
            }
            if (config != null) {
                localcontext.setRequestConfig(config);
            }
            return this.requestExecutor.execute(route, wrapper, localcontext, execAware);
        } catch (final HttpException httpException) {
            throw new ClientProtocolException(httpException);
        }
    }

    public HttpParams getParams() {
        return this.params;
    }

    public void close() {
        this.connManager.shutdown();
    }

    public ClientConnectionManager getConnectionManager() {

        return new ClientConnectionManager() {

            public void shutdown() {
                connManager.shutdown();
            }

            public ClientConnectionRequest requestConnection(
                    final HttpRoute route, final Object state) {
                throw new UnsupportedOperationException();
            }

            public void releaseConnection(
                    final ManagedClientConnection conn,
                    final long validDuration, final TimeUnit timeUnit) {
                throw new UnsupportedOperationException();
            }

            public SchemeRegistry getSchemeRegistry() {
                throw new UnsupportedOperationException();
            }

            public void closeIdleConnections(final long idletime, final TimeUnit tunit) {
                connManager.closeIdleConnections(idletime, tunit);
            }

            public void closeExpiredConnections() {
                connManager.closeExpiredConnections();
            }

        };

    }

}
