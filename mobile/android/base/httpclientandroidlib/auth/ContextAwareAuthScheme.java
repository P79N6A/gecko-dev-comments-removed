

























package ch.boye.httpclientandroidlib.auth;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.protocol.HttpContext;











public interface ContextAwareAuthScheme extends AuthScheme {

    











    Header authenticate(
            Credentials credentials,
            HttpRequest request,
            HttpContext context) throws AuthenticationException;

}
