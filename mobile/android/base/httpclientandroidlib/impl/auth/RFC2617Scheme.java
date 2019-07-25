

























package ch.boye.httpclientandroidlib.impl.auth;

import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;

import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.auth.MalformedChallengeException;
import ch.boye.httpclientandroidlib.message.BasicHeaderValueParser;
import ch.boye.httpclientandroidlib.message.HeaderValueParser;
import ch.boye.httpclientandroidlib.message.ParserCursor;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;








@NotThreadSafe 
public abstract class RFC2617Scheme extends AuthSchemeBase {

    


    private Map<String, String> params;

    


    public RFC2617Scheme() {
        super();
    }

    @Override
    protected void parseChallenge(
            final CharArrayBuffer buffer, int pos, int len) throws MalformedChallengeException {
        HeaderValueParser parser = BasicHeaderValueParser.DEFAULT;
        ParserCursor cursor = new ParserCursor(pos, buffer.length());
        HeaderElement[] elements = parser.parseElements(buffer, cursor);
        if (elements.length == 0) {
            throw new MalformedChallengeException("Authentication challenge is empty");
        }

        this.params = new HashMap<String, String>(elements.length);
        for (HeaderElement element : elements) {
            this.params.put(element.getName(), element.getValue());
        }
    }

    




    protected Map<String, String> getParameters() {
        if (this.params == null) {
            this.params = new HashMap<String, String>();
        }
        return this.params;
    }

    






    public String getParameter(final String name) {
        if (name == null) {
            throw new IllegalArgumentException("Parameter name may not be null");
        }
        if (this.params == null) {
            return null;
        }
        return this.params.get(name.toLowerCase(Locale.ENGLISH));
    }

    




    public String getRealm() {
        return getParameter("realm");
    }

}
