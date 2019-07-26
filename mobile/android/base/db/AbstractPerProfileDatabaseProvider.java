



package org.mozilla.gecko.db;

import org.mozilla.gecko.mozglue.RobocopTarget;

import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.net.Uri;








public abstract class AbstractPerProfileDatabaseProvider extends AbstractTransactionalProvider {

    



    protected abstract PerProfileDatabases<? extends SQLiteOpenHelper> getDatabases();

    







    @Override
    protected SQLiteDatabase getReadableDatabase(Uri uri) {
        String profile = null;
        if (uri != null) {
            profile = uri.getQueryParameter(BrowserContract.PARAM_PROFILE);
        }

        return getDatabases().getDatabaseHelperForProfile(profile, isTest(uri)).getReadableDatabase();
    }

    







    @Override
    protected SQLiteDatabase getWritableDatabase(Uri uri) {
        String profile = null;
        if (uri != null) {
            profile = uri.getQueryParameter(BrowserContract.PARAM_PROFILE);
        }

        return getDatabases().getDatabaseHelperForProfile(profile, isTest(uri)).getWritableDatabase();
    }

    protected SQLiteDatabase getWritableDatabaseForProfile(String profile, boolean isTest) {
        return getDatabases().getDatabaseHelperForProfile(profile, isTest).getWritableDatabase();
    }

    





    @Override
    @RobocopTarget
    public SQLiteDatabase getWritableDatabaseForTesting(Uri uri) {
        return getWritableDatabase(uri);
    }
}
