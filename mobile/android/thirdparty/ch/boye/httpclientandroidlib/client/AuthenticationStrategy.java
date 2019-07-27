


























package ch.boye.httpclientandroidlib.client;

import java.util.Map;
import java.util.Queue;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.auth.AuthOption;
import ch.boye.httpclientandroidlib.auth.AuthScheme;
import ch.boye.httpclientandroidlib.auth.MalformedChallengeException;
import ch.boye.httpclientandroidlib.protocol.HttpContext;











public interface AuthenticationStrategy {

    










    boolean isAuthenticationRequested(
            HttpHost authhost,
            HttpResponse response,
            HttpContext context);

    












    Map<String, Header> getChallenges(
            HttpHost authhost,
            HttpResponse response,
            HttpContext context) throws MalformedChallengeException;

    












    Queue<AuthOption> select(
            Map<String, Header> challenges,
            HttpHost authhost,
            HttpResponse response,
            HttpContext context) throws MalformedChallengeException;

    






    void authSucceeded(
            HttpHost authhost,
            AuthScheme authScheme,
            HttpContext context);

    






    void authFailed(
            HttpHost authhost,
            AuthScheme authScheme,
            HttpContext context);

}
