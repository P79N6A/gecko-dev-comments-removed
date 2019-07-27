



package org.mozilla.gecko.sync;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.net.URLDecoder;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Locale;
import java.util.Map;
import java.util.TreeMap;

import org.json.simple.JSONArray;
import org.mozilla.apache.commons.codec.binary.Base32;
import org.mozilla.apache.commons.codec.binary.Base64;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.nativecode.NativeCrypto;
import org.mozilla.gecko.sync.setup.Constants;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.text.Spannable;
import android.text.Spanned;
import android.text.style.ClickableSpan;

public class Utils {

  private static final String LOG_TAG = "Utils";

  private static final SecureRandom sharedSecureRandom = new SecureRandom();

  
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

  


  public static String byte2Hex(final byte[] b) {
    return byte2Hex(b, 2 * b.length);
  }

  public static String byte2Hex(final byte[] b, int hexLength) {
    final StringBuilder hs = new StringBuilder(Math.max(2*b.length, hexLength));
    String stmp;

    for (int n = 0; n < hexLength - 2*b.length; n++) {
      hs.append("0");
    }

    for (int n = 0; n < b.length; n++) {
      stmp = Integer.toHexString(b[n] & 0XFF);

      if (stmp.length() == 1) {
        hs.append("0");
      }
      hs.append(stmp);
    }

    return hs.toString();
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
    return converter.decode(translated.toUpperCase(Locale.US));
  }

  public static byte[] hex2Byte(String str, int byteLength) {
    byte[] second = hex2Byte(str);
    if (second.length >= byteLength) {
      return second;
    }
    
    
    byte[] first = new byte[byteLength - second.length];
    return Utils.concatAll(first, second);
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
    return millisecondsToDecimalSeconds(ms).toString();
  }

  
  public static BigDecimal millisecondsToDecimalSeconds(long ms) {
    return new BigDecimal(ms).movePointLeft(3);
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
    return (decimal * 1000);
  }

  public static byte[] sha256(byte[] in)
      throws NoSuchAlgorithmException {
    MessageDigest sha1 = MessageDigest.getInstance("SHA-256");
    return sha1.digest(in);
  }

  protected static byte[] sha1(final String utf8)
      throws NoSuchAlgorithmException, UnsupportedEncodingException {
    final byte[] bytes = utf8.getBytes("UTF-8");
    try {
      return NativeCrypto.sha1(bytes);
    } catch (final LinkageError e) {
      
      
      
      Logger.warn(LOG_TAG, "Got throwable stretching password using native sha1 implementation; " +
          "ignoring and using Java implementation.", e);
      final MessageDigest sha1 = MessageDigest.getInstance("SHA-1");
      return sha1.digest(utf8.getBytes("UTF-8"));
    }
  }

  protected static String sha1Base32(final String utf8)
      throws NoSuchAlgorithmException, UnsupportedEncodingException {
    return new Base32().encodeAsString(sha1(utf8)).toLowerCase(Locale.US);
  }

  









  public static String usernameFromAccount(final String account) throws NoSuchAlgorithmException, UnsupportedEncodingException {
    if (account == null || account.equals("")) {
      throw new IllegalArgumentException("No account name provided.");
    }
    if (account.matches("^[A-Za-z0-9._-]+$")) {
      return account.toLowerCase(Locale.US);
    }
    return sha1Base32(account.toLowerCase(Locale.US));
  }

  public static SharedPreferences getSharedPreferences(final Context context, final String product, final String username, final String serverURL, final String profile, final long version)
      throws NoSuchAlgorithmException, UnsupportedEncodingException {
    String prefsPath = getPrefsPath(product, username, serverURL, profile, version);
    return context.getSharedPreferences(prefsPath, SHARED_PREFERENCES_MODE);
  }

  











  public static String getPrefsPath(final String product, final String username, final String serverURL, final String profile, final long version)
      throws NoSuchAlgorithmException, UnsupportedEncodingException {
    final String encodedAccount = sha1Base32(serverURL + ":" + usernameFromAccount(username));

    if (version <= 0) {
      return "sync.prefs." + encodedAccount;
    } else {
      final String sanitizedProduct = product.replace('.', '!').replace(' ', '!');
      return "sync.prefs." + sanitizedProduct + "." + encodedAccount + "." + profile + "." + version;
    }
  }

