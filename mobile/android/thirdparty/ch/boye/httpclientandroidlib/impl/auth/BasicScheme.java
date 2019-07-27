

























package ch.boye.httpclientandroidlib.impl.auth;

import java.nio.charset.Charset;

import org.mozilla.apache.commons.codec.binary.Base64;
import ch.boye.httpclientandroidlib.Consts;
import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.auth.AUTH;
import ch.boye.httpclientandroidlib.auth.AuthenticationException;
import ch.boye.httpclientandroidlib.auth.ChallengeState;
import ch.boye.httpclientandroidlib.auth.Credentials;
import ch.boye.httpclientandroidlib.auth.MalformedChallengeException;
import ch.boye.httpclientandroidlib.message.BufferedHeader;
import ch.boye.httpclientandroidlib.protocol.BasicHttpContext;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;
import ch.boye.httpclientandroidlib.util.EncodingUtils;






@NotThreadSafe
public class BasicScheme extends RFC2617Scheme {


    
    private boolean complete;

    


    public BasicScheme(final Charset credentialsCharset) {
        super(credentialsCharset);

        this.complete = false;
    }

    







    @Deprecated
    public BasicScheme(final ChallengeState challengeState) {
        super(challengeState);

    }

    public BasicScheme() {
        this(Consts.ASCII);
    }

    




    public String getSchemeName() {
        return "basic";
    }

    







    @Override
    public void processChallenge(
            final Header header) throws MalformedChallengeException {
        super.processChallenge(header);
        this.complete = true;
    }

    





    public boolean isComplete() {
        return this.complete;
    }

    




    public boolean isConnectionBased() {
        return false;
    }

    



    @Deprecated
    public Header authenticate(
            final Credentials credentials, final HttpRequest request) throws AuthenticationException {
        return authenticate(credentials, request, new BasicHttpContext());
    }

    











    @Override
    public Header authenticate(
            final Credentials credentials,
            final HttpRequest request,
            final HttpContext context) throws AuthenticationException {

        Args.notNull(credentials, "Credentials");
        Args.notNull(request, "HTTP request");
        final StringBuilder tmp = new StringBuilder();
        tmp.append(credentials.getUserPrincipal().getName());
        tmp.append(":");
        tmp.append((credentials.getPassword() == null) ? "null" : credentials.getPassword());

        final byte[] base64password = Base64.encodeBase64(
                EncodingUtils.getBytes(tmp.toString(), getCredentialsCharset(request)));

        final CharArrayBuffer buffer = new CharArrayBuffer(32);
        if (isProxy()) {
            buffer.append(AUTH.PROXY_AUTH_RESP);
        } else {
            buffer.append(AUTH.WWW_AUTH_RESP);
        }
        buffer.append(": Basic ");
        buffer.append(base64password, 0, base64password.length);

        return new BufferedHeader(buffer);
    }

    










    @Deprecated
    public static Header authenticate(
            final Credentials credentials,
            final String charset,
            final boolean proxy) {
        Args.notNull(credentials, "Credentials");
        Args.notNull(charset, "charset");

        final StringBuilder tmp = new StringBuilder();
        tmp.append(credentials.getUserPrincipal().getName());
        tmp.append(":");
        tmp.append((credentials.getPassword() == null) ? "null" : credentials.getPassword());

        final byte[] base64password = Base64.encodeBase64(
                EncodingUtils.getBytes(tmp.toString(), charset));

        final CharArrayBuffer buffer = new CharArrayBuffer(32);
        if (proxy) {
            buffer.append(AUTH.PROXY_AUTH_RESP);
        } else {
            buffer.append(AUTH.WWW_AUTH_RESP);
        }
        buffer.append(": Basic ");
        buffer.append(base64password, 0, base64password.length);

        return new BufferedHeader(buffer);
    }

}
