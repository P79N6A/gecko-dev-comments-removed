




































package org.mozilla.gecko.sync.jpake;

import java.util.Random;

public class JPakeUtils {

  public static byte[] generateRandomBytes(int length) {
    byte[] bytes = new byte[length];
    Random random = new Random(System.nanoTime());
    random.nextBytes(bytes);
    return bytes;
  }
}
