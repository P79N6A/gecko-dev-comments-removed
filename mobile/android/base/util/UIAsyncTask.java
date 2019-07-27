




package org.mozilla.gecko.util;

import android.os.Handler;
import android.os.Looper;





















public abstract class UIAsyncTask<Param, Result> {
    



    public static abstract class WithoutParams<InnerResult> extends UIAsyncTask<Void, InnerResult> {
        public WithoutParams(Handler backgroundThreadHandler) {
            super(backgroundThreadHandler);
        }

        public void execute() {
            execute(null);
        }

        @Override
        protected InnerResult doInBackground(Void unused) {
            return doInBackground();
        }

        protected abstract InnerResult doInBackground();
    }

    final Handler mBackgroundThreadHandler;
    private volatile boolean mCancelled;
    private static Handler sHandler;

    




    public UIAsyncTask(Handler backgroundThreadHandler) {
        mBackgroundThreadHandler = backgroundThreadHandler;
    }

    private static synchronized Handler getUiHandler() {
        if (sHandler == null) {
            sHandler = new Handler(Looper.getMainLooper());
        }

        return sHandler;
    }

    private final class BackgroundTaskRunnable implements Runnable {
        private Param mParam;

        public BackgroundTaskRunnable(Param param) {
            mParam = param;
        }

        @Override
        public void run() {
            final Result result = doInBackground(mParam);

            getUiHandler().post(new Runnable() {
                @Override
                public void run() {
                    if (mCancelled) {
                        onCancelled();
                    } else {
                        onPostExecute(result);
                    }
                }
            });
        }
    }

    protected void execute(final Param param) {
        getUiHandler().post(new Runnable() {
            @Override
            public void run() {
                onPreExecute();
                mBackgroundThreadHandler.post(new BackgroundTaskRunnable(param));
            }
        });
    }

    public final boolean cancel() {
        mCancelled = true;
        return mCancelled;
    }

    public final boolean isCancelled() {
        return mCancelled;
    }

    protected void onPreExecute() { }
    protected void onPostExecute(Result result) { }
    protected void onCancelled() { }
    protected abstract Result doInBackground(Param param);
}
