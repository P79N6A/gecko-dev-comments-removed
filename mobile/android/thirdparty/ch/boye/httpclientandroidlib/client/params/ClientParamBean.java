


























package ch.boye.httpclientandroidlib.client.params;

import java.util.Collection;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.params.HttpAbstractParamBean;
import ch.boye.httpclientandroidlib.params.HttpParams;










@Deprecated
@NotThreadSafe
public class ClientParamBean extends HttpAbstractParamBean {

    public ClientParamBean (final HttpParams params) {
        super(params);
    }

    


    @Deprecated
    public void setConnectionManagerFactoryClassName (final String factory) {
        params.setParameter(ClientPNames.CONNECTION_MANAGER_FACTORY_CLASS_NAME, factory);
    }

    public void setHandleRedirects (final boolean handle) {
        params.setBooleanParameter(ClientPNames.HANDLE_REDIRECTS, handle);
    }

    public void setRejectRelativeRedirect (final boolean reject) {
        params.setBooleanParameter(ClientPNames.REJECT_RELATIVE_REDIRECT, reject);
    }

    public void setMaxRedirects (final int maxRedirects) {
        params.setIntParameter(ClientPNames.MAX_REDIRECTS, maxRedirects);
    }

    public void setAllowCircularRedirects (final boolean allow) {
        params.setBooleanParameter(ClientPNames.ALLOW_CIRCULAR_REDIRECTS, allow);
    }

    public void setHandleAuthentication (final boolean handle) {
        params.setBooleanParameter(ClientPNames.HANDLE_AUTHENTICATION, handle);
    }

    public void setCookiePolicy (final String policy) {
        params.setParameter(ClientPNames.COOKIE_POLICY, policy);
    }

    public void setVirtualHost (final HttpHost host) {
        params.setParameter(ClientPNames.VIRTUAL_HOST, host);
    }

    public void setDefaultHeaders (final Collection <Header> headers) {
        params.setParameter(ClientPNames.DEFAULT_HEADERS, headers);
    }

    public void setDefaultHost (final HttpHost host) {
        params.setParameter(ClientPNames.DEFAULT_HOST, host);
    }

    


    public void setConnectionManagerTimeout(final long timeout) {
        params.setLongParameter(ClientPNames.CONN_MANAGER_TIMEOUT, timeout);
    }

}
