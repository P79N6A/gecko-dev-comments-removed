



package org.mozilla.gecko.background.fxa;

import org.mozilla.gecko.background.fxa.FxAccountClient10.RequestDelegate;
import org.mozilla.gecko.background.fxa.FxAccountClient10.StatusResponse;
import org.mozilla.gecko.background.fxa.FxAccountClient10.TwoKeys;
import org.mozilla.gecko.background.fxa.FxAccountClient20.LoginResponse;
import org.mozilla.gecko.sync.ExtendedJSONObject;

public interface FxAccountClient {
  public void loginAndGetKeys(final byte[] emailUTF8, final byte[] quickStretchedPW, final RequestDelegate<LoginResponse> requestDelegate);
  public void status(byte[] sessionToken, RequestDelegate<StatusResponse> requestDelegate);
  public void keys(byte[] keyFetchToken, RequestDelegate<TwoKeys> requestDelegate);
  public void sign(byte[] sessionToken, ExtendedJSONObject publicKey, long certificateDurationInMilliseconds, RequestDelegate<String> requestDelegate);
}
