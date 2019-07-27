

























package ch.boye.httpclientandroidlib.impl.auth;

import java.util.Locale;

import ch.boye.httpclientandroidlib.FormattedHeader;
import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.auth.AUTH;
import ch.boye.httpclientandroidlib.auth.AuthenticationException;
import ch.boye.httpclientandroidlib.auth.ChallengeState;
import ch.boye.httpclientandroidlib.auth.ContextAwareAuthScheme;
import ch.boye.httpclientandroidlib.auth.Credentials;
import ch.boye.httpclientandroidlib.auth.MalformedChallengeException;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;











@NotThreadSafe
public abstract class AuthSchemeBase implements ContextAwareAuthScheme {

    private ChallengeState challengeState;

    







    @Deprecated
    public AuthSchemeBase(final ChallengeState challengeState) {
        super();
        this.challengeState = challengeState;
    }

    public AuthSchemeBase() {
        super();
    }

    









    public void processChallenge(final Header header) throws MalformedChallengeException {
        Args.notNull(header, "Header");
        final String authheader = header.getName();
        if (authheader.equalsIgnoreCase(AUTH.WWW_AUTH)) {
            this.challengeState = ChallengeState.TARGET;
        } else if (authheader.equalsIgnoreCase(AUTH.PROXY_AUTH)) {
            this.challengeState = ChallengeState.PROXY;
        } else {
            throw new MalformedChallengeException("Unexpected header name: " + authheader);
        }

        final CharArrayBuffer buffer;
        int pos;
        if (header instanceof FormattedHeader) {
            buffer = ((FormattedHeader) header).getBuffer();
            pos = ((FormattedHeader) header).getValuePos();
        } else {
            final String s = header.getValue();
            if (s == null) {
                throw new MalformedChallengeException("Header value is null");
            }
            buffer = new CharArrayBuffer(s.length());
            buffer.append(s);
            pos = 0;
        }
        while (pos < buffer.length() && HTTP.isWhitespace(buffer.charAt(pos))) {
            pos++;
        }
        final int beginIndex = pos;
        while (pos < buffer.length() && !HTTP.isWhitespace(buffer.charAt(pos))) {
            pos++;
        }
        final int endIndex = pos;
        final String s = buffer.substring(beginIndex, endIndex);
        if (!s.equalsIgnoreCase(getSchemeName())) {
            throw new MalformedChallengeException("Invalid scheme identifier: " + s);
        }

        parseChallenge(buffer, pos, buffer.length());
    }


    @SuppressWarnings("deprecation")
    public Header authenticate(
            final Credentials credentials,
            final HttpRequest request,
            final HttpContext context) throws AuthenticationException {
        return authenticate(credentials, request);
    }

    protected abstract void parseChallenge(
            CharArrayBuffer buffer, int beginIndex, int endIndex) throws MalformedChallengeException;

    



    public boolean isProxy() {
        return this.challengeState != null && this.challengeState == ChallengeState.PROXY;
    }

    




    public ChallengeState getChallengeState() {
        return this.challengeState;
    }

    @Override
    public String toString() {
        final String name = getSchemeName();
        if (name != null) {
            return name.toUpperCase(Locale.ENGLISH);
        } else {
            return super.toString();
        }
    }

}
