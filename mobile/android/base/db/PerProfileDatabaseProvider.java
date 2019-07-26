



package org.mozilla.gecko.db;

import org.mozilla.gecko.db.PerProfileDatabases.DatabaseHelperFactory;

import android.content.Context;
import android.database.sqlite.SQLiteOpenHelper;
import android.os.Build;






public abstract class PerProfileDatabaseProvider<T extends SQLiteOpenHelper> extends AbstractPerProfileDatabaseProvider {
    private PerProfileDatabases<T> databases;

    @Override
    protected PerProfileDatabases<T> getDatabases() {
        return databases;
    }

    protected abstract String getDatabaseName();

    






    protected abstract T createDatabaseHelper(Context context, String databasePath);

    @Override
    public boolean onCreate() {
        synchronized (this) {
            databases = new PerProfileDatabases<T>(
                getContext(), getDatabaseName(), new DatabaseHelperFactory<T>() {
                    @Override
                    public T makeDatabaseHelper(Context context, String databasePath) {
                        final T helper = createDatabaseHelper(context, databasePath);
                        if (Build.VERSION.SDK_INT >= 16) {
                            helper.setWriteAheadLoggingEnabled(true);
                        }
                        return helper;
                    }
                });
        }

        return true;
    }
}
