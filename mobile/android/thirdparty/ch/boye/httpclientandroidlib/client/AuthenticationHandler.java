


























package ch.boye.httpclientandroidlib.client;

import java.util.Map;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.auth.AuthScheme;
import ch.boye.httpclientandroidlib.auth.AuthenticationException;
import ch.boye.httpclientandroidlib.auth.MalformedChallengeException;
import ch.boye.httpclientandroidlib.protocol.HttpContext;















@Deprecated
public interface AuthenticationHandler {

    








    boolean isAuthenticationRequested(
            HttpResponse response,
            HttpContext context);

    











    Map<String, Header> getChallenges(
            HttpResponse response,
            HttpContext context) throws MalformedChallengeException;

    










    AuthScheme selectScheme(
            Map<String, Header> challenges,
            HttpResponse response,
            HttpContext context) throws AuthenticationException;

}
