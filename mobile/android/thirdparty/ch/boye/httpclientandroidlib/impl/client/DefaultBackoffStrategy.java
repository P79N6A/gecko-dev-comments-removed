

























package ch.boye.httpclientandroidlib.impl.client;

import java.net.ConnectException;
import java.net.SocketTimeoutException;

import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpStatus;
import ch.boye.httpclientandroidlib.client.ConnectionBackoffStrategy;








public class DefaultBackoffStrategy implements ConnectionBackoffStrategy {

    public boolean shouldBackoff(final Throwable t) {
        return (t instanceof SocketTimeoutException
                || t instanceof ConnectException);
    }

    public boolean shouldBackoff(final HttpResponse resp) {
        return (resp.getStatusLine().getStatusCode() == HttpStatus.SC_SERVICE_UNAVAILABLE);
    }

}
