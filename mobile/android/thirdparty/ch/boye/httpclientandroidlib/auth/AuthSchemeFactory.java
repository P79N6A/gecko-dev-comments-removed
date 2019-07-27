


























package ch.boye.httpclientandroidlib.auth;

import ch.boye.httpclientandroidlib.params.HttpParams;








@Deprecated
public interface AuthSchemeFactory {

    






    AuthScheme newInstance(HttpParams params);

}
