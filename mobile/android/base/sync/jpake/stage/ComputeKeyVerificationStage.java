



package org.mozilla.gecko.sync.jpake.stage;

import java.io.UnsupportedEncodingException;

import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.Logger;
import org.mozilla.gecko.sync.crypto.CryptoException;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.jpake.JPakeClient;
import org.mozilla.gecko.sync.setup.Constants;

public class ComputeKeyVerificationStage extends JPakeStage {

  @Override
  public void execute(JPakeClient jClient) {
    Logger.debug(LOG_TAG, "Computing verification to send.");
    if (jClient.myKeyBundle == null) {
      Logger.error(LOG_TAG, "KeyBundle has not been set; aborting.");
      jClient.abort(Constants.JPAKE_ERROR_INTERNAL);
      return;
    }
    try {
      jClient.jOutgoing = computeKeyVerification(jClient.myKeyBundle, jClient.mySignerId);
    } catch (UnsupportedEncodingException e) {
      Logger.error(LOG_TAG, "Failure in key verification.", e);
      jClient.abort(Constants.JPAKE_ERROR_INVALID);
      return;
    } catch (CryptoException e) {
      Logger.error(LOG_TAG, "Encryption failure in key verification.", e);
      jClient.abort(Constants.JPAKE_ERROR_INTERNAL);
      return;
    }

    jClient.runNextStage();
  }

  





  public ExtendedJSONObject computeKeyVerification(KeyBundle keyBundle, String signerId)
      throws UnsupportedEncodingException, CryptoException
  {
    Logger.debug(LOG_TAG, "Encrypting key verification value.");
    ExtendedJSONObject jPayload = JPakeClient.encryptPayload(JPakeClient.JPAKE_VERIFY_VALUE, keyBundle, true);
    ExtendedJSONObject result = new ExtendedJSONObject();
    result.put(Constants.JSON_KEY_TYPE, signerId + "3");
    result.put(Constants.JSON_KEY_VERSION, JPakeClient.KEYEXCHANGE_VERSION);
    result.put(Constants.JSON_KEY_PAYLOAD, jPayload.object);
    return result;
  }

}
