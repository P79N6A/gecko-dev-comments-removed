

























package ch.boye.httpclientandroidlib.client.cache;







public class HttpCacheUpdateException extends Exception {

    private static final long serialVersionUID = 823573584868632876L;

    public HttpCacheUpdateException(final String message) {
        super(message);
    }

    public HttpCacheUpdateException(final String message, final Throwable cause) {
        super(message);
        initCause(cause);
    }

}
