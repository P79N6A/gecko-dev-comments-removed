

























package ch.boye.httpclientandroidlib.client.cache;

import java.io.IOException;





public class HttpCacheEntrySerializationException extends IOException {

    private static final long serialVersionUID = 9219188365878433519L;

    public HttpCacheEntrySerializationException(final String message) {
        super();
    }

    public HttpCacheEntrySerializationException(final String message, final Throwable cause) {
        super(message);
        initCause(cause);
    }

}
