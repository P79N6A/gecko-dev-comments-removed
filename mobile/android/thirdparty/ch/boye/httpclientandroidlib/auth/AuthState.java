

























package ch.boye.httpclientandroidlib.auth;

import java.util.Queue;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;






@NotThreadSafe
public class AuthState {

    
    private AuthProtocolState state;

    
    private AuthScheme authScheme;

    
    private AuthScope authScope;

    
    private Credentials credentials;

    
    private Queue<AuthOption> authOptions;

    public AuthState() {
        super();
        this.state = AuthProtocolState.UNCHALLENGED;
    }

    




    public void reset() {
        this.state = AuthProtocolState.UNCHALLENGED;
        this.authOptions = null;
        this.authScheme = null;
        this.authScope = null;
        this.credentials = null;
    }

    


    public AuthProtocolState getState() {
        return this.state;
    }

    


    public void setState(final AuthProtocolState state) {
        this.state = state != null ? state : AuthProtocolState.UNCHALLENGED;
    }

    


    public AuthScheme getAuthScheme() {
        return this.authScheme;
    }

    


    public Credentials getCredentials() {
        return this.credentials;
    }

    







    public void update(final AuthScheme authScheme, final Credentials credentials) {
        Args.notNull(authScheme, "Auth scheme");
        Args.notNull(credentials, "Credentials");
        this.authScheme = authScheme;
        this.credentials = credentials;
        this.authOptions = null;
    }

    




    public Queue<AuthOption> getAuthOptions() {
        return this.authOptions;
    }

    





    public boolean hasAuthOptions() {
        return this.authOptions != null && !this.authOptions.isEmpty();
    }

    






    public void update(final Queue<AuthOption> authOptions) {
        Args.notEmpty(authOptions, "Queue of auth options");
        this.authOptions = authOptions;
        this.authScheme = null;
        this.credentials = null;
    }

    




    @Deprecated
    public void invalidate() {
        reset();
    }

    


    @Deprecated
    public boolean isValid() {
        return this.authScheme != null;
    }

    






    @Deprecated
    public void setAuthScheme(final AuthScheme authScheme) {
        if (authScheme == null) {
            reset();
            return;
        }
        this.authScheme = authScheme;
    }

    






    @Deprecated
    public void setCredentials(final Credentials credentials) {
        this.credentials = credentials;
    }

    






    @Deprecated
    public AuthScope getAuthScope() {
        return this.authScope;
    }

    






    @Deprecated
    public void setAuthScope(final AuthScope authScope) {
        this.authScope = authScope;
    }

    @Override
    public String toString() {
        final StringBuilder buffer = new StringBuilder();
        buffer.append("state:").append(this.state).append(";");
        if (this.authScheme != null) {
            buffer.append("auth scheme:").append(this.authScheme.getSchemeName()).append(";");
        }
        if (this.credentials != null) {
            buffer.append("credentials present");
        }
        return buffer.toString();
    }

}
