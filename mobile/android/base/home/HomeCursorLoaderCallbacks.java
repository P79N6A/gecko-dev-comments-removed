




package org.mozilla.gecko.home;

import android.content.Context;
import android.database.Cursor;
import android.os.Bundle;
import android.support.v4.app.LoaderManager;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.Loader;




abstract class HomeCursorLoaderCallbacks implements LoaderCallbacks<Cursor> {

    
    private static final int LOADER_ID_FAVICONS = 100;

    private final Context mContext;
    private final LoaderManager mLoaderManager;

    public HomeCursorLoaderCallbacks(Context context, LoaderManager loaderManager) {
        mContext = context;
        mLoaderManager = loaderManager;
    }

    public void loadFavicons(Cursor cursor) {
        FaviconsLoader.restartFromCursor(mLoaderManager, LOADER_ID_FAVICONS, this, cursor);
    }

    @Override
    public Loader<Cursor> onCreateLoader(int id, Bundle args) {
        if (id == LOADER_ID_FAVICONS) {
            return FaviconsLoader.createInstance(mContext, args);
        }

        return null;
    }

    @Override
    public void onLoadFinished(Loader<Cursor> loader, Cursor c) {
        if (loader.getId() == LOADER_ID_FAVICONS) {
            onFaviconsLoaded();
        }
    }

    @Override
    public void onLoaderReset(Loader<Cursor> loader) {
        
    }

    
    public abstract void onFaviconsLoaded();
}
