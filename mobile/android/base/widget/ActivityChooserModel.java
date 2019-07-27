



















package org.mozilla.gecko.widget;


import android.accounts.Account;
import android.content.pm.PackageManager;
import org.mozilla.gecko.distribution.Distribution;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.overlays.ui.ShareDialog;
import org.mozilla.gecko.sync.repositories.android.ClientsDatabaseAccessor;
import org.mozilla.gecko.sync.setup.SyncAccounts;
import org.mozilla.gecko.R;
import java.io.File;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ResolveInfo;
import android.database.DataSetObservable;
import android.os.AsyncTask;
import android.text.TextUtils;
import android.util.Log;
import android.util.Xml;






import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlSerializer;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.math.BigDecimal;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

















































public class ActivityChooserModel extends DataSetObservable {

    


    public interface ActivityChooserModelClient {

        




        public void setActivityChooserModel(ActivityChooserModel dataModel);
    }

    



    public interface ActivitySorter {

        







        
        
        public void sort(Intent intent, List<ActivityResolveInfo> activities,
                List<HistoricalRecord> historicalRecords);
    }

    


    public interface OnChooseActivityListener {

        















        public boolean onChooseActivity(ActivityChooserModel host, Intent intent);
    }

    


    private static final boolean DEBUG = false;

    


    static final String LOG_TAG = ActivityChooserModel.class.getSimpleName();

    


    private static final String TAG_HISTORICAL_RECORDS = "historical-records";

    


    private static final String TAG_HISTORICAL_RECORD = "historical-record";

    


    private static final String ATTRIBUTE_ACTIVITY = "activity";

    


    private static final String ATTRIBUTE_TIME = "time";

    


    private static final String ATTRIBUTE_WEIGHT = "weight";

    


    public static final int DEFAULT_HISTORY_MAX_LENGTH = 50;

    


    private static final int DEFAULT_ACTIVITY_INFLATION = 5;

    


    private static final float DEFAULT_HISTORICAL_RECORD_WEIGHT = 1.0f;

    


    private static final String HISTORY_FILE_EXTENSION = ".xml";

    


    private static final int INVALID_INDEX = -1;

    


    private static final Object sRegistryLock = new Object();

    


    private static final Map<String, ActivityChooserModel> sDataModelRegistry =
        new HashMap<String, ActivityChooserModel>();

    


    private final Object mInstanceLock = new Object();

    


    private final List<ActivityResolveInfo> mActivities = new ArrayList<ActivityResolveInfo>();

    


    private final List<HistoricalRecord> mHistoricalRecords = new ArrayList<HistoricalRecord>();

    


    


    private final DataModelPackageMonitor mPackageMonitor = new DataModelPackageMonitor();

    


    final Context mContext;

    


    final String mHistoryFileName;

    


    private Intent mIntent;

    


    private ActivitySorter mActivitySorter = new DefaultSorter();

    


    private int mHistoryMaxSize = DEFAULT_HISTORY_MAX_LENGTH;

    







    boolean mCanReadHistoricalData = true;

    








    private boolean mReadShareHistoryCalled;

    





    private boolean mHistoricalRecordsChanged = true;

    


    boolean mReloadActivities;

    


    private OnChooseActivityListener mActivityChooserModelPolicy;

    


    private final SyncStatusListener mSyncStatusListener = new SyncStatusListener();

    

















    public static ActivityChooserModel get(Context context, String historyFileName) {
        synchronized (sRegistryLock) {
            ActivityChooserModel dataModel = sDataModelRegistry.get(historyFileName);
            if (dataModel == null) {
                dataModel = new ActivityChooserModel(context, historyFileName);
                sDataModelRegistry.put(historyFileName, dataModel);
            }
            return dataModel;
        }
    }

    





    private ActivityChooserModel(Context context, String historyFileName) {
        mContext = context.getApplicationContext();
        if (!TextUtils.isEmpty(historyFileName)
                && !historyFileName.endsWith(HISTORY_FILE_EXTENSION)) {
            mHistoryFileName = historyFileName + HISTORY_FILE_EXTENSION;
        } else {
            mHistoryFileName = historyFileName;
        }

        


        mPackageMonitor.register(mContext);

        


        
        FirefoxAccounts.addSyncStatusListener(mSyncStatusListener);
    }

    








