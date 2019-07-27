


























package ch.boye.httpclientandroidlib.impl.execchain;

import java.io.IOException;
import java.lang.reflect.UndeclaredThrowableException;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.BackoffManager;
import ch.boye.httpclientandroidlib.client.ConnectionBackoffStrategy;
import ch.boye.httpclientandroidlib.client.methods.CloseableHttpResponse;
import ch.boye.httpclientandroidlib.client.methods.HttpExecutionAware;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestWrapper;
import ch.boye.httpclientandroidlib.client.protocol.HttpClientContext;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.util.Args;




@Immutable
public class BackoffStrategyExec implements ClientExecChain {

    private final ClientExecChain requestExecutor;
    private final ConnectionBackoffStrategy connectionBackoffStrategy;
    private final BackoffManager backoffManager;

    public BackoffStrategyExec(
            final ClientExecChain requestExecutor,
            final ConnectionBackoffStrategy connectionBackoffStrategy,
            final BackoffManager backoffManager) {
        super();
        Args.notNull(requestExecutor, "HTTP client request executor");
        Args.notNull(connectionBackoffStrategy, "Connection backoff strategy");
        Args.notNull(backoffManager, "Backoff manager");
        this.requestExecutor = requestExecutor;
        this.connectionBackoffStrategy = connectionBackoffStrategy;
        this.backoffManager = backoffManager;
    }

    public CloseableHttpResponse execute(
            final HttpRoute route,
            final HttpRequestWrapper request,
            final HttpClientContext context,
            final HttpExecutionAware execAware) throws IOException, HttpException {
        Args.notNull(route, "HTTP route");
        Args.notNull(request, "HTTP request");
        Args.notNull(context, "HTTP context");
        CloseableHttpResponse out = null;
        try {
            out = this.requestExecutor.execute(route, request, context, execAware);
        } catch (final Exception ex) {
            if (out != null) {
                out.close();
            }
            if (this.connectionBackoffStrategy.shouldBackoff(ex)) {
                this.backoffManager.backOff(route);
            }
            if (ex instanceof RuntimeException) {
                throw (RuntimeException) ex;
            }
            if (ex instanceof HttpException) {
                throw (HttpException) ex;
            }
            if (ex instanceof IOException) {
                throw (IOException) ex;
            }
            throw new UndeclaredThrowableException(ex);
        }
        if (this.connectionBackoffStrategy.shouldBackoff(out)) {
            this.backoffManager.backOff(route);
        } else {
            this.backoffManager.probe(route);
        }
        return out;
    }

}
