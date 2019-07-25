



package org.mozilla.gecko.sync.jpake;

import java.math.BigInteger;

public class BigIntegerHelper {

  public static byte[] BigIntegerToByteArrayWithoutSign(BigInteger value) {
    byte[] bytes = value.toByteArray();
    if (bytes[0] == (byte) 0) {
      bytes = copyArray(bytes, 1, bytes.length - 1);
    }
    return bytes;
  }

  private static byte[] copyArray(byte[] original, int start, int length) {
    byte[] copy = new byte[length];
    System.arraycopy(original, start, copy, 0,
        Math.min(original.length - start, length));
    return copy;
  }

  


  public static BigInteger ByteArrayToBigIntegerWithoutSign(byte[] array) {
    return new BigInteger(1, array);
  }

  



  public static String toEvenLengthHex(BigInteger value) {
    String result = value.toString(16);
    if (result.length() % 2 != 0) {
      result = "0" + result;
    }
    return result;
  }
}
