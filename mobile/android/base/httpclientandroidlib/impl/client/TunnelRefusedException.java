


























package ch.boye.httpclientandroidlib.impl.client;

import ch.boye.httpclientandroidlib.annotation.Immutable;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpResponse;






@Immutable
public class TunnelRefusedException extends HttpException {

    private static final long serialVersionUID = -8646722842745617323L;

    private final HttpResponse response;

    public TunnelRefusedException(final String message, final HttpResponse response) {
        super(message);
        this.response = response;
    }

    public HttpResponse getResponse() {
        return this.response;
    }

}
