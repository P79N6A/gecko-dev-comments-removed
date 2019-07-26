



package org.mozilla.gecko.background.healthreport;

import org.mozilla.gecko.background.healthreport.Environment.UIType;
import org.mozilla.gecko.background.healthreport.EnvironmentBuilder.ConfigurationProvider;
import org.mozilla.gecko.sync.jpake.stage.GetRequestStage.GetStepTimerTask;
import org.mozilla.gecko.util.HardwareUtils;

import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.util.DisplayMetrics;

public class AndroidConfigurationProvider implements ConfigurationProvider {
  private static final float MILLIMETERS_PER_INCH = 25.4f;

  private final Configuration configuration;
  private final DisplayMetrics displayMetrics;

  public AndroidConfigurationProvider(final Context context) {
    final Resources resources = context.getResources();
    this.configuration = resources.getConfiguration();
    this.displayMetrics = resources.getDisplayMetrics();

    HardwareUtils.init(context);
  }

  @Override
  public boolean hasHardwareKeyboard() {
    return configuration.keyboard != Configuration.KEYBOARD_NOKEYS;
  }

  @Override
  public UIType getUIType() {
    if (HardwareUtils.isLargeTablet()) {
      return UIType.LARGE_TABLET;
    }

    if (HardwareUtils.isSmallTablet()) {
      return UIType.SMALL_TABLET;
    }

    return UIType.DEFAULT;
  }

  @Override
  public int getUIModeType() {
    return configuration.uiMode & Configuration.UI_MODE_TYPE_MASK;
  }

  @Override
  public int getScreenLayoutSize() {
    return configuration.screenLayout & Configuration.SCREENLAYOUT_SIZE_MASK;
  }

  





  @Override
  public int getScreenXInMM() {
    return Math.round((displayMetrics.widthPixels / displayMetrics.xdpi) * MILLIMETERS_PER_INCH);
  }

  


  @Override
  public int getScreenYInMM() {
    return Math.round((displayMetrics.heightPixels / displayMetrics.ydpi) * MILLIMETERS_PER_INCH);
  }
}
