



package org.mozilla.gecko.sync.jpake;

import java.math.BigInteger;

import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.setup.Constants;

public class JPakeJson {
  


  public static ExtendedJSONObject makeJZkp(BigInteger gr, BigInteger b, String id) {
    ExtendedJSONObject result = new ExtendedJSONObject();
    result.put(Constants.ZKP_KEY_GR, BigIntegerHelper.toEvenLengthHex(gr));
    result.put(Constants.ZKP_KEY_B, BigIntegerHelper.toEvenLengthHex(b));
    result.put(Constants.ZKP_KEY_ID, id);
    return result;
  }
}
