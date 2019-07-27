


























package ch.boye.httpclientandroidlib.impl.client;

import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.FormattedHeader;
import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.auth.AuthScheme;
import ch.boye.httpclientandroidlib.auth.AuthSchemeRegistry;
import ch.boye.httpclientandroidlib.auth.AuthenticationException;
import ch.boye.httpclientandroidlib.auth.MalformedChallengeException;
import ch.boye.httpclientandroidlib.client.AuthenticationHandler;
import ch.boye.httpclientandroidlib.client.params.AuthPolicy;
import ch.boye.httpclientandroidlib.client.protocol.ClientContext;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Asserts;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;








@Deprecated
@Immutable
public abstract class AbstractAuthenticationHandler implements AuthenticationHandler {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    private static final List<String> DEFAULT_SCHEME_PRIORITY =
        Collections.unmodifiableList(Arrays.asList(new String[] {
                AuthPolicy.SPNEGO,
                AuthPolicy.NTLM,
                AuthPolicy.DIGEST,
                AuthPolicy.BASIC
    }));

    public AbstractAuthenticationHandler() {
        super();
    }

    protected Map<String, Header> parseChallenges(
            final Header[] headers) throws MalformedChallengeException {

        final Map<String, Header> map = new HashMap<String, Header>(headers.length);
        for (final Header header : headers) {
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
            map.put(s.toLowerCase(Locale.ENGLISH), header);
        }
        return map;
    }

    




    protected List<String> getAuthPreferences() {
        return DEFAULT_SCHEME_PRIORITY;
    }

    








    protected List<String> getAuthPreferences(
            final HttpResponse response,
            final HttpContext context) {
        return getAuthPreferences();
    }

    public AuthScheme selectScheme(
            final Map<String, Header> challenges,
            final HttpResponse response,
            final HttpContext context) throws AuthenticationException {

        final AuthSchemeRegistry registry = (AuthSchemeRegistry) context.getAttribute(
                ClientContext.AUTHSCHEME_REGISTRY);
        Asserts.notNull(registry, "AuthScheme registry");
        Collection<String> authPrefs = getAuthPreferences(response, context);
        if (authPrefs == null) {
            authPrefs = DEFAULT_SCHEME_PRIORITY;
        }

        if (this.log.isDebugEnabled()) {
            this.log.debug("Authentication schemes in the order of preference: "
                + authPrefs);
        }

        AuthScheme authScheme = null;
        for (final String id: authPrefs) {
            final Header challenge = challenges.get(id.toLowerCase(Locale.ENGLISH));

            if (challenge != null) {
                if (this.log.isDebugEnabled()) {
                    this.log.debug(id + " authentication scheme selected");
                }
                try {
                    authScheme = registry.getAuthScheme(id, response.getParams());
                    break;
                } catch (final IllegalStateException e) {
                    if (this.log.isWarnEnabled()) {
                        this.log.warn("Authentication scheme " + id + " not supported");
                        
                    }
                }
            } else {
                if (this.log.isDebugEnabled()) {
                    this.log.debug("Challenge for " + id + " authentication scheme not available");
                    
                }
            }
        }
        if (authScheme == null) {
            
            throw new AuthenticationException(
                "Unable to respond to any of these challenges: "
                    + challenges);
        }
        return authScheme;
    }

}
