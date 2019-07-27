


























package ch.boye.httpclientandroidlib.impl.client;

import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpStatus;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.ServiceUnavailableRetryStrategy;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;








@Immutable
public class DefaultServiceUnavailableRetryStrategy implements ServiceUnavailableRetryStrategy {

    



    private final int maxRetries;

    



    private final long retryInterval;

    public DefaultServiceUnavailableRetryStrategy(final int maxRetries, final int retryInterval) {
        super();
        Args.positive(maxRetries, "Max retries");
        Args.positive(retryInterval, "Retry interval");
        this.maxRetries = maxRetries;
        this.retryInterval = retryInterval;
    }

    public DefaultServiceUnavailableRetryStrategy() {
        this(1, 1000);
    }

    public boolean retryRequest(final HttpResponse response, final int executionCount, final HttpContext context) {
        return executionCount <= maxRetries &&
            response.getStatusLine().getStatusCode() == HttpStatus.SC_SERVICE_UNAVAILABLE;
    }

    public long getRetryInterval() {
        return retryInterval;
    }

}
