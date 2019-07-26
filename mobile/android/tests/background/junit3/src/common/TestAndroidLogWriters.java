


package org.mozilla.gecko.background.common;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.common.log.writers.AndroidLevelCachingLogWriter;
import org.mozilla.gecko.background.common.log.writers.AndroidLogWriter;
import org.mozilla.gecko.background.common.log.writers.LogWriter;
import org.mozilla.gecko.background.helpers.AndroidSyncTestCase;

public class TestAndroidLogWriters extends AndroidSyncTestCase {
  public static final String TEST_LOG_TAG = "TestAndroidLogWriters";

  public static final String TEST_MESSAGE_1 = "LOG TEST MESSAGE one";
  public static final String TEST_MESSAGE_2 = "LOG TEST MESSAGE two";
  public static final String TEST_MESSAGE_3 = "LOG TEST MESSAGE three";

  public void setUp() {
    Logger.stopLoggingToAll();
  }

  public void tearDown() {
    Logger.resetLogging();
  }

  






  public void testAndroidLogWriter() {
    LogWriter lw = new AndroidLogWriter();

    Logger.error(TEST_LOG_TAG, TEST_MESSAGE_1, new RuntimeException());
    Logger.startLoggingTo(lw);
    Logger.error(TEST_LOG_TAG, TEST_MESSAGE_2);
    Logger.warn(TEST_LOG_TAG, TEST_MESSAGE_2);
    Logger.info(TEST_LOG_TAG, TEST_MESSAGE_2);
    Logger.debug(TEST_LOG_TAG, TEST_MESSAGE_2);
    Logger.trace(TEST_LOG_TAG, TEST_MESSAGE_2);
    Logger.stopLoggingTo(lw);
    Logger.error(TEST_LOG_TAG, TEST_MESSAGE_3, new RuntimeException());
  }

  






  public void testAndroidLevelCachingLogWriter() throws Exception {
    LogWriter lw = new AndroidLevelCachingLogWriter(new AndroidLogWriter());

    Logger.error(TEST_LOG_TAG, TEST_MESSAGE_1, new RuntimeException());
    Logger.startLoggingTo(lw);
    Logger.error(TEST_LOG_TAG, TEST_MESSAGE_2);
    Logger.warn(TEST_LOG_TAG, TEST_MESSAGE_2);
    Logger.info(TEST_LOG_TAG, TEST_MESSAGE_2);
    Logger.debug(TEST_LOG_TAG, TEST_MESSAGE_2);
    Logger.trace(TEST_LOG_TAG, TEST_MESSAGE_2);
    Logger.stopLoggingTo(lw);
    Logger.error(TEST_LOG_TAG, TEST_MESSAGE_3, new RuntimeException());
  }
}