    public void setIntent(Intent intent) {
        synchronized (mInstanceLock) {
            if (mIntent == intent) {
                return;
            }
            mIntent = intent;
            mReloadActivities = true;
            ensureConsistentState();
        }
    }

    




    public Intent getIntent() {
        synchronized (mInstanceLock) {
            return mIntent;
        }
    }

    






    public int getActivityCount() {
        synchronized (mInstanceLock) {
            ensureConsistentState();
            return mActivities.size();
        }
    }

    







    public ResolveInfo getActivity(int index) {
        synchronized (mInstanceLock) {
            ensureConsistentState();
            return mActivities.get(index).resolveInfo;
        }
    }

    






    public int getActivityIndex(ResolveInfo activity) {
        synchronized (mInstanceLock) {
            ensureConsistentState();
            List<ActivityResolveInfo> activities = mActivities;
            final int activityCount = activities.size();
            for (int i = 0; i < activityCount; i++) {
                ActivityResolveInfo currentActivity = activities.get(i);
                if (currentActivity.resolveInfo == activity) {
                    return i;
                }
            }
            return INVALID_INDEX;
        }
    }

    

















    public Intent chooseActivity(int index) {
        synchronized (mInstanceLock) {
            if (mIntent == null) {
                return null;
            }

            ensureConsistentState();

            ActivityResolveInfo chosenActivity = mActivities.get(index);

            ComponentName chosenName = new ComponentName(
                    chosenActivity.resolveInfo.activityInfo.packageName,
                    chosenActivity.resolveInfo.activityInfo.name);

            Intent choiceIntent = new Intent(mIntent);
            choiceIntent.setComponent(chosenName);

            if (mActivityChooserModelPolicy != null) {
                
                Intent choiceIntentCopy = new Intent(choiceIntent);
                final boolean handled = mActivityChooserModelPolicy.onChooseActivity(this,
                        choiceIntentCopy);
                if (handled) {
                    return null;
                }
            }

            HistoricalRecord historicalRecord = new HistoricalRecord(chosenName,
                    System.currentTimeMillis(), DEFAULT_HISTORICAL_RECORD_WEIGHT);
            addHistoricalRecord(historicalRecord);

            return choiceIntent;
        }
    }

    




    public void setOnChooseActivityListener(OnChooseActivityListener listener) {
        synchronized (mInstanceLock) {
            mActivityChooserModelPolicy = listener;
        }
    }

    








    public ResolveInfo getDefaultActivity() {
        synchronized (mInstanceLock) {
            ensureConsistentState();
            if (!mActivities.isEmpty()) {
                return mActivities.get(0).resolveInfo;
            }
        }
        return null;
    }

    









    public void setDefaultActivity(int index) {
        synchronized (mInstanceLock) {
            ensureConsistentState();

            ActivityResolveInfo newDefaultActivity = mActivities.get(index);
            ActivityResolveInfo oldDefaultActivity = mActivities.get(0);

            final float weight;
            if (oldDefaultActivity != null) {
                
                weight = oldDefaultActivity.weight - newDefaultActivity.weight
                    + DEFAULT_ACTIVITY_INFLATION;
            } else {
                weight = DEFAULT_HISTORICAL_RECORD_WEIGHT;
            }

            ComponentName defaultName = new ComponentName(
                    newDefaultActivity.resolveInfo.activityInfo.packageName,
                    newDefaultActivity.resolveInfo.activityInfo.name);
            HistoricalRecord historicalRecord = new HistoricalRecord(defaultName,
                    System.currentTimeMillis(), weight);
            addHistoricalRecord(historicalRecord);
        }
    }

    








    private void persistHistoricalDataIfNeeded() {
        if (!mReadShareHistoryCalled) {
            throw new IllegalStateException("No preceding call to #readHistoricalData");
        }
        if (!mHistoricalRecordsChanged) {
            return;
        }
        mHistoricalRecordsChanged = false;
        if (!TextUtils.isEmpty(mHistoryFileName)) {
            


            new PersistHistoryAsyncTask().execute(new ArrayList<HistoricalRecord>(mHistoricalRecords), mHistoryFileName);
        }
    }

    






    public void setActivitySorter(ActivitySorter activitySorter) {
        synchronized (mInstanceLock) {
            if (mActivitySorter == activitySorter) {
                return;
            }
            mActivitySorter = activitySorter;
            if (sortActivitiesIfNeeded()) {
                notifyChanged();
            }
        }
    }

    












