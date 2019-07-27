


























package ch.boye.httpclientandroidlib.params;

import ch.boye.httpclientandroidlib.util.Args;









@Deprecated
public final class HttpConnectionParams implements CoreConnectionPNames {

    private HttpConnectionParams() {
        super();
    }

    






    public static int getSoTimeout(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        return params.getIntParameter(CoreConnectionPNames.SO_TIMEOUT, 0);
    }

    





    public static void setSoTimeout(final HttpParams params, final int timeout) {
        Args.notNull(params, "HTTP parameters");
        params.setIntParameter(CoreConnectionPNames.SO_TIMEOUT, timeout);

    }

    








    public static boolean getSoReuseaddr(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        return params.getBooleanParameter(CoreConnectionPNames.SO_REUSEADDR, false);
    }

    







    public static void setSoReuseaddr(final HttpParams params, final boolean reuseaddr) {
        Args.notNull(params, "HTTP parameters");
        params.setBooleanParameter(CoreConnectionPNames.SO_REUSEADDR, reuseaddr);
    }

    






    public static boolean getTcpNoDelay(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        return params.getBooleanParameter(CoreConnectionPNames.TCP_NODELAY, true);
    }

    





    public static void setTcpNoDelay(final HttpParams params, final boolean value) {
        Args.notNull(params, "HTTP parameters");
        params.setBooleanParameter(CoreConnectionPNames.TCP_NODELAY, value);
    }

    






    public static int getSocketBufferSize(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        return params.getIntParameter(CoreConnectionPNames.SOCKET_BUFFER_SIZE, -1);
    }

    






    public static void setSocketBufferSize(final HttpParams params, final int size) {
        Args.notNull(params, "HTTP parameters");
        params.setIntParameter(CoreConnectionPNames.SOCKET_BUFFER_SIZE, size);
    }

    






    public static int getLinger(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        return params.getIntParameter(CoreConnectionPNames.SO_LINGER, -1);
    }

    





    public static void setLinger(final HttpParams params, final int value) {
        Args.notNull(params, "HTTP parameters");
        params.setIntParameter(CoreConnectionPNames.SO_LINGER, value);
    }

    






    public static int getConnectionTimeout(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        return params.getIntParameter(CoreConnectionPNames.CONNECTION_TIMEOUT, 0);
    }

    






    public static void setConnectionTimeout(final HttpParams params, final int timeout) {
        Args.notNull(params, "HTTP parameters");
        params.setIntParameter(CoreConnectionPNames.CONNECTION_TIMEOUT, timeout);
    }

    






    public static boolean isStaleCheckingEnabled(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        return params.getBooleanParameter(CoreConnectionPNames.STALE_CONNECTION_CHECK, true);
    }

    






    public static void setStaleCheckingEnabled(final HttpParams params, final boolean value) {
        Args.notNull(params, "HTTP parameters");
        params.setBooleanParameter(CoreConnectionPNames.STALE_CONNECTION_CHECK, value);
    }

    








    public static boolean getSoKeepalive(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        return params.getBooleanParameter(CoreConnectionPNames.SO_KEEPALIVE, false);
    }

    







    public static void setSoKeepalive(final HttpParams params, final boolean enableKeepalive) {
        Args.notNull(params, "HTTP parameters");
        params.setBooleanParameter(CoreConnectionPNames.SO_KEEPALIVE, enableKeepalive);
    }

}
