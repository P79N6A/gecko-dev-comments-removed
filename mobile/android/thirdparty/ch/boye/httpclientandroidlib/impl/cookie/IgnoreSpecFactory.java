


























package ch.boye.httpclientandroidlib.impl.cookie;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.cookie.CookieSpec;
import ch.boye.httpclientandroidlib.cookie.CookieSpecFactory;
import ch.boye.httpclientandroidlib.cookie.CookieSpecProvider;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.HttpContext;






@Immutable
@SuppressWarnings("deprecation")
public class IgnoreSpecFactory implements CookieSpecFactory, CookieSpecProvider {

    public IgnoreSpecFactory() {
        super();
    }

    public CookieSpec newInstance(final HttpParams params) {
        return new IgnoreSpec();
    }

    public CookieSpec create(final HttpContext context) {
        return new IgnoreSpec();
    }

}
