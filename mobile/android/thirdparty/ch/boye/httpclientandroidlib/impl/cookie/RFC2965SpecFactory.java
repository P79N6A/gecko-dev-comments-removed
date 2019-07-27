


























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
public class RFC2965SpecFactory implements CookieSpecFactory, CookieSpecProvider {

    private final String[] datepatterns;
    private final boolean oneHeader;

    public RFC2965SpecFactory(final String[] datepatterns, final boolean oneHeader) {
        super();
        this.datepatterns = datepatterns;
        this.oneHeader = oneHeader;
    }

    public RFC2965SpecFactory() {
        this(null, false);
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
            final boolean singleHeader = params.getBooleanParameter(
                    CookieSpecPNames.SINGLE_COOKIE_HEADER, false);

            return new RFC2965Spec(patterns, singleHeader);
        } else {
            return new RFC2965Spec();
        }
    }

    public CookieSpec create(final HttpContext context) {
        return new RFC2965Spec(this.datepatterns, this.oneHeader);
    }

}
