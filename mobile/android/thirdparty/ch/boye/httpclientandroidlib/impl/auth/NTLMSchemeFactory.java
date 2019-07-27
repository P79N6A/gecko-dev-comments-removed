


























package ch.boye.httpclientandroidlib.impl.auth;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.auth.AuthScheme;
import ch.boye.httpclientandroidlib.auth.AuthSchemeFactory;
import ch.boye.httpclientandroidlib.auth.AuthSchemeProvider;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.HttpContext;








@Immutable
@SuppressWarnings("deprecation")
public class NTLMSchemeFactory implements AuthSchemeFactory, AuthSchemeProvider {

    public AuthScheme newInstance(final HttpParams params) {
        return new NTLMScheme();
    }

    public AuthScheme create(final HttpContext context) {
        return new NTLMScheme();
    }

}
