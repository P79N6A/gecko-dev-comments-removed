



package org.mozilla.gecko.sync.setup.activities;

import org.mozilla.gecko.sync.setup.InvalidSyncKeyException;

public class ActivityUtils {
  









  public static String validateSyncKey(String key) throws InvalidSyncKeyException {
    String charKey = key.trim().replace("-", "").toLowerCase();
    if (!charKey.matches("^[abcdefghijkmnpqrstuvwxyz23456789]{26}$")) {
      throw new InvalidSyncKeyException();
    }
    return charKey;
  }
}
