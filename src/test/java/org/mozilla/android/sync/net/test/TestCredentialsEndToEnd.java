


package org.mozilla.android.sync.net.test;

import static org.junit.Assert.assertEquals;

import java.io.IOException;
import java.io.UnsupportedEncodingException;

import org.json.simple.parser.ParseException;
import org.junit.Test;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.NonObjectJSONException;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.net.BaseResource;

import ch.boye.httpclientandroidlib.Header;





public class TestCredentialsEndToEnd {

  public static final String REAL_PASSWORD         = "pïgéons1";
  public static final String USERNAME              = "utvm3mk6hnngiir2sp4jsxf2uvoycrv6";
  public static final String DESKTOP_PASSWORD_JSON = "{\"password\":\"pÃ¯gÃ©ons1\"}";
  public static final String BTOA_PASSWORD         = "cMOvZ8Opb25zMQ==";
  public static final int    DESKTOP_ASSERTED_SIZE = 10;
  public static final String DESKTOP_BASIC_AUTH    = "Basic dXR2bTNtazZobm5naWlyMnNwNGpzeGYydXZveWNydjY6cMOvZ8Opb25zMQ==";

  private String getCreds(String password) {
    Header authenticate = BaseResource.getBasicAuthHeader(USERNAME + ":" + password);
    return authenticate.getValue();
  }

  @SuppressWarnings("static-method")
  @Test
  public void testUTF8() throws UnsupportedEncodingException {
    final String in  = "pÃ¯gÃ©ons1";
    final String out = "pïgéons1";
    assertEquals(out, Utils.decodeUTF8(in));
  }

  @Test
  public void testAuthHeaderFromPassword() throws NonObjectJSONException, IOException, ParseException {
    final ExtendedJSONObject parsed = new ExtendedJSONObject(DESKTOP_PASSWORD_JSON);

    final String password = parsed.getString("password");
    final String decoded = Utils.decodeUTF8(password);

    final byte[] expectedBytes = Utils.decodeBase64(BTOA_PASSWORD);
    final String expected = new String(expectedBytes, "UTF-8");

    assertEquals(DESKTOP_ASSERTED_SIZE, password.length());
    assertEquals(expected, decoded);

    System.out.println("Retrieved password: " + password);
    System.out.println("Expected password:  " + expected);
    System.out.println("Rescued password:   " + decoded);

    assertEquals(getCreds(expected), getCreds(decoded));
    assertEquals(getCreds(decoded), DESKTOP_BASIC_AUTH);
  }

  
  
  
}
