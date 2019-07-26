


package org.mozilla.gecko.background.common;

import java.text.SimpleDateFormat;
import java.util.Locale;
import java.util.TimeZone;

import junit.framework.TestCase;

import org.mozilla.gecko.background.common.DateUtils.DateFormatter;


public class TestDateUtils extends TestCase {
  
  public static String getDateStringUsingFormatter(long time) {
    final SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd", Locale.US);
    format.setTimeZone(TimeZone.getTimeZone("UTC"));
    return format.format(time);
  }

  private void checkDateString(long time) {
    assertEquals(getDateStringUsingFormatter(time),
                 new DateUtils.DateFormatter().getDateString(time));
  }

  public void testDateImplementations() {
    checkDateString(1L);
    checkDateString(System.currentTimeMillis());
    checkDateString(1379118065844L);
    checkDateString(1379110000000L);
    for (long i = 0L; i < (2 * GlobalConstants.MILLISECONDS_PER_DAY); i += 11000) {
      checkDateString(i);
    }
  }

  @SuppressWarnings("static-method")
  public void testReuse() {
    DateFormatter formatter = new DateFormatter();
    long time = System.currentTimeMillis();
    assertEquals(formatter.getDateString(time), formatter.getDateString(time));
  }

  
  




































}
