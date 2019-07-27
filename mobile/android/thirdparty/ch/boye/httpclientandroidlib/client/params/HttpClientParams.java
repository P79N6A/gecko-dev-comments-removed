

























package ch.boye.httpclientandroidlib.client.params;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.params.HttpConnectionParams;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.util.Args;








@Deprecated
@Immutable
public class HttpClientParams {

    private HttpClientParams() {
        super();
    }

    public static boolean isRedirecting(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        return params.getBooleanParameter
            (ClientPNames.HANDLE_REDIRECTS, true);
    }

    public static void setRedirecting(final HttpParams params, final boolean value) {
        Args.notNull(params, "HTTP parameters");
        params.setBooleanParameter
            (ClientPNames.HANDLE_REDIRECTS, value);
    }

    public static boolean isAuthenticating(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        return params.getBooleanParameter
            (ClientPNames.HANDLE_AUTHENTICATION, true);
    }

    public static void setAuthenticating(final HttpParams params, final boolean value) {
        Args.notNull(params, "HTTP parameters");
        params.setBooleanParameter
            (ClientPNames.HANDLE_AUTHENTICATION, value);
    }

    public static String getCookiePolicy(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        final String cookiePolicy = (String)
            params.getParameter(ClientPNames.COOKIE_POLICY);
        if (cookiePolicy == null) {
            return CookiePolicy.BEST_MATCH;
        }
        return cookiePolicy;
    }

    public static void setCookiePolicy(final HttpParams params, final String cookiePolicy) {
        Args.notNull(params, "HTTP parameters");
        params.setParameter(ClientPNames.COOKIE_POLICY, cookiePolicy);
    }

    




    public static void setConnectionManagerTimeout(final HttpParams params, final long timeout) {
        Args.notNull(params, "HTTP parameters");
        params.setLongParameter(ClientPNames.CONN_MANAGER_TIMEOUT, timeout);
    }

    








    public static long getConnectionManagerTimeout(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        final Long timeout = (Long) params.getParameter(ClientPNames.CONN_MANAGER_TIMEOUT);
        if (timeout != null) {
            return timeout.longValue();
        }
        return HttpConnectionParams.getConnectionTimeout(params);
    }

}