    public void setHistoryMaxSize(int historyMaxSize) {
        synchronized (mInstanceLock) {
            if (mHistoryMaxSize == historyMaxSize) {
                return;
            }
            mHistoryMaxSize = historyMaxSize;
            pruneExcessiveHistoricalRecordsIfNeeded();
            if (sortActivitiesIfNeeded()) {
                notifyChanged();
            }
        }
    }

    




    public int getHistoryMaxSize() {
        synchronized (mInstanceLock) {
            return mHistoryMaxSize;
        }
    }

    




    public int getHistorySize() {
        synchronized (mInstanceLock) {
            ensureConsistentState();
            return mHistoricalRecords.size();
        }
    }

    public int getDistinctActivityCountInHistory() {
        synchronized (mInstanceLock) {
            ensureConsistentState();
            final List<String> packages = new ArrayList<String>();
            for (HistoricalRecord record : mHistoricalRecords) {
              String activity = record.activity.flattenToString();
              if (!packages.contains(activity)) {
                packages.add(activity);
              }
            }
            return packages.size();
        }
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();

        


        mPackageMonitor.unregister();
        FirefoxAccounts.removeSyncStatusListener(mSyncStatusListener);
    }

    





    private void ensureConsistentState() {
        boolean stateChanged = loadActivitiesIfNeeded();
        stateChanged |= readHistoricalDataIfNeeded();
        pruneExcessiveHistoricalRecordsIfNeeded();
        if (stateChanged) {
            sortActivitiesIfNeeded();
            notifyChanged();
        }
    }

    






    private boolean sortActivitiesIfNeeded() {
        if (mActivitySorter != null && mIntent != null
                && !mActivities.isEmpty() && !mHistoricalRecords.isEmpty()) {
            mActivitySorter.sort(mIntent, mActivities,
                    Collections.unmodifiableList(mHistoricalRecords));
            return true;
        }
        return false;
    }

    





    private boolean loadActivitiesIfNeeded() {
        if (mReloadActivities && mIntent != null) {
            mReloadActivities = false;
            mActivities.clear();
            List<ResolveInfo> resolveInfos = mContext.getPackageManager()
                    .queryIntentActivities(mIntent, 0);
            final int resolveInfoCount = resolveInfos.size();

            


            final PackageManager packageManager = mContext.getPackageManager();
            final String channelToRemoveLabel = mContext.getResources().getString(R.string.overlay_share_label);
            final String shareDialogClassName = ShareDialog.class.getCanonicalName();

            for (int i = 0; i < resolveInfoCount; i++) {
                ResolveInfo resolveInfo = resolveInfos.get(i);

                








                if (shareDialogClassName.equals(resolveInfo.activityInfo.name) &&
                        channelToRemoveLabel.equals(resolveInfo.loadLabel(packageManager))) {
                    
                    if (!hasOtherSyncClients()) {
                        continue;
                    }

                    resolveInfo.labelRes = R.string.overlay_share_send_other;
                    resolveInfo.icon = R.drawable.icon_shareplane;
                }

                mActivities.add(new ActivityResolveInfo(resolveInfo));
            }
            return true;
        }
        return false;
    }

    






    private boolean readHistoricalDataIfNeeded() {
        if (mCanReadHistoricalData && mHistoricalRecordsChanged &&
                !TextUtils.isEmpty(mHistoryFileName)) {
            mCanReadHistoricalData = false;
            mReadShareHistoryCalled = true;
            readHistoricalDataImpl();
            return true;
        }
        return false;
    }

    





    private boolean addHistoricalRecord(HistoricalRecord historicalRecord) {
        final boolean added = mHistoricalRecords.add(historicalRecord);
        if (added) {
            mHistoricalRecordsChanged = true;
            pruneExcessiveHistoricalRecordsIfNeeded();
            persistHistoricalDataIfNeeded();
            sortActivitiesIfNeeded();
            notifyChanged();
        }
        return added;
    }

    





