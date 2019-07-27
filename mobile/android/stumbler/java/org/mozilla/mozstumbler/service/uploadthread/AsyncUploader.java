



package org.mozilla.mozstumbler.service.uploadthread;

import android.os.AsyncTask;
import android.util.Log;
import java.io.IOException;
import java.util.concurrent.atomic.AtomicBoolean;

import org.mozilla.mozstumbler.service.Prefs;
import org.mozilla.mozstumbler.service.utils.AbstractCommunicator;
import org.mozilla.mozstumbler.service.utils.AbstractCommunicator.SyncSummary;
import org.mozilla.mozstumbler.service.AppGlobals;
import org.mozilla.mozstumbler.service.stumblerthread.datahandling.DataStorageManager;
import org.mozilla.mozstumbler.service.utils.NetworkUtils;








public class AsyncUploader extends AsyncTask<Void, Void, SyncSummary> {
    private static final String LOG_TAG = AppGlobals.makeLogTag(AsyncUploader.class.getSimpleName());
    private final AsyncUploadArgs mUploadArgs;
    private final Object mListenerLock = new Object();
    private AsyncUploaderListener mListener;
    private static final AtomicBoolean sIsUploading = new AtomicBoolean();
    private String mNickname;

    public interface AsyncUploaderListener {
        public void onUploadComplete(SyncSummary result);
        public void onUploadProgress();
    }

    public static class AsyncUploadArgs {
        public final NetworkUtils mNetworkUtils;
        public final boolean mShouldIgnoreWifiStatus;
        public final boolean mUseWifiOnly;
        public AsyncUploadArgs(NetworkUtils networkUtils,
                               boolean shouldIgnoreWifiStatus,
                               boolean useWifiOnly) {
            mNetworkUtils = networkUtils;
            mShouldIgnoreWifiStatus = shouldIgnoreWifiStatus;
            mUseWifiOnly = useWifiOnly;
        }
    }

    public AsyncUploader(AsyncUploadArgs args, AsyncUploaderListener listener) {
        mListener = listener;
        mUploadArgs = args;
    }

    public void setNickname(String name) {
        mNickname = name;
    }

    public void clearListener() {
        synchronized (mListenerLock) {
            mListener = null;
        }
    }

    public static boolean isUploading() {
        return sIsUploading.get();
    }

    @Override
    protected SyncSummary doInBackground(Void... voids) {
        if (sIsUploading.get()) {
            
            Log.d(LOG_TAG, "Usage error: check isUploading first, only one at a time task usage is permitted.");
            return null;
        }

        sIsUploading.set(true);
        SyncSummary result = new SyncSummary();
        Runnable progressListener = null;

        
        if (mListener != null) {
            progressListener = new Runnable() {
                @Override
                public void run() {
                    synchronized (mListenerLock) {
                        if (mListener != null) {
                            mListener.onUploadProgress();
                        }
                    }
                }
            };
        }

        uploadReports(result, progressListener);

        return result;
    }
    @Override
    protected void onPostExecute(SyncSummary result) {
        sIsUploading.set(false);

        synchronized (mListenerLock) {
            if (mListener != null) {
                mListener.onUploadComplete(result);
            }
        }
    }
    @Override
    protected void onCancelled(SyncSummary result) {
        sIsUploading.set(false);
    }

    private class Submitter extends AbstractCommunicator {
        private static final String SUBMIT_URL = "https://location.services.mozilla.com/v1/submit";

        @Override
        public String getUrlString() {
            return SUBMIT_URL;
        }

        @Override
        public String getNickname(){
            return mNickname;
        }

        @Override
        public NetworkSendResult cleanSend(byte[] data) {
            final NetworkSendResult result = new NetworkSendResult();
            try {
                result.bytesSent = this.send(data, ZippedState.eAlreadyZipped);
                result.errorCode = 0;
            } catch (IOException ex) {
                String msg = "Error submitting: " + ex;
                if (ex instanceof HttpErrorException) {
                    result.errorCode = ((HttpErrorException) ex).responseCode;
                    msg += " Code:" + result.errorCode;
                }
                Log.e(LOG_TAG, msg);
                AppGlobals.guiLogError(msg);
            }
            return result;
        }
    }

    private void uploadReports(AbstractCommunicator.SyncSummary syncResult, Runnable progressListener) {
        long uploadedObservations = 0;
        long uploadedCells = 0;
        long uploadedWifis = 0;

        if (!mUploadArgs.mShouldIgnoreWifiStatus && mUploadArgs.mUseWifiOnly &&
               mUploadArgs.mNetworkUtils.isWifiAvailable()) {
            if (AppGlobals.isDebug) {
                Log.d(LOG_TAG, "not on WiFi, not sending");
            }
            syncResult.numIoExceptions += 1;
            return;
        }

        Submitter submitter = new Submitter();
        DataStorageManager dm = DataStorageManager.getInstance();

        String error = null;

        try {
            DataStorageManager.ReportBatch batch = dm.getFirstBatch();
            while (batch != null) {
                AbstractCommunicator.NetworkSendResult result = submitter.cleanSend(batch.data);

                if (result.errorCode == 0) {
                    syncResult.totalBytesSent += result.bytesSent;

                    dm.delete(batch.filename);

                    uploadedObservations += batch.reportCount;
                    uploadedWifis += batch.wifiCount;
                    uploadedCells += batch.cellCount;
                } else {
                    if (result.errorCode / 100 == 4) {
                        
                        dm.delete(batch.filename);
                    } else {
                        DataStorageManager.getInstance().saveCurrentReportsSendBufferToDisk();
                    }
                    syncResult.numIoExceptions += 1;
                }

                if (progressListener != null) {
                    progressListener.run();
                }

                batch = dm.getNextBatch();
            }
        }
        catch (IOException ex) {
            error = ex.toString();
        }

        try {
            dm.incrementSyncStats(syncResult.totalBytesSent, uploadedObservations, uploadedCells, uploadedWifis);
        } catch (IOException ex) {
            error = ex.toString();
        } finally {
            if (error != null) {
                syncResult.numIoExceptions += 1;
                Log.d(LOG_TAG, error);
                AppGlobals.guiLogError(error + " (uploadReports)");
            }
            submitter.close();
        }
    }
}
