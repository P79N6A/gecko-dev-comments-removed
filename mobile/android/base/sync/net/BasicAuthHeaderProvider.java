



package org.mozilla.gecko.sync.net;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.auth.Credentials;
import ch.boye.httpclientandroidlib.auth.UsernamePasswordCredentials;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestBase;
import ch.boye.httpclientandroidlib.impl.auth.BasicScheme;
import ch.boye.httpclientandroidlib.impl.client.DefaultHttpClient;
import ch.boye.httpclientandroidlib.protocol.BasicHttpContext;




public class BasicAuthHeaderProvider implements AuthHeaderProvider {
  protected final String credentials;

  




  public BasicAuthHeaderProvider(String credentials) {
    this.credentials = credentials;
  }

  





  public BasicAuthHeaderProvider(String user, String pass) {
    this(user + ":" + pass);
  }

  



  @Override
  public Header getAuthHeader(HttpRequestBase request, BasicHttpContext context, DefaultHttpClient client) {
    Credentials creds = new UsernamePasswordCredentials(credentials);

    
    return BasicScheme.authenticate(creds, "UTF-8", false);
  }
}
