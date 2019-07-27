



package org.mozilla.gecko.background.common.telemetry;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import org.mozilla.gecko.background.common.log.Logger;

















public class TelemetryWrapper {
  private static final String LOG_TAG = TelemetryWrapper.class.getSimpleName();

  
  private static volatile Method mAddToHistogram;

  public static void addToHistogram(String key, int value) {
    if (mAddToHistogram == null) {
      try {
        final Class<?> telemetry = Class.forName("org.mozilla.gecko.Telemetry");
        mAddToHistogram = telemetry.getMethod("addToHistogram", String.class, int.class);
      } catch (ClassNotFoundException e) {
        Logger.warn(LOG_TAG, "org.mozilla.gecko.Telemetry class found!");
        return;
      } catch (NoSuchMethodException e) {
        Logger.warn(LOG_TAG, "org.mozilla.gecko.Telemetry.addToHistogram(String, int) method not found!");
        return;
      }
    }

    if (mAddToHistogram != null) {
      try {
        mAddToHistogram.invoke(null, key, value);
      } catch (IllegalArgumentException | InvocationTargetException | IllegalAccessException e) {
        Logger.warn(LOG_TAG, "Got exception invoking telemetry!");
      }
    }
  }
}
