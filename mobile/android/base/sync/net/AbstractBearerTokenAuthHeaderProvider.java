



package org.mozilla.gecko.sync.net;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestBase;
import ch.boye.httpclientandroidlib.impl.client.DefaultHttpClient;
import ch.boye.httpclientandroidlib.message.BasicHeader;
import ch.boye.httpclientandroidlib.protocol.BasicHttpContext;





public abstract class AbstractBearerTokenAuthHeaderProvider implements AuthHeaderProvider {
  protected final String header;

  public AbstractBearerTokenAuthHeaderProvider(String token) {
    if (token == null) {
      throw new IllegalArgumentException("token must not be null.");
    }

    this.header = getPrefix() + " " + token;
  }

  protected abstract String getPrefix();

  @Override
  public Header getAuthHeader(HttpRequestBase request, BasicHttpContext context, DefaultHttpClient client) {
    return new BasicHeader("Authorization", header);
  }
}
