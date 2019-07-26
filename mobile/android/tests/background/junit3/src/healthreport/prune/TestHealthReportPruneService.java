


package org.mozilla.gecko.background.healthreport.prune;

import java.util.concurrent.BrokenBarrierException;

import org.mozilla.gecko.background.common.GlobalConstants;
import org.mozilla.gecko.background.helpers.BackgroundServiceTestCase;

import android.content.Intent;
import android.content.SharedPreferences;
import android.test.mock.MockContext;

public class TestHealthReportPruneService
    extends BackgroundServiceTestCase<TestHealthReportPruneService.MockHealthReportPruneService> {
  public static class MockHealthReportPruneService extends HealthReportPruneService {
    protected MockPrunePolicy prunePolicy;

    @Override
    protected SharedPreferences getSharedPreferences() {
      return this.getSharedPreferences(sharedPrefsName,
          GlobalConstants.SHARED_PREFERENCES_MODE);
    }

    @Override
    public void onHandleIntent(Intent intent) {
      super.onHandleIntent(intent);
      try {
        barrier.await();
      } catch (InterruptedException e) {
        fail("Awaiting thread should not be interrupted.");
      } catch (BrokenBarrierException e) {
        
      }
    }

    @Override
    public boolean isIntentValid(final Intent intent) {
      return super.isIntentValid(intent);
    }

    @Override
    public PrunePolicy getPrunePolicy(final String profilePath) {
      final PrunePolicyStorage storage = new PrunePolicyDatabaseStorage(new MockContext(), profilePath);
      prunePolicy = new MockPrunePolicy(storage, getSharedPreferences());
      return prunePolicy;
    }

    public boolean wasTickCalled() {
      if (prunePolicy == null) {
        return false;
      }
      return prunePolicy.wasTickCalled();
    }
  }

  
  public static class MockPrunePolicy extends PrunePolicy {
    private boolean wasTickCalled;

    public MockPrunePolicy(final PrunePolicyStorage storage, final SharedPreferences sharedPreferences) {
      super(storage, sharedPreferences);
      wasTickCalled = false;
    }

    @Override
    public void tick(final long time) {
      wasTickCalled = true;
    }

    public boolean wasTickCalled() {
      return wasTickCalled;
    }
  }

  public TestHealthReportPruneService() {
    super(MockHealthReportPruneService.class);
  }

  @Override
  public void setUp() throws Exception {
    super.setUp();
  }

  public void testIsIntentValid() throws Exception {
    
    startService(intent);
    await();
    assertFalse(getService().wasTickCalled());
    barrier.reset();

    
    intent.putExtra("profileName", "profileName");
    startService(intent);
    await();
    assertFalse(getService().wasTickCalled());
    barrier.reset();

    
    intent.putExtra("profilePath", "profilePath")
          .removeExtra("profileName");
    startService(intent);
    await();
    assertFalse(getService().wasTickCalled());
    barrier.reset();

    intent.putExtra("profileName", "profileName");
    startService(intent);
    await();
    assertTrue(getService().wasTickCalled());
  }
}
