

























package ch.boye.httpclientandroidlib.auth;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpRequest;



























public interface AuthScheme {

    






    void processChallenge(final Header header) throws MalformedChallengeException;

    




    String getSchemeName();

    






    String getParameter(final String name);

    






    String getRealm();

    






    boolean isConnectionBased();

    








    boolean isComplete();

    











    @Deprecated
    Header authenticate(Credentials credentials, HttpRequest request)
            throws AuthenticationException;

}
