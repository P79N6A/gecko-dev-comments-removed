

























package ch.boye.httpclientandroidlib.conn.ssl;

import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;








public interface TrustStrategy {

    













    boolean isTrusted(X509Certificate[] chain, String authType) throws CertificateException;

}
