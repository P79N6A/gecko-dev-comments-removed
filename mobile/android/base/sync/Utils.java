





































package org.mozilla.gecko.sync;

import java.io.UnsupportedEncodingException;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.util.HashMap;

import org.mozilla.apache.commons.codec.binary.Base32;
import org.mozilla.apache.commons.codec.binary.Base64;
import org.mozilla.gecko.sync.crypto.Cryptographer;

import android.content.Context;
import android.content.SharedPreferences;

public class Utils {

  private static final String LOG_TAG = "Utils";

  private static SecureRandom sharedSecureRandom = new SecureRandom();

  
  public static final int SHARED_PREFERENCES_MODE = 0;

  public static String generateGuid() {
    byte[] encodedBytes = Base64.encodeBase64(generateRandomBytes(9), false);
    return new String(encodedBytes).replace("+", "-").replace("/", "_");
  }

  




  public static byte[] generateRandomBytes(int length) {
    byte[] bytes = new byte[length];
    sharedSecureRandom.nextBytes(bytes);
    return bytes;
  }

  




  public static BigInteger generateBigIntegerLessThan(BigInteger r) {
    int maxBytes = (int) Math.ceil(((double) r.bitLength()) / 8);
    BigInteger randInt = new BigInteger(generateRandomBytes(maxBytes));
    return randInt.mod(r);
  }

  


  public static void reseedSharedRandom() {
    sharedSecureRandom.setSeed(sharedSecureRandom.generateSeed(8));
  }

  




  public static String byte2hex(byte[] b) {
    
    String hs = "";
    String stmp;

    for (int n = 0; n < b.length; n++) {
      stmp = java.lang.Integer.toHexString(b[n] & 0XFF);

      if (stmp.length() == 1) {
        hs = hs + "0" + stmp;
      } else {
        hs = hs + stmp;
      }

      if (n < b.length - 1) {
        hs = hs + "";
      }
    }

    return hs;
  }

  




  public static byte[] concatAll(byte[] first, byte[]... rest) {
    int totalLength = first.length;
    for (byte[] array : rest) {
      totalLength += array.length;
    }

    byte[] result = new byte[totalLength];
    int offset = first.length;

    System.arraycopy(first, 0, result, 0, offset);

    for (byte[] array : rest) {
      System.arraycopy(array, 0, result, offset, array.length);
      offset += array.length;
    }
    return result;
  }

  










  public static byte[] decodeBase64(String base64) throws UnsupportedEncodingException {
    return Base64.decodeBase64(base64.getBytes("UTF-8"));
  }

  


  public static byte[] decodeFriendlyBase32(String base32) {
    Base32 converter = new Base32();
    final String translated = base32.replace('8', 'l').replace('9', 'o');
    return converter.decode(translated.toUpperCase());
  }

  




  public static byte[] hex2Byte(String str) {
    if (str.length() % 2 == 1) {
      str = "0" + str;
    }

    byte[] bytes = new byte[str.length() / 2];
    for (int i = 0; i < bytes.length; i++) {
      bytes[i] = (byte) Integer.parseInt(str.substring(2 * i, 2 * i + 2), 16);
    }
    return bytes;
  }

  public static String millisecondsToDecimalSecondsString(long ms) {
    return new BigDecimal(ms).movePointLeft(3).toString();
  }

  
  public static long decimalSecondsToMilliseconds(String decimal) {
    try {
      return new BigDecimal(decimal).movePointRight(3).longValue();
    } catch (Exception e) {
      return -1;
    }
  }

  
  public static long decimalSecondsToMilliseconds(Double decimal) {
    
    return (long)(decimal * 1000);
  }
  public static long decimalSecondsToMilliseconds(Long decimal) {
    return decimal * 1000;
  }
  public static long decimalSecondsToMilliseconds(Integer decimal) {
    return (long)(decimal * 1000);
  }


  public static String getPrefsPath(String username, String serverURL)
    throws NoSuchAlgorithmException, UnsupportedEncodingException {
    return "sync.prefs." + Cryptographer.sha1Base32(serverURL + ":" + username);
  }

  public static SharedPreferences getSharedPreferences(Context context, String username, String serverURL) throws NoSuchAlgorithmException, UnsupportedEncodingException {
    String prefsPath = getPrefsPath(username, serverURL);
    Logger.debug(LOG_TAG, "Shared preferences: " + prefsPath);
    return context.getSharedPreferences(prefsPath, SHARED_PREFERENCES_MODE);
  }

  







  public static void fillArraySpaces(String[] dest, HashMap<String, Long> source) throws Exception {
    int i = 0;
    int c = dest.length;
    int needed = source.size();
    if (needed == 0) {
      return;
    }
    if (needed > c) {
      throw new Exception("Need " + needed + " array spaces, have no more than " + c);
    }
    for (String key : source.keySet()) {
      while (i < c) {
        if (dest[i] == null) {
          
          dest[i] = key;
          source.put(key, (long) i);
          break;
        }
        ++i;
      }
    }
    if (i >= c) {
      throw new Exception("Could not fill array spaces.");
    }
  }
}
