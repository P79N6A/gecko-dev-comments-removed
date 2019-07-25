


























package ch.boye.httpclientandroidlib.auth;

import ch.boye.httpclientandroidlib.params.HttpParams;






public interface AuthSchemeFactory {

    






    AuthScheme newInstance(HttpParams params);

}
