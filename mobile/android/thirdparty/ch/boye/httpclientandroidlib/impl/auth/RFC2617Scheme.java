

























package ch.boye.httpclientandroidlib.impl.auth;

import java.nio.charset.Charset;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

import ch.boye.httpclientandroidlib.Consts;
import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.auth.ChallengeState;
import ch.boye.httpclientandroidlib.auth.MalformedChallengeException;
import ch.boye.httpclientandroidlib.auth.params.AuthPNames;
import ch.boye.httpclientandroidlib.message.BasicHeaderValueParser;
import ch.boye.httpclientandroidlib.message.HeaderValueParser;
import ch.boye.httpclientandroidlib.message.ParserCursor;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;








@SuppressWarnings("deprecation")
@NotThreadSafe 
public abstract class RFC2617Scheme extends AuthSchemeBase {

    private final Map<String, String> params;
    private final Charset credentialsCharset;

    







    @Deprecated
    public RFC2617Scheme(final ChallengeState challengeState) {
        super(challengeState);
        this.params = new HashMap<String, String>();
        this.credentialsCharset = Consts.ASCII;
    }

    


    public RFC2617Scheme(final Charset credentialsCharset) {
        super();
        this.params = new HashMap<String, String>();
        this.credentialsCharset = credentialsCharset != null ? credentialsCharset : Consts.ASCII;
    }

    public RFC2617Scheme() {
        this(Consts.ASCII);
    }


    


    public Charset getCredentialsCharset() {
        return credentialsCharset;
    }

    String getCredentialsCharset(final HttpRequest request) {
        String charset = (String) request.getParams().getParameter(AuthPNames.CREDENTIAL_CHARSET);
        if (charset == null) {
            charset = getCredentialsCharset().name();
        }
        return charset;
    }

    @Override
    protected void parseChallenge(
            final CharArrayBuffer buffer, final int pos, final int len) throws MalformedChallengeException {
        final HeaderValueParser parser = BasicHeaderValueParser.INSTANCE;
        final ParserCursor cursor = new ParserCursor(pos, buffer.length());
        final HeaderElement[] elements = parser.parseElements(buffer, cursor);
        if (elements.length == 0) {
            throw new MalformedChallengeException("Authentication challenge is empty");
        }
        this.params.clear();
        for (final HeaderElement element : elements) {
            this.params.put(element.getName().toLowerCase(Locale.ENGLISH), element.getValue());
        }
    }

    




    protected Map<String, String> getParameters() {
        return this.params;
    }

    






    public String getParameter(final String name) {
        if (name == null) {
            return null;
        }
        return this.params.get(name.toLowerCase(Locale.ENGLISH));
    }

    




    public String getRealm() {
        return getParameter("realm");
    }

}
