


























package ch.boye.httpclientandroidlib.auth.params;

import ch.boye.httpclientandroidlib.params.HttpAbstractParamBean;
import ch.boye.httpclientandroidlib.params.HttpParams;








public class AuthParamBean extends HttpAbstractParamBean {

    public AuthParamBean (final HttpParams params) {
        super(params);
    }

    public void setCredentialCharset (final String charset) {
        AuthParams.setCredentialCharset(params, charset);
    }

}
