



package org.mozilla.gecko.background.fxa;

import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Locale;

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

  









  public static boolean isMagicYear(int yearOfBirth) {
    final Calendar cal = Calendar.getInstance();
    final int thisYear = cal.get(Calendar.YEAR);
    return (thisYear - yearOfBirth) == FxAccountConstants.MINIMUM_AGE_TO_CREATE_AN_ACCOUNT;
  }

  










  public static boolean passesAgeCheck(final int dayOfBirth, final int zeroBasedMonthOfBirth, final int yearOfBirth) {
    final Calendar latestBirthday = Calendar.getInstance();
    final int y = latestBirthday.get(Calendar.YEAR);
    final int m = latestBirthday.get(Calendar.MONTH);
    final int d = latestBirthday.get(Calendar.DAY_OF_MONTH);
    latestBirthday.clear();
    latestBirthday.set(y - FxAccountConstants.MINIMUM_AGE_TO_CREATE_AN_ACCOUNT, m, d);

    
    latestBirthday.add(Calendar.SECOND, 1);

    final Calendar birthday = Calendar.getInstance();
    birthday.clear();
    birthday.set(yearOfBirth, zeroBasedMonthOfBirth, dayOfBirth);

    boolean oldEnough = birthday.before(latestBirthday);

    if (FxAccountUtils.LOG_PERSONAL_INFORMATION) {
      final StringBuilder message = new StringBuilder();
      final SimpleDateFormat sdf = new SimpleDateFormat("yyyy/MM/dd", Locale.getDefault());
      message.append("Age check ");
      message.append(oldEnough ? "passes" : "fails");
      message.append(": birthday is ");
      message.append(sdf.format(birthday.getTime()));
      message.append("; latest birthday is ");
      message.append(sdf.format(latestBirthday.getTime()));
      message.append(" (Y/M/D).");
      FxAccountUtils.pii(LOG_TAG, message.toString());
    }

    return oldEnough;
  }

  


  public static boolean passesAgeCheck(int dayOfBirth, int zeroBaseMonthOfBirth, String yearText, String[] yearItems) {
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

    return passesAgeCheck(dayOfBirth, zeroBaseMonthOfBirth, yearOfBirth);
  }
}
