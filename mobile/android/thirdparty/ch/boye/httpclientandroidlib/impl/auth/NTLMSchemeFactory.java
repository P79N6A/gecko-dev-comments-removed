


























package ch.boye.httpclientandroidlib.impl.auth;

import ch.boye.httpclientandroidlib.annotation.Immutable;

import ch.boye.httpclientandroidlib.auth.AuthScheme;
import ch.boye.httpclientandroidlib.auth.AuthSchemeFactory;
import ch.boye.httpclientandroidlib.params.HttpParams;








@Immutable
public class NTLMSchemeFactory implements AuthSchemeFactory {

    public AuthScheme newInstance(final HttpParams params) {
        return new NTLMScheme(new NTLMEngineImpl());
    }

}
