


























package ch.boye.httpclientandroidlib.auth;

import ch.boye.httpclientandroidlib.protocol.HttpContext;






public interface AuthSchemeProvider {

    




    AuthScheme create(HttpContext context);

}