    boolean removeHistoricalRecordsForPackage(final String pkg) {
        boolean removed = false;

        for (Iterator<HistoricalRecord> i = mHistoricalRecords.iterator(); i.hasNext();) {
            final HistoricalRecord record = i.next();
            if (record.activity.getPackageName().equals(pkg)) {
                i.remove();
                removed = true;
            }
        }

        if (removed) {
            mHistoricalRecordsChanged = true;
            pruneExcessiveHistoricalRecordsIfNeeded();
            persistHistoricalDataIfNeeded();
            sortActivitiesIfNeeded();
            notifyChanged();
        }

        return removed;
    }

    


    private void pruneExcessiveHistoricalRecordsIfNeeded() {
        final int pruneCount = mHistoricalRecords.size() - mHistoryMaxSize;
        if (pruneCount <= 0) {
            return;
        }
        mHistoricalRecordsChanged = true;
        for (int i = 0; i < pruneCount; i++) {
            HistoricalRecord prunedRecord = mHistoricalRecords.remove(0);
            if (DEBUG) {
                Log.i(LOG_TAG, "Pruned: " + prunedRecord);
            }
        }
    }

    


    public final static class HistoricalRecord {

        


        public final ComponentName activity;

        


        public final long time;

        


        public final float weight;

        






        public HistoricalRecord(String activityName, long time, float weight) {
            this(ComponentName.unflattenFromString(activityName), time, weight);
        }

        






        public HistoricalRecord(ComponentName activityName, long time, float weight) {
            this.activity = activityName;
            this.time = time;
            this.weight = weight;
        }

        @Override
        public int hashCode() {
            final int prime = 31;
            int result = 1;
            result = prime * result + ((activity == null) ? 0 : activity.hashCode());
            result = prime * result + (int) (time ^ (time >>> 32));
            result = prime * result + Float.floatToIntBits(weight);
            return result;
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (obj == null) {
                return false;
            }
            if (getClass() != obj.getClass()) {
                return false;
            }
            HistoricalRecord other = (HistoricalRecord) obj;
            if (activity == null) {
                if (other.activity != null) {
                    return false;
                }
            } else if (!activity.equals(other.activity)) {
                return false;
            }
            if (time != other.time) {
                return false;
            }
            if (Float.floatToIntBits(weight) != Float.floatToIntBits(other.weight)) {
                return false;
            }
            return true;
        }

        @Override
        public String toString() {
            StringBuilder builder = new StringBuilder();
            builder.append("[");
            builder.append("; activity:").append(activity);
            builder.append("; time:").append(time);
            builder.append("; weight:").append(new BigDecimal(weight));
            builder.append("]");
            return builder.toString();
        }
    }

    


    public final class ActivityResolveInfo implements Comparable<ActivityResolveInfo> {

        


        public final ResolveInfo resolveInfo;

        


        public float weight;

        




        public ActivityResolveInfo(ResolveInfo resolveInfo) {
            this.resolveInfo = resolveInfo;
        }

        @Override
        public int hashCode() {
            return 31 + Float.floatToIntBits(weight);
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (obj == null) {
                return false;
            }
            if (getClass() != obj.getClass()) {
                return false;
            }
            ActivityResolveInfo other = (ActivityResolveInfo) obj;
            if (Float.floatToIntBits(weight) != Float.floatToIntBits(other.weight)) {
                return false;
            }
            return true;
        }

        @Override
        public int compareTo(ActivityResolveInfo another) {
             return  Float.floatToIntBits(another.weight) - Float.floatToIntBits(weight);
        }

        @Override
        public String toString() {
            StringBuilder builder = new StringBuilder();
            builder.append("[");
            builder.append("resolveInfo:").append(resolveInfo.toString());
            builder.append("; weight:").append(new BigDecimal(weight));
            builder.append("]");
            return builder.toString();
        }
    }

    


    private final class DefaultSorter implements ActivitySorter {
        private static final float WEIGHT_DECAY_COEFFICIENT = 0.95f;

        private final Map<String, ActivityResolveInfo> mPackageNameToActivityMap =
            new HashMap<String, ActivityResolveInfo>();

