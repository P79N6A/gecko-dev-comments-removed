




package org.mozilla.gecko.util;

import android.os.Handler;




public abstract class UiAsyncTask<Params, Progress, Result> {
    private volatile boolean mCancelled = false;
    private final Handler mBackgroundThreadHandler;
    private final Handler mUiHandler;

    public UiAsyncTask(Handler uiHandler, Handler backgroundThreadHandler) {
        mUiHandler = uiHandler;
        mBackgroundThreadHandler = backgroundThreadHandler;
    }

    private final class BackgroundTaskRunnable implements Runnable {
        private Params[] mParams;

        public BackgroundTaskRunnable(Params... params) {
            mParams = params;
        }

        public void run() {
            final Result result = doInBackground(mParams);

            mUiHandler.post(new Runnable() {
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
        mUiHandler.post(new Runnable() {
            public void run() {
                onPreExecute();
                mBackgroundThreadHandler.post(new BackgroundTaskRunnable(params));
            }
        });
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
