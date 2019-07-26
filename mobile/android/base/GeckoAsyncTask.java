




package org.mozilla.gecko.util;

import android.app.Activity;
import android.os.Handler;




public abstract class GeckoAsyncTask<Params, Progress, Result> {
    public enum Priority { NORMAL, HIGH };

    private final Activity mActivity;
    private volatile boolean mCancelled = false;
    private final Handler mBackgroundThreadHandler;
    private Priority mPriority = Priority.NORMAL;

    public GeckoAsyncTask(Activity activity, Handler backgroundThreadHandler) {
        mActivity = activity;
        mBackgroundThreadHandler = backgroundThreadHandler;
    }

    private final class BackgroundTaskRunnable implements Runnable {
        private Params[] mParams;

        public BackgroundTaskRunnable(Params... params) {
            mParams = params;
        }

        public void run() {
            final Result result = doInBackground(mParams);

            mActivity.runOnUiThread(new Runnable() {
                public void run() {
                    if (mCancelled)
                        onCancelled();
                    else
                        onPostExecute(result);
                }
            });
        }
    }

    public final void execute(final Params... params) {
        mActivity.runOnUiThread(new Runnable() {
            public void run() {
                onPreExecute();

                BackgroundTaskRunnable runnable = new BackgroundTaskRunnable(params);
                if (mPriority == Priority.HIGH)
                    mBackgroundThreadHandler.postAtFrontOfQueue(runnable);
                else
                    mBackgroundThreadHandler.post(runnable);
            }
        });
    }

    public final GeckoAsyncTask<Params, Progress, Result> setPriority(Priority priority) {
        mPriority = priority;
        return this;
    }

    @SuppressWarnings({"UnusedParameters"})
    public final boolean cancel(boolean mayInterruptIfRunning) {
        mCancelled = true;
        return mCancelled;
    }

    public final boolean isCancelled() {
        return mCancelled;
    }

    protected void onPreExecute() { }
    protected void onPostExecute(Result result) { }
    protected void onCancelled() { }
    protected abstract Result doInBackground(Params... params);
}
