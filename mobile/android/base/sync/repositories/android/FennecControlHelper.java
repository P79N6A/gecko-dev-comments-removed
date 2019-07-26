



package org.mozilla.gecko.sync.repositories.android;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.db.BrowserContract.Control;
import org.mozilla.gecko.sync.repositories.NoContentProviderException;

import android.content.ContentProviderClient;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;





public class FennecControlHelper {
  private static final String LOG_TAG = "FennecControlHelper";

  protected ContentProviderClient providerClient;
  protected final RepoUtils.QueryHelper queryHelper;

  public FennecControlHelper(Context context) throws NoContentProviderException {
    providerClient = acquireContentProvider(context);
    queryHelper = new RepoUtils.QueryHelper(context, Control.CONTENT_URI, LOG_TAG);
  }

  








  public static ContentProviderClient acquireContentProvider(final Context context)
      throws NoContentProviderException {
    final Uri uri = Control.CONTENT_URI;
    final ContentProviderClient client = context.getContentResolver().acquireContentProviderClient(uri);
    if (client == null) {
      throw new NoContentProviderException(uri);
    }
    return client;
  }

  


  public void releaseProviders() {
    try {
      if (providerClient != null) {
        providerClient.release();
      }
    } catch (Exception e) {
    }
    providerClient = null;
  }

  @Override
  protected void finalize() throws Throwable {
    this.releaseProviders();
    super.finalize();
  }

  private static String[] HISTORY_MIGRATION_COLUMNS = new String[] { Control.ENSURE_HISTORY_MIGRATED };
  private static String[] BOOKMARKS_MIGRATION_COLUMNS = new String[] { Control.ENSURE_BOOKMARKS_MIGRATED };

  







  protected boolean isColumnMigrated(final String[] columns) {
    try {
      final Cursor cursor = queryHelper.safeQuery(providerClient, ".isColumnMigrated(" + columns[0] + ")",
                                                  columns, null, null, null);
      try {
        if (!cursor.moveToFirst()) {
          return false;
        }

        
        return cursor.getInt(0) > 0;
      } finally {
        cursor.close();
      }
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Caught exception checking if Fennec has migrated column " + columns[0] + ".", e);
      return false;
    }
  }

  





  protected static boolean isColumnMigrated(Context context, String[] columns) {
    if (context == null) {
      return false;
    }

    try {
      final FennecControlHelper control = new FennecControlHelper(context);
      try {
        return control.isColumnMigrated(columns);
      } finally {
        control.releaseProviders();
      }
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Caught exception checking if Fennec has migrated column " + columns[0] + ".", e);
      return false;
    }
  }

  public static boolean isHistoryMigrated(Context context) {
    return isColumnMigrated(context, HISTORY_MIGRATION_COLUMNS);
  }

  public static boolean areBookmarksMigrated(Context context) {
    return isColumnMigrated(context, BOOKMARKS_MIGRATION_COLUMNS);
  }
}