        @Override
        public void sort(Intent intent, List<ActivityResolveInfo> activities,
                List<HistoricalRecord> historicalRecords) {
            Map<String, ActivityResolveInfo> packageNameToActivityMap =
                mPackageNameToActivityMap;
            packageNameToActivityMap.clear();

            final int activityCount = activities.size();
            for (int i = 0; i < activityCount; i++) {
                ActivityResolveInfo activity = activities.get(i);
                activity.weight = 0.0f;

                
                ComponentName chosenName = new ComponentName(
                        activity.resolveInfo.activityInfo.packageName,
                        activity.resolveInfo.activityInfo.name);
                String packageName = chosenName.flattenToString();
                packageNameToActivityMap.put(packageName, activity);
            }

            final int lastShareIndex = historicalRecords.size() - 1;
            float nextRecordWeight = 1;
            for (int i = lastShareIndex; i >= 0; i--) {
                HistoricalRecord historicalRecord = historicalRecords.get(i);
                String packageName = historicalRecord.activity.flattenToString();
                ActivityResolveInfo activity = packageNameToActivityMap.get(packageName);
                if (activity != null) {
                    activity.weight += historicalRecord.weight * nextRecordWeight;
                    nextRecordWeight = nextRecordWeight * WEIGHT_DECAY_COEFFICIENT;
                }
            }

            Collections.sort(activities);

            if (DEBUG) {
                for (int i = 0; i < activityCount; i++) {
                    Log.i(LOG_TAG, "Sorted: " + activities.get(i));
                }
            }
        }
    }

    


    private void readHistoricalDataImpl() {
        try {
            GeckoProfile profile = GeckoProfile.get(mContext);
            File f = profile.getFile(mHistoryFileName);
            if (!f.exists()) {
                
                File oldFile = new File(mHistoryFileName);
                oldFile.renameTo(f);
            }
            readHistoricalDataFromStream(new FileInputStream(f));
        } catch (FileNotFoundException fnfe) {
            final Distribution dist = Distribution.getInstance(mContext);
            dist.addOnDistributionReadyCallback(new Distribution.ReadyCallback() {
                @Override
                public void distributionNotFound() {
                }

                @Override
                public void distributionFound(Distribution distribution) {
                    try {
                        File distFile = dist.getDistributionFile("quickshare/" + mHistoryFileName);
                        if (distFile == null) {
                            if (DEBUG) {
                                Log.i(LOG_TAG, "Could not open historical records file: " + mHistoryFileName);
                            }
                            return;
                        }
                        readHistoricalDataFromStream(new FileInputStream(distFile));
                    } catch (Exception ex) {
                        if (DEBUG) {
                            Log.i(LOG_TAG, "Could not open historical records file: " + mHistoryFileName);
                        }
                        return;
                    }
                }

                @Override
                public void distributionArrivedLate(Distribution distribution) {
                    distributionFound(distribution);
                }
            });
        }
    }

    void readHistoricalDataFromStream(FileInputStream fis) {
        try {
            XmlPullParser parser = Xml.newPullParser();
            parser.setInput(fis, null);

            int type = XmlPullParser.START_DOCUMENT;
            while (type != XmlPullParser.END_DOCUMENT && type != XmlPullParser.START_TAG) {
                type = parser.next();
            }

            if (!TAG_HISTORICAL_RECORDS.equals(parser.getName())) {
                throw new XmlPullParserException("Share records file does not start with "
                        + TAG_HISTORICAL_RECORDS + " tag.");
            }

            List<HistoricalRecord> historicalRecords = mHistoricalRecords;
            historicalRecords.clear();

            while (true) {
                type = parser.next();
                if (type == XmlPullParser.END_DOCUMENT) {
                    break;
                }
                if (type == XmlPullParser.END_TAG || type == XmlPullParser.TEXT) {
                    continue;
                }
                String nodeName = parser.getName();
                if (!TAG_HISTORICAL_RECORD.equals(nodeName)) {
                    throw new XmlPullParserException("Share records file not well-formed.");
                }

                String activity = parser.getAttributeValue(null, ATTRIBUTE_ACTIVITY);
                final long time =
                    Long.parseLong(parser.getAttributeValue(null, ATTRIBUTE_TIME));
                final float weight =
                    Float.parseFloat(parser.getAttributeValue(null, ATTRIBUTE_WEIGHT));
                 HistoricalRecord readRecord = new HistoricalRecord(activity, time, weight);
                historicalRecords.add(readRecord);

                if (DEBUG) {
                    Log.i(LOG_TAG, "Read " + readRecord.toString());
                }
            }

            if (DEBUG) {
                Log.i(LOG_TAG, "Read " + historicalRecords.size() + " historical records.");
            }
        } catch (XmlPullParserException | IOException xppe) {
            Log.e(LOG_TAG, "Error reading historical record file: " + mHistoryFileName, xppe);
        } finally {
            if (fis != null) {
                try {
                    fis.close();
                } catch (IOException ioe) {
                    
                }
            }
        }
    }

    


