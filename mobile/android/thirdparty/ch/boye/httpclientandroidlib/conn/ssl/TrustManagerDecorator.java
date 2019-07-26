

























package ch.boye.httpclientandroidlib.conn.ssl;

import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;

import javax.net.ssl.X509TrustManager;




class TrustManagerDecorator implements X509TrustManager {

    private final X509TrustManager trustManager;
    private final TrustStrategy trustStrategy;

    TrustManagerDecorator(final X509TrustManager trustManager, final TrustStrategy trustStrategy) {
        super();
        this.trustManager = trustManager;
        this.trustStrategy = trustStrategy;
    }

    public void checkClientTrusted(
            final X509Certificate[] chain, final String authType) throws CertificateException {
        this.trustManager.checkClientTrusted(chain, authType);
    }

    public void checkServerTrusted(
            final X509Certificate[] chain, final String authType) throws CertificateException {
        if (!this.trustStrategy.isTrusted(chain, authType)) {
            this.trustManager.checkServerTrusted(chain, authType);
        }
    }

    public X509Certificate[] getAcceptedIssuers() {
        return this.trustManager.getAcceptedIssuers();
    }

}
