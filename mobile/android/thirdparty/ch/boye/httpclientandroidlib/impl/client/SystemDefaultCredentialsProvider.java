

























package ch.boye.httpclientandroidlib.impl.client;

import java.net.Authenticator;
import java.net.PasswordAuthentication;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.auth.AuthScope;
import ch.boye.httpclientandroidlib.auth.Credentials;
import ch.boye.httpclientandroidlib.auth.NTCredentials;
import ch.boye.httpclientandroidlib.auth.UsernamePasswordCredentials;
import ch.boye.httpclientandroidlib.client.CredentialsProvider;
import ch.boye.httpclientandroidlib.client.config.AuthSchemes;
import ch.boye.httpclientandroidlib.util.Args;







@ThreadSafe
public class SystemDefaultCredentialsProvider implements CredentialsProvider {

    private static final Map<String, String> SCHEME_MAP;

    static {
        SCHEME_MAP = new ConcurrentHashMap<String, String>();
        SCHEME_MAP.put(AuthSchemes.BASIC.toUpperCase(Locale.ENGLISH), "Basic");
        SCHEME_MAP.put(AuthSchemes.DIGEST.toUpperCase(Locale.ENGLISH), "Digest");
        SCHEME_MAP.put(AuthSchemes.NTLM.toUpperCase(Locale.ENGLISH), "NTLM");
        SCHEME_MAP.put(AuthSchemes.SPNEGO.toUpperCase(Locale.ENGLISH), "SPNEGO");
        SCHEME_MAP.put(AuthSchemes.KERBEROS.toUpperCase(Locale.ENGLISH), "Kerberos");
    }

    private static String translateScheme(final String key) {
        if (key == null) {
            return null;
        }
        final String s = SCHEME_MAP.get(key);
        return s != null ? s : key;
    }

    private final BasicCredentialsProvider internal;

    


    public SystemDefaultCredentialsProvider() {
        super();
        this.internal = new BasicCredentialsProvider();
    }

    public void setCredentials(final AuthScope authscope, final Credentials credentials) {
        internal.setCredentials(authscope, credentials);
    }

    private static PasswordAuthentication getSystemCreds(
            final AuthScope authscope,
            final Authenticator.RequestorType requestorType) {
        final String hostname = authscope.getHost();
        final int port = authscope.getPort();
        final String protocol = port == 443 ? "https" : "http";
        return Authenticator.requestPasswordAuthentication(
                hostname,
                null,
                port,
                protocol,
                null,
                translateScheme(authscope.getScheme()),
                null,
                requestorType);
    }

    public Credentials getCredentials(final AuthScope authscope) {
        Args.notNull(authscope, "Auth scope");
        final Credentials localcreds = internal.getCredentials(authscope);
        if (localcreds != null) {
            return localcreds;
        }
        if (authscope.getHost() != null) {
            PasswordAuthentication systemcreds = getSystemCreds(
                    authscope, Authenticator.RequestorType.SERVER);
            if (systemcreds == null) {
                systemcreds = getSystemCreds(
                        authscope, Authenticator.RequestorType.PROXY);
            }
            if (systemcreds != null) {
                final String domain = System.getProperty("http.auth.ntlm.domain");
                if (domain != null) {
                    return new NTCredentials(
                            systemcreds.getUserName(),
                            new String(systemcreds.getPassword()),
                            null, domain);
                } else {
                    if (AuthSchemes.NTLM.equalsIgnoreCase(authscope.getScheme())) {
                        
                        return new NTCredentials(
                                systemcreds.getUserName(),
                                new String(systemcreds.getPassword()),
                                null, null);
                    } else {
                        return new UsernamePasswordCredentials(
                                systemcreds.getUserName(),
                                new String(systemcreds.getPassword()));
                    }
                }
            }
        }
        return null;
    }

    public void clear() {
        internal.clear();
    }

}
