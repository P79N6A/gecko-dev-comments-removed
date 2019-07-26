



package org.mozilla.gecko.fxa.activities;

import java.io.UnsupportedEncodingException;
import java.security.GeneralSecurityException;
import java.util.concurrent.CountDownLatch;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.fxa.FxAccountClient10.RequestDelegate;
import org.mozilla.gecko.background.fxa.FxAccountClient20;
import org.mozilla.gecko.background.fxa.FxAccountClient20.LoginResponse;
import org.mozilla.gecko.background.fxa.FxAccountClientException.FxAccountClientRemoteException;
import org.mozilla.gecko.background.fxa.FxAccountUtils;
import org.mozilla.gecko.fxa.activities.FxAccountSetupTask.InnerRequestDelegate;

import android.app.ProgressDialog;
import android.content.Context;
import android.os.AsyncTask;










abstract class FxAccountSetupTask<T> extends AsyncTask<Void, Void, InnerRequestDelegate<T>> {
  protected static final String LOG_TAG = FxAccountSetupTask.class.getSimpleName();

  protected final Context context;
  protected final FxAccountClient20 client;

  protected ProgressDialog progressDialog = null;

  
  protected byte[] quickStretchedPW;

  
  protected final CountDownLatch latch = new CountDownLatch(1);
  protected final InnerRequestDelegate<T> innerDelegate = new InnerRequestDelegate<T>(latch);

  protected final RequestDelegate<T> delegate;

  public FxAccountSetupTask(Context context, boolean shouldShowProgressDialog, FxAccountClient20 client, RequestDelegate<T> delegate) {
    this.context = context;
    this.client = client;
    this.delegate = delegate;
    if (shouldShowProgressDialog) {
      progressDialog = new ProgressDialog(context);
    }
  }

  @Override
  protected void onPreExecute() {
    if (progressDialog != null) {
      progressDialog.setTitle("Firefox Account..."); 
      progressDialog.setMessage("Please wait.");
      progressDialog.setCancelable(false);
      progressDialog.setIndeterminate(true);
      progressDialog.show();
    }
  }

  @Override
  protected void onPostExecute(InnerRequestDelegate<T> result) {
    if (progressDialog != null) {
      progressDialog.dismiss();
    }

    
    if (innerDelegate.exception != null) {
      delegate.handleError(innerDelegate.exception);
    } else {
      delegate.handleSuccess(result.response);
    }
  }

  @Override
  protected void onCancelled(InnerRequestDelegate<T> result) {
    if (progressDialog != null) {
      progressDialog.dismiss();
    }
    delegate.handleError(new IllegalStateException("Task was cancelled."));
  }

  protected static class InnerRequestDelegate<T> implements RequestDelegate<T> {
    protected final CountDownLatch latch;
    public T response = null;
    public Exception exception = null;

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
      this.exception = e;
      latch.countDown();
    }

    @Override
    public void handleSuccess(T result) {
      Logger.info(LOG_TAG, "Got success.");
      this.response = result;
      latch.countDown();
    }
  }

  public static class FxAccountCreateAccountTask extends FxAccountSetupTask<String> {
    protected static final String LOG_TAG = FxAccountCreateAccountTask.class.getSimpleName();

    protected final byte[] emailUTF8;
    protected final byte[] passwordUTF8;

    public FxAccountCreateAccountTask(Context context, String email, String password, FxAccountClient20 client, RequestDelegate<String> delegate) throws UnsupportedEncodingException {
      super(context, true, client, delegate);
      this.emailUTF8 = email.getBytes("UTF-8");
      this.passwordUTF8 = password.getBytes("UTF-8");
    }

    






    public byte[] generateQuickStretchedPW() throws UnsupportedEncodingException, GeneralSecurityException {
      if (this.quickStretchedPW == null) {
        this.quickStretchedPW = FxAccountUtils.generateQuickStretchedPW(emailUTF8, passwordUTF8);
      }
      return this.quickStretchedPW;
    }

    @Override
    protected InnerRequestDelegate<String> doInBackground(Void... arg0) {
      try {
        client.createAccount(emailUTF8, generateQuickStretchedPW(), false, innerDelegate);
        latch.await();
        return innerDelegate;
      } catch (Exception e) {
        Logger.error(LOG_TAG, "Got exception logging in.", e);
        delegate.handleError(e);
      }
      return null;
    }
  }

  public static class FxAccountSignInTask extends FxAccountSetupTask<LoginResponse> {
    protected static final String LOG_TAG = FxAccountCreateAccountTask.class.getSimpleName();

    protected final byte[] emailUTF8;
    protected final byte[] passwordUTF8;

    public FxAccountSignInTask(Context context, String email, String password, FxAccountClient20 client, RequestDelegate<LoginResponse> delegate) throws UnsupportedEncodingException {
      super(context, true, client, delegate);
      this.emailUTF8 = email.getBytes("UTF-8");
      this.passwordUTF8 = password.getBytes("UTF-8");
    }

    






    public byte[] generateQuickStretchedPW() throws UnsupportedEncodingException, GeneralSecurityException {
      if (this.quickStretchedPW == null) {
        this.quickStretchedPW = FxAccountUtils.generateQuickStretchedPW(emailUTF8, passwordUTF8);
      }
      return this.quickStretchedPW;
    }

    @Override
    protected InnerRequestDelegate<LoginResponse> doInBackground(Void... arg0) {
      try {
        client.loginAndGetKeys(emailUTF8, generateQuickStretchedPW(), innerDelegate);
        latch.await();
        return innerDelegate;
      } catch (Exception e) {
        Logger.error(LOG_TAG, "Got exception signing in.", e);
        delegate.handleError(e);
      }
      return null;
    }
  }
}