  public static void addToIndexBucketMap(TreeMap<Long, ArrayList<String>> map, long index, String value) {
    ArrayList<String> bucket = map.get(index);
    if (bucket == null) {
      bucket = new ArrayList<String>();
    }
    bucket.add(value);
    map.put(index, bucket);
  }

  


  private static boolean same(Object a, Object b) {
    if (a == b) {
      return true;
    }
    if (a == null || b == null) {
      return false;      
    }
    return a.equals(b);
  }

  



  public static boolean sameArrays(JSONArray a, JSONArray b) {
    if (a == b) {
      return true;
    }
    if (a == null || b == null) {
      return false;
    }
    final int size = a.size();
    if (size != b.size()) {
      return false;
    }
    for (int i = 0; i < size; ++i) {
      if (!same(a.get(i), b.get(i))) {
        return false;
      }
    }
    return true;
  }

  



  @SuppressWarnings("deprecation")
  public static Map<String, String> extractURIComponents(String scheme, String uri) {
    if (uri.indexOf(scheme) != 0) {
      throw new IllegalArgumentException("URI scheme does not match: " + scheme);
    }

    
    
    String components = uri.substring(scheme.length());
    HashMap<String, String> out = new HashMap<String, String>();
    String[] parts = components.split("&");
    for (int i = 0; i < parts.length; ++i) {
      String part = parts[i];
      if (part.length() == 0) {
        continue;
      }
      String[] pair = part.split("=", 2);
      switch (pair.length) {
      case 0:
        continue;
      case 1:
        out.put(URLDecoder.decode(pair[0]), null);
        break;
      case 2:
        out.put(URLDecoder.decode(pair[0]), URLDecoder.decode(pair[1]));
        break;
      }
    }
    return out;
  }

  
  public static String toDelimitedString(String delimiter, Collection<? extends Object> items) {
    if (items == null || items.size() == 0) {
      return "";
    }

    StringBuilder sb = new StringBuilder();
    int i = 0;
    int c = items.size();
    for (Object object : items) {
      sb.append(object.toString());
      if (++i < c) {
        sb.append(delimiter);
      }
    }
    return sb.toString();
  }

  public static String toCommaSeparatedString(Collection<? extends Object> items) {
    return toDelimitedString(", ", items);
  }

  







  public static Collection<String> getStagesToSync(final Collection<String> knownStageNames, Collection<String> toSync, Collection<String> toSkip) {
    if (toSkip == null) {
      toSkip = new HashSet<String>();
    } else {
      toSkip = new HashSet<String>(toSkip);
    }

    if (toSync == null) {
      toSync = new HashSet<String>(knownStageNames);
    } else {
      toSync = new HashSet<String>(toSync);
    }
    toSync.retainAll(knownStageNames);
    toSync.removeAll(toSkip);
    return toSync;
  }

  









  public static Collection<String> getStagesToSyncFromBundle(final Collection<String> knownStageNames, final Bundle extras) {
    if (extras == null) {
      return knownStageNames;
    }
    String toSyncString = extras.getString(Constants.EXTRAS_KEY_STAGES_TO_SYNC);
    String toSkipString = extras.getString(Constants.EXTRAS_KEY_STAGES_TO_SKIP);
    if (toSyncString == null && toSkipString == null) {
      return knownStageNames;
    }

    ArrayList<String> toSync = null;
    ArrayList<String> toSkip = null;
    if (toSyncString != null) {
      try {
        toSync = new ArrayList<String>(ExtendedJSONObject.parseJSONObject(toSyncString).keySet());
      } catch (Exception e) {
        Logger.warn(LOG_TAG, "Got exception parsing stages to sync: '" + toSyncString + "'.", e);
      }
    }
    if (toSkipString != null) {
      try {
        toSkip = new ArrayList<String>(ExtendedJSONObject.parseJSONObject(toSkipString).keySet());
      } catch (Exception e) {
        Logger.warn(LOG_TAG, "Got exception parsing stages to skip: '" + toSkipString + "'.", e);
      }
    }

    Logger.info(LOG_TAG, "Asked to sync '" + Utils.toCommaSeparatedString(toSync) +
                         "' and to skip '" + Utils.toCommaSeparatedString(toSkip) + "'.");
    return getStagesToSync(knownStageNames, toSync, toSkip);
  }

  











