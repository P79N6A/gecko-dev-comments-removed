


























package ch.boye.httpclientandroidlib.cookie.params;

import java.util.Collection;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.params.HttpAbstractParamBean;
import ch.boye.httpclientandroidlib.params.HttpParams;











@Deprecated
@NotThreadSafe
public class CookieSpecParamBean extends HttpAbstractParamBean {

    public CookieSpecParamBean (final HttpParams params) {
        super(params);
    }

    public void setDatePatterns (final Collection <String> patterns) {
        params.setParameter(CookieSpecPNames.DATE_PATTERNS, patterns);
    }

    public void setSingleHeader (final boolean singleHeader) {
        params.setBooleanParameter(CookieSpecPNames.SINGLE_COOKIE_HEADER, singleHeader);
    }

}
