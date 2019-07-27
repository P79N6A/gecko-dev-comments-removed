

























package ch.boye.httpclientandroidlib.impl.client;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.auth.AuthScope;
import ch.boye.httpclientandroidlib.auth.Credentials;
import ch.boye.httpclientandroidlib.client.CredentialsProvider;
import ch.boye.httpclientandroidlib.util.Args;






@ThreadSafe
public class BasicCredentialsProvider implements CredentialsProvider {

    private final ConcurrentHashMap<AuthScope, Credentials> credMap;

    


    public BasicCredentialsProvider() {
        super();
        this.credMap = new ConcurrentHashMap<AuthScope, Credentials>();
    }

    public void setCredentials(
            final AuthScope authscope,
            final Credentials credentials) {
        Args.notNull(authscope, "Authentication scope");
        credMap.put(authscope, credentials);
    }

    







    private static Credentials matchCredentials(
            final Map<AuthScope, Credentials> map,
            final AuthScope authscope) {
        
        Credentials creds = map.get(authscope);
        if (creds == null) {
            
            
            int bestMatchFactor  = -1;
            AuthScope bestMatch  = null;
            for (final AuthScope current: map.keySet()) {
                final int factor = authscope.match(current);
                if (factor > bestMatchFactor) {
                    bestMatchFactor = factor;
                    bestMatch = current;
                }
            }
            if (bestMatch != null) {
                creds = map.get(bestMatch);
            }
        }
        return creds;
    }

    public Credentials getCredentials(final AuthScope authscope) {
        Args.notNull(authscope, "Authentication scope");
        return matchCredentials(this.credMap, authscope);
    }

    public void clear() {
        this.credMap.clear();
    }

    @Override
    public String toString() {
        return credMap.toString();
    }

}
