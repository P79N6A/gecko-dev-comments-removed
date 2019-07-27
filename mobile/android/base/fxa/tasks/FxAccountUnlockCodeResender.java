



package org.mozilla.gecko.fxa.tasks;

import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

import org.mozilla.gecko.R;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.fxa.FxAccountClient;
import org.mozilla.gecko.background.fxa.FxAccountClient10.RequestDelegate;
import org.mozilla.gecko.background.fxa.FxAccountClient20;
import org.mozilla.gecko.background.fxa.FxAccountClientException.FxAccountClientRemoteException;

import android.content.Context;
import android.os.AsyncTask;
import android.widget.Toast;





public class FxAccountUnlockCodeResender {
  private static final String LOG_TAG = FxAccountUnlockCodeResender.class.getSimpleName();

  private static class FxAccountUnlockCodeTask extends FxAccountSetupTask<Void> {
    protected static final String LOG_TAG = FxAccountUnlockCodeTask.class.getSimpleName();

    protected final byte[] emailUTF8;

    public FxAccountUnlockCodeTask(Context context, byte[] emailUTF8, FxAccountClient client, RequestDelegate<Void> delegate) {
      super(context, null, client, delegate);
      this.emailUTF8 = emailUTF8;
    }

    @Override
    protected InnerRequestDelegate<Void> doInBackground(Void... arg0) {
      try {
        client.resendUnlockCode(emailUTF8, innerDelegate);
        latch.await();
        return innerDelegate;
      } catch (Exception e) {
        Logger.error(LOG_TAG, "Got exception signing in.", e);
        delegate.handleError(e);
      }
      return null;
    }
  }

  private static class ResendUnlockCodeDelegate implements RequestDelegate<Void> {
    public final Context context;

    public ResendUnlockCodeDelegate(Context context) {
      this.context = context;
    }

    @Override
    public void handleError(Exception e) {
      Logger.warn(LOG_TAG, "Got exception requesting fresh unlock code; ignoring.", e);
      Toast.makeText(context, R.string.fxaccount_unlock_code_not_sent, Toast.LENGTH_LONG).show();
    }

    @Override
    public void handleFailure(FxAccountClientRemoteException e) {
      handleError(e);
    }

    @Override
    public void handleSuccess(Void result) {
      Toast.makeText(context, R.string.fxaccount_unlock_code_sent, Toast.LENGTH_SHORT).show();
    }
  }

  
















  public static void resendUnlockCode(Context context, String authServerURI, byte[] emailUTF8) {
    RequestDelegate<Void> delegate = new ResendUnlockCodeDelegate(context);

    if (emailUTF8 == null) {
      delegate.handleError(new IllegalArgumentException("emailUTF8 must not be null"));
      return;
    }

    final Executor executor = Executors.newSingleThreadExecutor();
    final FxAccountClient client = new FxAccountClient20(authServerURI, executor);
    new FxAccountUnlockCodeTask(context, emailUTF8, client, delegate).execute();
  }
}
