


























package ch.boye.httpclientandroidlib.impl.cookie;

import java.util.Collection;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.cookie.CookieSpec;
import ch.boye.httpclientandroidlib.cookie.CookieSpecFactory;
import ch.boye.httpclientandroidlib.cookie.CookieSpecProvider;
import ch.boye.httpclientandroidlib.cookie.params.CookieSpecPNames;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.HttpContext;







@Immutable
@SuppressWarnings("deprecation")
public class NetscapeDraftSpecFactory implements CookieSpecFactory, CookieSpecProvider {

    private final String[] datepatterns;

    public NetscapeDraftSpecFactory(final String[] datepatterns) {
        super();
        this.datepatterns = datepatterns;
    }

    public NetscapeDraftSpecFactory() {
        this(null);
    }

    public CookieSpec newInstance(final HttpParams params) {
        if (params != null) {

            String[] patterns = null;
            final Collection<?> param = (Collection<?>) params.getParameter(
                    CookieSpecPNames.DATE_PATTERNS);
            if (param != null) {
                patterns = new String[param.size()];
                patterns = param.toArray(patterns);
            }
            return new NetscapeDraftSpec(patterns);
        } else {
            return new NetscapeDraftSpec();
        }
    }

    public CookieSpec create(final HttpContext context) {
        return new NetscapeDraftSpec(this.datepatterns);
    }

}
