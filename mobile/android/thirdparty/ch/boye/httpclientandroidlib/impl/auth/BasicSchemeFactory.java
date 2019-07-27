


























package ch.boye.httpclientandroidlib.impl.auth;

import java.nio.charset.Charset;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.auth.AuthScheme;
import ch.boye.httpclientandroidlib.auth.AuthSchemeFactory;
import ch.boye.httpclientandroidlib.auth.AuthSchemeProvider;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.HttpContext;







@Immutable
@SuppressWarnings("deprecation")
public class BasicSchemeFactory implements AuthSchemeFactory, AuthSchemeProvider {

    private final Charset charset;

    


    public BasicSchemeFactory(final Charset charset) {
        super();
        this.charset = charset;
    }

    public BasicSchemeFactory() {
        this(null);
    }

    public AuthScheme newInstance(final HttpParams params) {
        return new BasicScheme();
    }

    public AuthScheme create(final HttpContext context) {
        return new BasicScheme(this.charset);
    }

}
