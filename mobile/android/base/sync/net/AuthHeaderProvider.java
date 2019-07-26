



package org.mozilla.gecko.sync.net;

import java.security.GeneralSecurityException;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestBase;
import ch.boye.httpclientandroidlib.impl.client.DefaultHttpClient;
import ch.boye.httpclientandroidlib.protocol.BasicHttpContext;





public interface AuthHeaderProvider {
  








  Header getAuthHeader(HttpRequestBase request, BasicHttpContext context, DefaultHttpClient client)
    throws GeneralSecurityException;
}
