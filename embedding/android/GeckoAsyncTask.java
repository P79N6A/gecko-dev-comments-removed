




































package org.mozilla.gecko;




public abstract class GeckoAsyncTask<Params, Progress, Result> {
    public void execute(final Params... params) {
        GeckoAppShell.getHandler().post(new Runnable() {
            public void run() {
                final Result result = doInBackground(params);
                GeckoApp.mAppContext.runOnUiThread(new Runnable() {
                    public void run() {
                        onPostExecute(result);
                    }});
            }});
    }

    protected abstract Result doInBackground(Params... params);
    protected abstract void  onPostExecute(Result result);
}
