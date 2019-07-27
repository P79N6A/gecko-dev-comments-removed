


























package ch.boye.httpclientandroidlib.auth.params;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.util.Args;











@Immutable
@Deprecated
public final class AuthParams {

    private AuthParams() {
        super();
    }

    






    public static String getCredentialCharset(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        String charset = (String) params.getParameter
            (AuthPNames.CREDENTIAL_CHARSET);
        if (charset == null) {
            charset = HTTP.DEF_PROTOCOL_CHARSET.name();
        }
        return charset;
    }


    





    public static void setCredentialCharset(final HttpParams params, final String charset) {
        Args.notNull(params, "HTTP parameters");
        params.setParameter(AuthPNames.CREDENTIAL_CHARSET, charset);
    }

}
