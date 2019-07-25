


























package ch.boye.httpclientandroidlib.impl.cookie;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.cookie.CookieSpec;
import ch.boye.httpclientandroidlib.cookie.CookieSpecFactory;
import ch.boye.httpclientandroidlib.params.HttpParams;






@Immutable
public class IgnoreSpecFactory implements CookieSpecFactory {

    public CookieSpec newInstance(final HttpParams params) {
        return new IgnoreSpec();
    }

}
