


























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
public class BrowserCompatSpecFactory implements CookieSpecFactory, CookieSpecProvider {

    public enum SecurityLevel {
        SECURITYLEVEL_DEFAULT,
        SECURITYLEVEL_IE_MEDIUM
    }

    private final String[] datepatterns;
    private final SecurityLevel securityLevel;

    public BrowserCompatSpecFactory(final String[] datepatterns, final SecurityLevel securityLevel) {
        super();
        this.datepatterns = datepatterns;
        this.securityLevel = securityLevel;
    }

    public BrowserCompatSpecFactory(final String[] datepatterns) {
        this(null, SecurityLevel.SECURITYLEVEL_DEFAULT);
    }

    public BrowserCompatSpecFactory() {
        this(null, SecurityLevel.SECURITYLEVEL_DEFAULT);
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
            return new BrowserCompatSpec(patterns, securityLevel);
        } else {
            return new BrowserCompatSpec(null, securityLevel);
        }
    }

    public CookieSpec create(final HttpContext context) {
        return new BrowserCompatSpec(this.datepatterns);
    }

}
