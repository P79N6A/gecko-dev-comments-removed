



package org.mozilla.gecko.background.fxa;

import java.io.UnsupportedEncodingException;
import java.security.GeneralSecurityException;

import org.json.simple.JSONObject;
import org.mozilla.gecko.background.fxa.FxAccountClient10.CreateDelegate;
import org.mozilla.gecko.sync.Utils;





public class FxAccount20LoginDelegate implements CreateDelegate {
  protected final byte[] emailUTF8;
  protected final byte[] authPW;

  public FxAccount20LoginDelegate(byte[] emailUTF8, byte[] quickStretchedPW) throws UnsupportedEncodingException, GeneralSecurityException {
    this.emailUTF8 = emailUTF8;
    this.authPW = FxAccountUtils.generateAuthPW(quickStretchedPW);
  }

  @SuppressWarnings("unchecked")
  @Override
  public JSONObject getCreateBody() throws FxAccountClientException {
    final JSONObject body = new JSONObject();
    try {
      body.put("email", new String(emailUTF8, "UTF-8"));
      body.put("authPW", Utils.byte2Hex(authPW));
      return body;
    } catch (UnsupportedEncodingException e) {
      throw new FxAccountClientException(e);
    }
  }
}
