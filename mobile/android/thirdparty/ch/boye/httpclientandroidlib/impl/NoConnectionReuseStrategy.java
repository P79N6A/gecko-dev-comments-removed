


























package ch.boye.httpclientandroidlib.impl;

import ch.boye.httpclientandroidlib.ConnectionReuseStrategy;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.protocol.HttpContext;






@Immutable
public class NoConnectionReuseStrategy implements ConnectionReuseStrategy {

    public static final NoConnectionReuseStrategy INSTANCE = new NoConnectionReuseStrategy();

    public NoConnectionReuseStrategy() {
        super();
    }

    public boolean keepAlive(final HttpResponse response, final HttpContext context) {
        return false;
    }

}
