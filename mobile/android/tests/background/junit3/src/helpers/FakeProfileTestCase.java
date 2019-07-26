


package org.mozilla.gecko.background.helpers;

import java.io.File;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.test.ActivityInstrumentationTestCase2;
import java.util.UUID;

import org.mozilla.gecko.background.common.GlobalConstants;

import org.mozilla.gecko.background.common.TestUtils;

public abstract class FakeProfileTestCase extends ActivityInstrumentationTestCase2<Activity> {

  protected Context context;
  protected File fakeProfileDirectory;
  private String sharedPrefsName;

  public FakeProfileTestCase() {
    super(Activity.class);
  }

  



  protected String getCacheSuffix() {
    return this.getClass().getName() + "-" + System.currentTimeMillis();
  }

  @Override
  protected void setUp() throws Exception {
    super.setUp();
    context = getInstrumentation().getTargetContext();
    File cache = context.getCacheDir();
    fakeProfileDirectory = new File(cache.getAbsolutePath() + getCacheSuffix());
    if (fakeProfileDirectory.exists()) {
      TestUtils.deleteDirectoryRecursively(fakeProfileDirectory);
    }
    if (!fakeProfileDirectory.mkdir()) {
      throw new IllegalStateException("Could not create temporary directory.");
    }
    
    sharedPrefsName = this.getClass().getName() + "-" + UUID.randomUUID();
  }

  @Override
  protected void tearDown() throws Exception {
    TestUtils.deleteDirectoryRecursively(fakeProfileDirectory);
    super.tearDown();
  }

  public SharedPreferences getSharedPreferences() {
    return context.getSharedPreferences(sharedPrefsName, GlobalConstants.SHARED_PREFERENCES_MODE);
  }
}
