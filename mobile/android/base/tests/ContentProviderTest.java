



package org.mozilla.gecko.tests;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.content.ContentProviderResult;
import android.content.ContentProviderOperation;
import android.content.OperationApplicationException;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.content.pm.ApplicationInfo;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.database.ContentObserver;
import android.os.Build;
import android.net.Uri;
import android.test.IsolatedContext;
import android.test.RenamingDelegatingContext;
import android.test.mock.MockContentResolver;
import android.test.mock.MockContext;

import java.io.File;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.concurrent.Callable;

import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.BrowserProvider;




















abstract class ContentProviderTest extends BaseTest {
    protected ContentProvider mProvider;
    protected ChangeRecordingMockContentResolver mResolver;
    protected ArrayList<Runnable> mTests;
    protected String mDatabaseName;
    protected String mProviderAuthority;
    protected IsolatedContext mProviderContext;

    private class ContentProviderMockContext extends MockContext {
        @Override
        public Resources getResources() {
            
            
            return ((Context)getActivity()).getResources();
        }

        @Override
        public String getPackageName() {
            return getInstrumentation().getContext().getPackageName();
        }

        @Override
        public String getPackageResourcePath() {
            return getInstrumentation().getContext().getPackageResourcePath();
        }

        @Override
        public File getDir(String name, int mode) {
            return getInstrumentation().getContext().getDir(this.getClass().getSimpleName() + "_" + name, mode);
        }

        @Override
        public Context getApplicationContext() {
            return this;
        }

        @Override
        public SharedPreferences getSharedPreferences(String name, int mode) {
            return getInstrumentation().getContext().getSharedPreferences(name, mode);
        }

        @Override
        public ApplicationInfo getApplicationInfo() {
            return getInstrumentation().getContext().getApplicationInfo();
        }
    }

    protected class DelegatingTestContentProvider extends ContentProvider {
        ContentProvider mTargetProvider;

        public DelegatingTestContentProvider(ContentProvider targetProvider) {
            super();
            mTargetProvider = targetProvider;
        }

        private Uri appendTestParam(Uri uri) {
            try {
                return appendUriParam(uri, BrowserContract.PARAM_IS_TEST, "1");
            } catch (Exception e) {}

            return null;
        }

        @Override
        public boolean onCreate() {
            return mTargetProvider.onCreate();
        }

        @Override
        public String getType(Uri uri) {
            return mTargetProvider.getType(uri);
        }

        @Override
        public int delete(Uri uri, String selection, String[] selectionArgs) {
            return mTargetProvider.delete(appendTestParam(uri), selection, selectionArgs);
        }

        @Override
        public Uri insert(Uri uri, ContentValues values) {
            return mTargetProvider.insert(appendTestParam(uri), values);
        }

        @Override
        public int update(Uri uri, ContentValues values, String selection,
                String[] selectionArgs) {
            return mTargetProvider.update(appendTestParam(uri), values,
                selection, selectionArgs);
        }

        @Override
        public Cursor query(Uri uri, String[] projection, String selection,
                String[] selectionArgs, String sortOrder) {
            return mTargetProvider.query(appendTestParam(uri), projection, selection,
                selectionArgs, sortOrder);
        }

        @Override
        public ContentProviderResult[] applyBatch (ArrayList<ContentProviderOperation> operations)
            throws OperationApplicationException {
            return mTargetProvider.applyBatch(operations);
        }

        @Override
        public int bulkInsert(Uri uri, ContentValues[] values) {
            return mTargetProvider.bulkInsert(appendTestParam(uri), values);
        }

        public ContentProvider getTargetProvider() {
            return mTargetProvider;
        }
    }

    




    protected class ChangeRecordingMockContentResolver extends MockContentResolver {
        public final LinkedList<Uri> notifyChangeList = new LinkedList<Uri>();

        @Override
        public void notifyChange(Uri uri, ContentObserver observer, boolean syncToNetwork) {
            notifyChangeList.addLast(uri);

            super.notifyChange(uri, observer, syncToNetwork);
        }
    }

    





    protected static Callable<ContentProvider> sBrowserProviderCallable = new Callable<ContentProvider>() {
        @Override
        public ContentProvider call() {
            return new BrowserProvider();
        }
    };

    private void setUpContentProvider(ContentProvider targetProvider) throws Exception {
        mResolver = new ChangeRecordingMockContentResolver();

        final String filenamePrefix = this.getClass().getSimpleName() + ".";
        RenamingDelegatingContext targetContextWrapper =
                new RenamingDelegatingContext(
                    new ContentProviderMockContext(),
                    (Context)getActivity(),
                    filenamePrefix);

        mProviderContext = new IsolatedContext(mResolver, targetContextWrapper);

        targetProvider.attachInfo(mProviderContext, null);

        mProvider = new DelegatingTestContentProvider(targetProvider);
        mProvider.attachInfo(mProviderContext, null);

        mResolver.addProvider(mProviderAuthority, mProvider);
    }

    public static Uri appendUriParam(Uri uri, String param, String value) {
        return uri.buildUpon().appendQueryParameter(param, value).build();
    }

    public void setTestName(String testName) {
        mAsserter.setTestName(this.getClass().getName() + " - " + testName);
    }

    @Override
    public void setUp() throws Exception {
        throw new UnsupportedOperationException("You should call setUp(authority, databaseName) instead");
    }

    public void setUp(Callable<ContentProvider> contentProviderFactory, String authority, String databaseName) throws Exception {
        super.setUp();

        mTests = new ArrayList<Runnable>();
        mDatabaseName = databaseName;

        mProviderAuthority = authority;

        setUpContentProvider(contentProviderFactory.call());
    }

    @Override
    public void tearDown() throws Exception {
        if (Build.VERSION.SDK_INT >= 11) {
            mProvider.shutdown();
        }

        if (mDatabaseName != null) {
            mProviderContext.deleteDatabase(mDatabaseName);
        }

        super.tearDown();
    }

    public AssetManager getAssetManager()  {
        return getInstrumentation().getContext().getAssets();
    }
}
