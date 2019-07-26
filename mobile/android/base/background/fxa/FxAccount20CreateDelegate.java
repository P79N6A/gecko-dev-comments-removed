



package org.mozilla.gecko.background.fxa;

import java.io.UnsupportedEncodingException;
import java.security.GeneralSecurityException;

import org.json.simple.JSONObject;

public class FxAccount20CreateDelegate extends FxAccount20LoginDelegate {
  protected final boolean preVerified;

  












  public FxAccount20CreateDelegate(byte[] emailUTF8, byte[] passwordUTF8, boolean preVerified) throws UnsupportedEncodingException, GeneralSecurityException {
    super(emailUTF8, passwordUTF8);
    this.preVerified = preVerified;
  }

  @SuppressWarnings("unchecked")
  @Override
  public JSONObject getCreateBody() throws FxAccountClientException {
    final JSONObject body = super.getCreateBody();
    body.put("preVerified", preVerified);
    return body;
  }
}
