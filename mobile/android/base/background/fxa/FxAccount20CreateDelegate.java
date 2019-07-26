



package org.mozilla.gecko.background.fxa;

import java.io.UnsupportedEncodingException;
import java.security.GeneralSecurityException;

import org.json.simple.JSONObject;
import org.mozilla.gecko.background.fxa.FxAccountClient10.CreateDelegate;
import org.mozilla.gecko.sync.Utils;

public class FxAccount20CreateDelegate implements CreateDelegate {
  protected final byte[] emailUTF8;
  protected final byte[] authPW;
  protected final boolean preVerified;

  












  public FxAccount20CreateDelegate(byte[] emailUTF8, byte[] quickStretchedPW, boolean preVerified) throws UnsupportedEncodingException, GeneralSecurityException {
    this.emailUTF8 = emailUTF8;
    this.authPW = FxAccountUtils.generateAuthPW(quickStretchedPW);
    this.preVerified = preVerified;
  }

  @SuppressWarnings("unchecked")
  @Override
  public JSONObject getCreateBody() throws FxAccountClientException {
    final JSONObject body = new JSONObject();
    try {
      body.put("email", new String(emailUTF8, "UTF-8"));
      body.put("authPW", Utils.byte2Hex(authPW));
      if (preVerified) {
        
        
        body.put("preVerified", preVerified);
      }
      return body;
    } catch (UnsupportedEncodingException e) {
      throw new FxAccountClientException(e);
    }
  }
}
