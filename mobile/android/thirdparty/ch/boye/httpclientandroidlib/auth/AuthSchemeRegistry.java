

























package ch.boye.httpclientandroidlib.auth;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.config.Lookup;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.ExecutionContext;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;









@ThreadSafe
@Deprecated
public final class AuthSchemeRegistry implements Lookup<AuthSchemeProvider> {

    private final ConcurrentHashMap<String,AuthSchemeFactory> registeredSchemes;

    public AuthSchemeRegistry() {
        super();
        this.registeredSchemes = new ConcurrentHashMap<String,AuthSchemeFactory>();
    }

    














    public void register(
            final String name,
            final AuthSchemeFactory factory) {
         Args.notNull(name, "Name");
        Args.notNull(factory, "Authentication scheme factory");
        registeredSchemes.put(name.toLowerCase(Locale.ENGLISH), factory);
    }

    





    public void unregister(final String name) {
         Args.notNull(name, "Name");
        registeredSchemes.remove(name.toLowerCase(Locale.ENGLISH));
    }

    










    public AuthScheme getAuthScheme(final String name, final HttpParams params)
        throws IllegalStateException {

        Args.notNull(name, "Name");
        final AuthSchemeFactory factory = registeredSchemes.get(name.toLowerCase(Locale.ENGLISH));
        if (factory != null) {
            return factory.newInstance(params);
        } else {
            throw new IllegalStateException("Unsupported authentication scheme: " + name);
        }
    }

    





    public List<String> getSchemeNames() {
        return new ArrayList<String>(registeredSchemes.keySet());
    }

    





    public void setItems(final Map<String, AuthSchemeFactory> map) {
        if (map == null) {
            return;
        }
        registeredSchemes.clear();
        registeredSchemes.putAll(map);
    }

    public AuthSchemeProvider lookup(final String name) {
        return new AuthSchemeProvider() {

            public AuthScheme create(final HttpContext context) {
                final HttpRequest request = (HttpRequest) context.getAttribute(
                        ExecutionContext.HTTP_REQUEST);
                return getAuthScheme(name, request.getParams());
            }

        };
    }

}
