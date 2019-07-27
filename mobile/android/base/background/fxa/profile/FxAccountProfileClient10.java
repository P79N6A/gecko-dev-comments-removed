



package org.mozilla.gecko.background.fxa.profile;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.concurrent.Executor;

import org.mozilla.gecko.background.fxa.oauth.FxAccountAbstractClient;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.net.AuthHeaderProvider;
import org.mozilla.gecko.sync.net.BaseResource;
import org.mozilla.gecko.sync.net.BearerAuthHeaderProvider;

import ch.boye.httpclientandroidlib.HttpResponse;







public class FxAccountProfileClient10 extends FxAccountAbstractClient {
  public FxAccountProfileClient10(String serverURI, Executor executor) {
    super(serverURI, executor);
  }

  public void profile(final String token, RequestDelegate<ExtendedJSONObject> delegate) {
    BaseResource resource;
    try {
      resource = new BaseResource(new URI(serverURI + "profile"));
    } catch (URISyntaxException e) {
      invokeHandleError(delegate, e);
      return;
    }

    resource.delegate = new ResourceDelegate<ExtendedJSONObject>(resource, delegate) {
      @Override
      public AuthHeaderProvider getAuthHeaderProvider() {
        return new BearerAuthHeaderProvider(token);
      }

      @Override
      public void handleSuccess(int status, HttpResponse response, ExtendedJSONObject body) {
        try {
          delegate.handleSuccess(body);
          return;
        } catch (Exception e) {
          delegate.handleError(e);
          return;
        }
      }
    };

    resource.get();
  }
}
