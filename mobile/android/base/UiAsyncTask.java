




package org.mozilla.gecko.util;

import android.os.Handler;
import android.os.Looper;








public abstract class UiAsyncTask<Params, Progress, Result> {
    private volatile boolean mCancelled = false;
    private final Handler mBackgroundThreadHandler;
    private static Handler sHandler;

    




    public UiAsyncTask(Handler backgroundThreadHandler) {
        mBackgroundThreadHandler = backgroundThreadHandler;
    }

    private static synchronized Handler getUiHandler() {
        if (sHandler == null) {
            sHandler = new Handler(Looper.getMainLooper());
        }
        return sHandler;
    }

    private final class BackgroundTaskRunnable implements Runnable {
        private Params[] mParams;

        public BackgroundTaskRunnable(Params... params) {
            mParams = params;
        }

        @Override
        public void run() {
            final Result result = doInBackground(mParams);

            getUiHandler().post(new Runnable() {
                @Override
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
        getUiHandler().post(new Runnable() {
            @Override
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
