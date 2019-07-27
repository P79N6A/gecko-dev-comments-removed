

























package ch.boye.httpclientandroidlib.impl.client;

import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.client.ConnectionBackoffStrategy;







public class NullBackoffStrategy implements ConnectionBackoffStrategy {

    public boolean shouldBackoff(final Throwable t) {
        return false;
    }

    public boolean shouldBackoff(final HttpResponse resp) {
        return false;
    }
}
