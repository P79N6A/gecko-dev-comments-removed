

























package ch.boye.httpclientandroidlib.auth;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.util.Args;




@Immutable
public final class AuthOption {

    private final AuthScheme authScheme;
    private final Credentials creds;

    public AuthOption(final AuthScheme authScheme, final Credentials creds) {
        super();
        Args.notNull(authScheme, "Auth scheme");
        Args.notNull(creds, "User credentials");
        this.authScheme = authScheme;
        this.creds = creds;
    }

    public AuthScheme getAuthScheme() {
        return this.authScheme;
    }

    public Credentials getCredentials() {
        return this.creds;
    }

    @Override
    public String toString() {
        return this.authScheme.toString();
    }

}
