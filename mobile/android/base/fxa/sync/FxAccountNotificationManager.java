



package org.mozilla.gecko.fxa.sync;

import org.mozilla.gecko.BrowserLocaleManager;
import org.mozilla.gecko.R;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.common.telemetry.TelemetryWrapper;
import org.mozilla.gecko.background.fxa.FxAccountUtils;
import org.mozilla.gecko.fxa.activities.FxAccountFinishMigratingActivity;
import org.mozilla.gecko.fxa.activities.FxAccountStatusActivity;
import org.mozilla.gecko.fxa.authenticator.AndroidFxAccount;
import org.mozilla.gecko.fxa.login.State;
import org.mozilla.gecko.fxa.login.State.Action;
import org.mozilla.gecko.sync.telemetry.TelemetryContract;

import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.support.v4.app.NotificationCompat;
import android.support.v4.app.NotificationCompat.Builder;











public class FxAccountNotificationManager {
  private static final String LOG_TAG = FxAccountNotificationManager.class.getSimpleName();

  protected final int notificationId;

  
  private volatile boolean localeUpdated;

  public FxAccountNotificationManager(int notificationId) {
    this.notificationId = notificationId;
  }

  





  public void clear(Context context) {
    final NotificationManager notificationManager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
    notificationManager.cancel(notificationId);
  }

  








  public void update(Context context, AndroidFxAccount fxAccount) {
    final NotificationManager notificationManager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);

    final State state = fxAccount.getState();
    final Action action = state.getNeededAction();
    if (action == Action.None) {
      Logger.info(LOG_TAG, "State " + state.getStateLabel() + " needs no action; cancelling any existing notification.");
      notificationManager.cancel(notificationId);
      return;
    }

    if (!localeUpdated) {
      localeUpdated = true;
      BrowserLocaleManager.getInstance().getAndApplyPersistedLocale(context);
    }

    final String title;
    final String text;
    final Intent notificationIntent;
    if (action == Action.NeedsFinishMigrating) {
      TelemetryWrapper.addToHistogram(TelemetryContract.SYNC11_MIGRATION_NOTIFICATIONS_OFFERED, 1);

      title = context.getResources().getString(R.string.fxaccount_sync_finish_migrating_notification_title);
      text = context.getResources().getString(R.string.fxaccount_sync_finish_migrating_notification_text, state.email);
      notificationIntent = new Intent(context, FxAccountFinishMigratingActivity.class);
    } else {
      title = context.getResources().getString(R.string.fxaccount_sync_sign_in_error_notification_title);
      text = context.getResources().getString(R.string.fxaccount_sync_sign_in_error_notification_text, state.email);
      notificationIntent = new Intent(context, FxAccountStatusActivity.class);
    }
    Logger.info(LOG_TAG, "State " + state.getStateLabel() + " needs action; offering notification with title: " + title);
    FxAccountUtils.pii(LOG_TAG, "And text: " + text);

    final PendingIntent pendingIntent = PendingIntent.getActivity(context, 0, notificationIntent, 0);

    final Builder builder = new NotificationCompat.Builder(context);
    builder
    .setContentTitle(title)
    .setContentText(text)
    .setSmallIcon(R.drawable.ic_status_logo)
    .setAutoCancel(true)
    .setContentIntent(pendingIntent);
    notificationManager.notify(notificationId, builder.build());
  }
}
