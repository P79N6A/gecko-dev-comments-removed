


























package ch.boye.httpclientandroidlib.params;











@Deprecated
public class HttpConnectionParamBean extends HttpAbstractParamBean {

    public HttpConnectionParamBean (final HttpParams params) {
        super(params);
    }

    public void setSoTimeout (final int soTimeout) {
        HttpConnectionParams.setSoTimeout(params, soTimeout);
    }

    public void setTcpNoDelay (final boolean tcpNoDelay) {
        HttpConnectionParams.setTcpNoDelay(params, tcpNoDelay);
    }

    public void setSocketBufferSize (final int socketBufferSize) {
        HttpConnectionParams.setSocketBufferSize(params, socketBufferSize);
    }

    public void setLinger (final int linger) {
        HttpConnectionParams.setLinger(params, linger);
    }

    public void setConnectionTimeout (final int connectionTimeout) {
        HttpConnectionParams.setConnectionTimeout(params, connectionTimeout);
    }

    public void setStaleCheckingEnabled (final boolean staleCheckingEnabled) {
        HttpConnectionParams.setStaleCheckingEnabled(params, staleCheckingEnabled);
    }

}
