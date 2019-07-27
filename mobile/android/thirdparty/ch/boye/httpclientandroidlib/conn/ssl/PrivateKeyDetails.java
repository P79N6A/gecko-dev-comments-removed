

























package ch.boye.httpclientandroidlib.conn.ssl;

import ch.boye.httpclientandroidlib.util.Args;

import java.security.cert.X509Certificate;
import java.util.Arrays;






public final class PrivateKeyDetails {

    private final String type;
    private final X509Certificate[] certChain;

    public PrivateKeyDetails(final String type, final X509Certificate[] certChain) {
        super();
        this.type = Args.notNull(type, "Private key type");
        this.certChain = certChain;
    }

    public String getType() {
        return type;
    }

    public X509Certificate[] getCertChain() {
        return certChain;
    }

    @Override
    public String toString() {
        return type + ':' + Arrays.toString(certChain);
    }

}