  public static void putStageNamesToSync(final Bundle bundle, final String[] stagesToSync, final String[] stagesToSkip) {
    if (bundle == null) {
      return;
    }

    if (stagesToSync != null) {
      ExtendedJSONObject o = new ExtendedJSONObject();
      for (String stageName : stagesToSync) {
        o.put(stageName, 0);
      }
      bundle.putString(Constants.EXTRAS_KEY_STAGES_TO_SYNC, o.toJSONString());
    }

    if (stagesToSkip != null) {
      ExtendedJSONObject o = new ExtendedJSONObject();
      for (String stageName : stagesToSkip) {
        o.put(stageName, 0);
      }
      bundle.putString(Constants.EXTRAS_KEY_STAGES_TO_SKIP, o.toJSONString());
    }
  }

  






  public static String readFile(final Context context, final String filename) {
    if (filename == null) {
      throw new IllegalArgumentException("Passed null filename in readFile.");
    }

    FileInputStream fis = null;
    InputStreamReader isr = null;
    BufferedReader br = null;

    try {
      fis = context.openFileInput(filename);
      isr = new InputStreamReader(fis);
      br = new BufferedReader(isr);
      StringBuilder sb = new StringBuilder();
      String line;
      while ((line = br.readLine()) != null) {
        sb.append(line);
      }
      return sb.toString();
    } catch (Exception e) {
      return null;
    } finally {
      if (isr != null) {
        try {
          isr.close();
        } catch (IOException e) {
          
        }
      }
      if (fis != null) {
        try {
          fis.close();
        } catch (IOException e) {
          
        }
      }
    }
  }

  






  public static String formatDuration(long startMillis, long endMillis) {
    final long duration = endMillis - startMillis;
    return new DecimalFormat("#0.00 seconds").format(((double) duration) / 1000);
  }

  






  public static String decodeUTF8(final String in) throws UnsupportedEncodingException {
    final int length = in.length();
    final byte[] asciiBytes = new byte[length];
    for (int i = 0; i < length; ++i) {
      asciiBytes[i] = (byte) in.codePointAt(i);
    }
    return new String(asciiBytes, "UTF-8");
  }

  


  public static String obfuscateEmail(final String in) {
    return in.replaceAll("[^@\\.]", "X");
  }

  public static String nodeWeaveURL(String serverURL, String username) {
    String userPart = username + "/node/weave";
    if (serverURL == null) {
      return SyncConstants.DEFAULT_AUTH_SERVER + "user/1.0/" + userPart;
    }
    if (!serverURL.endsWith("/")) {
      serverURL = serverURL + "/";
    }
    return serverURL + "user/1.0/" + userPart;
  }

  public static void throwIfNull(Object... objects) {
    for (Object object : objects) {
      if (object == null) {
        throw new IllegalArgumentException("object must not be null");
      }
    }
  }

  










  public static String getLanguageTag(final Locale locale) {
    
    

    String language = locale.getLanguage();  
    
    if (language.equals("iw")) {
      language = "he";
    } else if (language.equals("in")) {
      language = "id";
    } else if (language.equals("ji")) {
      language = "yi";
    }

    String country = locale.getCountry();    
    if (country.equals("")) {
      return language;
    }
    return language + "-" + country;
  }

  








  public static Spannable interpolateClickableSpan(Context context, int messageId, int clickableId, ClickableSpan clickableSpan) {
    
    
    
    final String clickablePart = context.getString(clickableId);
    final String message = context.getString(messageId, clickablePart);
    final int clickableStart = message.lastIndexOf(clickablePart);
    final int clickableEnd = clickableStart + clickablePart.length();
    final Spannable span = Spannable.Factory.getInstance().newSpannable(message);
    span.setSpan(clickableSpan, clickableStart, clickableEnd, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    return span;
  }
}
