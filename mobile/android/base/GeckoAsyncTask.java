




package org.mozilla.gecko.util;

import android.app.Activity;
import android.os.Handler;




public abstract class GeckoAsyncTask<Params, Progress, Result> {
    public enum Priority { NORMAL, HIGH };

    private final Activity mActivity;
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
                    onPostExecute(result);
                }
            });
        }
    }

    public final void execute(final Params... params) {
        BackgroundTaskRunnable runnable = new BackgroundTaskRunnable(params);
        if (mPriority == Priority.HIGH)
            mBackgroundThreadHandler.postAtFrontOfQueue(runnable);
        else
            mBackgroundThreadHandler.post(runnable);
    }

    public final GeckoAsyncTask<Params, Progress, Result> setPriority(Priority priority) {
        mPriority = priority;
        return this;
    }

    protected abstract Result doInBackground(Params... params);
    protected abstract void onPostExecute(Result result);
}