    private final class PersistHistoryAsyncTask extends AsyncTask<Object, Void, Void> {

        @Override
        @SuppressWarnings("unchecked")
        public Void doInBackground(Object... args) {
            List<HistoricalRecord> historicalRecords = (List<HistoricalRecord>) args[0];
            String historyFileName = (String) args[1];

            FileOutputStream fos = null;

            try {
                
                GeckoProfile profile = GeckoProfile.get(mContext);
                File file = profile.getFile(historyFileName);
                fos = new FileOutputStream(file);
            } catch (FileNotFoundException fnfe) {
                Log.e(LOG_TAG, "Error writing historical record file: " + historyFileName, fnfe);
                return null;
            }

            XmlSerializer serializer = Xml.newSerializer();

            try {
                serializer.setOutput(fos, null);
                serializer.startDocument("UTF-8", true);
                serializer.startTag(null, TAG_HISTORICAL_RECORDS);

                final int recordCount = historicalRecords.size();
                for (int i = 0; i < recordCount; i++) {
                    HistoricalRecord record = historicalRecords.remove(0);
                    serializer.startTag(null, TAG_HISTORICAL_RECORD);
                    serializer.attribute(null, ATTRIBUTE_ACTIVITY,
                            record.activity.flattenToString());
                    serializer.attribute(null, ATTRIBUTE_TIME, String.valueOf(record.time));
                    serializer.attribute(null, ATTRIBUTE_WEIGHT, String.valueOf(record.weight));
                    serializer.endTag(null, TAG_HISTORICAL_RECORD);
                    if (DEBUG) {
                        Log.i(LOG_TAG, "Wrote " + record.toString());
                    }
                }

                serializer.endTag(null, TAG_HISTORICAL_RECORDS);
                serializer.endDocument();

                if (DEBUG) {
                    Log.i(LOG_TAG, "Wrote " + recordCount + " historical records.");
                }
            } catch (IllegalArgumentException | IOException | IllegalStateException e) {
                Log.e(LOG_TAG, "Error writing historical record file: " + mHistoryFileName, e);
            } finally {
                mCanReadHistoricalData = true;
                if (fos != null) {
                    try {
                        fos.close();
                    } catch (IOException e) {
                        
                    }
                }
            }
            return null;
        }
    }

    


    


    private static final String LOGTAG = "GeckoActivityChooserModel";
    private final class DataModelPackageMonitor extends BroadcastReceiver {
        Context mContext;

        public DataModelPackageMonitor() { }

        public void register(Context context) {
            mContext = context;

            String[] intents = new String[] {
                Intent.ACTION_PACKAGE_REMOVED,
                Intent.ACTION_PACKAGE_ADDED,
                Intent.ACTION_PACKAGE_CHANGED
            };

            for (String intent : intents) {
                IntentFilter removeFilter = new IntentFilter(intent);
                removeFilter.addDataScheme("package");
                context.registerReceiver(this, removeFilter);
            }
        }

        public void unregister() {
            mContext.unregisterReceiver(this);
            mContext = null;
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (Intent.ACTION_PACKAGE_REMOVED.equals(action)) {
                String packageName = intent.getData().getSchemeSpecificPart();
                removeHistoricalRecordsForPackage(packageName);
            }

            mReloadActivities = true;
        }
    }

    


    private boolean hasOtherSyncClients() {
        
        
        if (!FirefoxAccounts.firefoxAccountsExist(mContext) &&
                !SyncAccounts.syncAccountsExist(mContext))  {
            return false;
        }

        final ClientsDatabaseAccessor db = new ClientsDatabaseAccessor(mContext);
        return db.clientsCount() > 0;
    }

    


    private class SyncStatusListener implements FirefoxAccounts.SyncStatusListener {
        @Override
        public Context getContext() {
            return mContext;
        }

        @Override
        public Account getAccount() {
            return FirefoxAccounts.getFirefoxAccount(getContext());
        }

        @Override
        public void onSyncStarted() {
        }

        @Override
        public void onSyncFinished() {
            
            
            synchronized (mInstanceLock) {
                mReloadActivities = true;
            }
        }
    }
}

