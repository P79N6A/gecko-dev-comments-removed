


























package ch.boye.httpclientandroidlib.conn.ssl;

import javax.net.ssl.SSLException;

import ch.boye.httpclientandroidlib.annotation.Immutable;














@Immutable
public class BrowserCompatHostnameVerifier extends AbstractVerifier {

    public final void verify(
            final String host,
            final String[] cns,
            final String[] subjectAlts) throws SSLException {
        verify(host, cns, subjectAlts, false);
    }

    @Override
    boolean validCountryWildcard(final String cn) {
        return true;
    }

    @Override
    public final String toString() {
        return "BROWSER_COMPATIBLE";
    }

}
