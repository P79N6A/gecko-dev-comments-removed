



package org.mozilla.gecko.fxa.tasks;

import java.util.Map;
import java.util.concurrent.CountDownLatch;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.fxa.FxAccountClient;
import org.mozilla.gecko.background.fxa.FxAccountClient10.RequestDelegate;
import org.mozilla.gecko.background.fxa.FxAccountClientException.FxAccountClientRemoteException;
import org.mozilla.gecko.fxa.tasks.FxAccountSetupTask.InnerRequestDelegate;

import android.content.Context;
import android.os.AsyncTask;










public abstract class FxAccountSetupTask<T> extends AsyncTask<Void, Void, InnerRequestDelegate<T>> {
  private static final String LOG_TAG = FxAccountSetupTask.class.getSimpleName();

  public interface ProgressDisplay {
    public void showProgress();
    public void dismissProgress();
  }

  protected final Context context;
  protected final FxAccountClient client;
  protected final ProgressDisplay progressDisplay;

  
  protected byte[] quickStretchedPW;

  
  protected final CountDownLatch latch = new CountDownLatch(1);
  protected final InnerRequestDelegate<T> innerDelegate = new InnerRequestDelegate<T>(latch);

  protected final Map<String, String> queryParameters;

  protected final RequestDelegate<T> delegate;

  public FxAccountSetupTask(Context context, ProgressDisplay progressDisplay, FxAccountClient client, Map<String, String> queryParameters, RequestDelegate<T> delegate) {
    this.context = context;
    this.client = client;
    this.delegate = delegate;
    this.progressDisplay = progressDisplay;
    this.queryParameters = queryParameters;
  }

  @Override
  protected void onPreExecute() {
    if (progressDisplay != null) {
      progressDisplay.showProgress();
    }
  }

  @Override
  protected void onPostExecute(InnerRequestDelegate<T> result) {
    if (progressDisplay != null) {
      progressDisplay.dismissProgress();
    }

    
    if (innerDelegate.failure != null) {
      delegate.handleFailure(innerDelegate.failure);
    } else if (innerDelegate.exception != null) {
      delegate.handleError(innerDelegate.exception);
    } else {
      delegate.handleSuccess(result.response);
    }
  }

  @Override
  protected void onCancelled(InnerRequestDelegate<T> result) {
    if (progressDisplay != null) {
      progressDisplay.dismissProgress();
    }
    delegate.handleError(new IllegalStateException("Task was cancelled."));
  }

  protected static class InnerRequestDelegate<T> implements RequestDelegate<T> {
    protected final CountDownLatch latch;
    public T response;
    public Exception exception;
    public FxAccountClientRemoteException failure;

    protected InnerRequestDelegate(CountDownLatch latch) {
      this.latch = latch;
    }

    @Override
    public void handleError(Exception e) {
      Logger.error(LOG_TAG, "Got exception.");
      this.exception = e;
      latch.countDown();
    }

    @Override
    public void handleFailure(FxAccountClientRemoteException e) {
      Logger.warn(LOG_TAG, "Got failure.");
      this.failure = e;
      latch.countDown();
    }

    @Override
    public void handleSuccess(T result) {
      Logger.info(LOG_TAG, "Got success.");
      this.response = result;
      latch.countDown();
    }
  }
}
