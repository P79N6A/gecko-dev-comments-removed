



package org.mozilla.gecko.background.fxa;

import java.util.Arrays;
import java.util.Calendar;

import org.mozilla.gecko.fxa.FxAccountConstants;













public class FxAccountAgeLockoutHelper {
  private static final String LOG_TAG = FxAccountAgeLockoutHelper.class.getSimpleName();

  protected static long ELAPSED_REALTIME_OF_LAST_FAILED_AGE_CHECK = 0L;

  public static synchronized boolean isLockedOut(long elapsedRealtime) {
    if (ELAPSED_REALTIME_OF_LAST_FAILED_AGE_CHECK == 0L) {
      
      return false;
    }

    
    long millsecondsSinceLastFailedAgeCheck = elapsedRealtime - ELAPSED_REALTIME_OF_LAST_FAILED_AGE_CHECK;
    boolean isLockedOut = millsecondsSinceLastFailedAgeCheck < FxAccountConstants.MINIMUM_TIME_TO_WAIT_AFTER_AGE_CHECK_FAILED_IN_MILLISECONDS;
    FxAccountUtils.pii(LOG_TAG, "Checking if locked out: it's been " + millsecondsSinceLastFailedAgeCheck + "ms " +
        "since last lockout, so " + (isLockedOut ? "yes." : "no."));
    return isLockedOut;
  }

  public static synchronized void lockOut(long elapsedRealtime) {
      FxAccountUtils.pii(LOG_TAG, "Locking out at time: " + elapsedRealtime);
      ELAPSED_REALTIME_OF_LAST_FAILED_AGE_CHECK = Math.max(elapsedRealtime, ELAPSED_REALTIME_OF_LAST_FAILED_AGE_CHECK);
  }

  










  public static boolean passesAgeCheck(int yearOfBirth) {
    int thisYear = Calendar.getInstance().get(Calendar.YEAR);
    int approximateAge = thisYear - yearOfBirth;
    boolean oldEnough = approximateAge >= FxAccountConstants.MINIMUM_AGE_TO_CREATE_AN_ACCOUNT;
    if (FxAccountUtils.LOG_PERSONAL_INFORMATION) {
      FxAccountUtils.pii(LOG_TAG, "Age check " + (oldEnough ? "passes" : "fails") +
          ": age is " + approximateAge + " = " + thisYear + " - " + yearOfBirth);
    }
    return oldEnough;
  }

  


  public static boolean passesAgeCheck(String yearText, String[] yearItems) {
    if (yearText == null) {
      throw new IllegalArgumentException("yearText must not be null");
    }
    if (yearItems == null) {
      throw new IllegalArgumentException("yearItems must not be null");
    }
    if (!Arrays.asList(yearItems).contains(yearText)) {
      
      FxAccountUtils.pii(LOG_TAG, "Failed age check: year text was not found in item list.");
      return false;
    }
    Integer yearOfBirth;
    try {
      yearOfBirth = Integer.valueOf(yearText, 10);
    } catch (NumberFormatException e) {
      
      
      FxAccountUtils.pii(LOG_TAG, "Passed age check: year text was found in item list but was not a number.");
      return true;
    }
    return passesAgeCheck(yearOfBirth);
  }
}
