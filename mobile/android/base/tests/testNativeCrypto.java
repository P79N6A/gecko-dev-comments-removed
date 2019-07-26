



package org.mozilla.gecko.tests;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.*;

import java.io.UnsupportedEncodingException;
import java.security.GeneralSecurityException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

import org.mozilla.gecko.background.nativecode.NativeCrypto;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.tests.helpers.*;

import android.os.SystemClock;









public class testNativeCrypto extends UITest {
  private final static String LOGTAG = "testNativeCrypto";

  







  public void test() throws Exception {
    
    
    
    
    
    GeckoHelper.blockForReady();

    _testPBKDF2SHA256A();
    _testPBKDF2SHA256B();
    _testPBKDF2SHA256C();
    _testPBKDF2SHA256scryptA();
    _testPBKDF2SHA256scryptB();
    _testPBKDF2SHA256InvalidLenArg();

    _testSHA1();
    _testSHA1AgainstMessageDigest();
  }

  public void _testPBKDF2SHA256A() throws UnsupportedEncodingException, GeneralSecurityException {
    final String  p = "password";
    final String  s = "salt";
    final int dkLen = 32;

    checkPBKDF2SHA256(p, s, 1, dkLen, "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b");
    checkPBKDF2SHA256(p, s, 4096, dkLen, "c5e478d59288c841aa530db6845c4c8d962893a001ce4e11a4963873aa98134a");
  }

  public void _testPBKDF2SHA256B() throws UnsupportedEncodingException, GeneralSecurityException {
    final String  p = "passwordPASSWORDpassword";
    final String  s = "saltSALTsaltSALTsaltSALTsaltSALTsalt";
    final int dkLen = 40;

    checkPBKDF2SHA256(p, s, 4096, dkLen, "348c89dbcbd32b2f32d814b8116e84cf2b17347ebc1800181c4e2a1fb8dd53e1c635518c7dac47e9");
  }

  public void _testPBKDF2SHA256scryptA() throws UnsupportedEncodingException, GeneralSecurityException {
    final String  p = "passwd";
    final String  s = "salt";
    final int dkLen = 64;

    checkPBKDF2SHA256(p, s, 1, dkLen, "55ac046e56e3089fec1691c22544b605f94185216dde0465e68b9d57c20dacbc49ca9cccf179b645991664b39d77ef317c71b845b1e30bd509112041d3a19783");
  }

  public void _testPBKDF2SHA256scryptB() throws UnsupportedEncodingException, GeneralSecurityException {
    final String  p = "Password";
    final String  s = "NaCl";
    final int dkLen = 64;

    checkPBKDF2SHA256(p, s, 80000, dkLen, "4ddcd8f60b98be21830cee5ef22701f9641a4418d04c0414aeff08876b34ab56a1d425a1225833549adb841b51c9b3176a272bdebba1d078478f62b397f33c8d");
  }

  public void _testPBKDF2SHA256C() throws UnsupportedEncodingException, GeneralSecurityException {
    final String  p = "pass\0word";
    final String  s = "sa\0lt";
    final int dkLen = 16;

    checkPBKDF2SHA256(p, s, 4096, dkLen, "89b69d0516f829893c696226650a8687");
  }

  public void _testPBKDF2SHA256InvalidLenArg() throws UnsupportedEncodingException, GeneralSecurityException {
    final String p = "password";
    final String s = "salt";
    final int c = 1;
    final int dkLen = -1; 

    try {
      final byte[] key = NativeCrypto.pbkdf2SHA256(p.getBytes("US-ASCII"), s.getBytes("US-ASCII"), c, dkLen);
      fail("Expected sha256 to throw with negative dkLen argument.");
    } catch (IllegalArgumentException e) { } 
  }

  private void _testSHA1() throws UnsupportedEncodingException {
    final String[] inputs = new String[] {
      "abc",
      "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
      "" 
    };
    final String baseStr = "01234567";
    final int repetitions = 80;
    final StringBuilder builder = new StringBuilder(baseStr.length() * repetitions);
    for (int i = 0; i < 80; ++i) {
      builder.append(baseStr);
    }
    inputs[2] = builder.toString();

    final String[] expecteds = new String[] {
      "a9993e364706816aba3e25717850c26c9cd0d89d",
      "84983e441c3bd26ebaae4aa1f95129e5e54670f1",
      "dea356a2cddd90c7a7ecedc5ebb563934f460452"
    };

    for (int i = 0; i < inputs.length; ++i) {
      final byte[] input = inputs[i].getBytes("US-ASCII");
      final String expected = expecteds[i];

      final byte[] actual = NativeCrypto.sha1(input);
      assertNotNull("Hashed value is non-null", actual);
      assertExpectedBytes(expected, actual);
    }
  }

  



  private void _testSHA1AgainstMessageDigest() throws UnsupportedEncodingException,
      NoSuchAlgorithmException {
    final String[] inputs = {
      "password",
      "saranghae",
      "aoeusnthaoeusnthaoeusnth \0 12345098765432109876_!"
    };

    final MessageDigest digest = MessageDigest.getInstance("SHA-1");
    for (final String input : inputs) {
      final byte[] inputBytes = input.getBytes("US-ASCII");

      final byte[] mdBytes = digest.digest(inputBytes);
      final byte[] ourBytes = NativeCrypto.sha1(inputBytes);
      assertArrayEquals("MessageDigest hash is the same as NativeCrypto SHA-1 hash", mdBytes, ourBytes);
    }
  }

  private void checkPBKDF2SHA256(String p, String s, int c, int dkLen, final String expectedStr)
      throws GeneralSecurityException, UnsupportedEncodingException {
    final long start = SystemClock.elapsedRealtime();

    final byte[] key = NativeCrypto.pbkdf2SHA256(p.getBytes("US-ASCII"), s.getBytes("US-ASCII"), c, dkLen);
    assertNotNull("Hash result is non-null", key);

    final long end = SystemClock.elapsedRealtime();
    dumpLog(LOGTAG, "SHA-256 " + c + " took " + (end - start) + "ms");

    if (expectedStr == null) {
      return;
    }

    assertEquals("Hash result is the appropriate length", dkLen,
        Utils.hex2Byte(expectedStr).length);
    assertExpectedBytes(expectedStr, key);
  }

  private void assertExpectedBytes(final String expectedStr, byte[] key) {
    assertEquals("Expected string matches hash result", expectedStr, Utils.byte2Hex(key));
    final byte[] expected = Utils.hex2Byte(expectedStr);

    assertEquals("Expected byte array length matches key length", expected.length, key.length);
    assertArrayEquals("Expected byte array matches key byte array", expected, key);
  }
}
