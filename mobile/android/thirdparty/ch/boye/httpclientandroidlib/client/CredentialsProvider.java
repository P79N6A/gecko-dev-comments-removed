

























package ch.boye.httpclientandroidlib.client;

import ch.boye.httpclientandroidlib.auth.AuthScope;
import ch.boye.httpclientandroidlib.auth.Credentials;











public interface CredentialsProvider {

    









    void setCredentials(AuthScope authscope, Credentials credentials);

    







    Credentials getCredentials(AuthScope authscope);

    


    void clear();

}
