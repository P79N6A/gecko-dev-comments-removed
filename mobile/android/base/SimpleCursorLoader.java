


















package org.mozilla.gecko;

import android.content.Context;
import android.database.ContentObserver;
import android.database.Cursor;
import android.support.v4.content.AsyncTaskLoader;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.Arrays;

public abstract class SimpleCursorLoader extends AsyncTaskLoader<Cursor> {
    final ForceLoadContentObserver mObserver;
    Cursor mCursor;

    public SimpleCursorLoader(Context context) {
        super(context);
        mObserver = new ForceLoadContentObserver();
    }

    



    protected abstract Cursor loadCursor();

    
    @Override
    public Cursor loadInBackground() {
        Cursor cursor = loadCursor();

        if (cursor != null) {
            
            cursor.getCount();
            cursor.registerContentObserver(mObserver);
        }

        return cursor;
    }

    
    @Override
    public void deliverResult(Cursor cursor) {
        if (isReset()) {
            
            if (cursor != null) {
                cursor.close();
            }

            return;
        }

        Cursor oldCursor = mCursor;
        mCursor = cursor;

        if (isStarted()) {
            super.deliverResult(cursor);
        }

        if (oldCursor != null && oldCursor != cursor && !oldCursor.isClosed()) {
            oldCursor.close();
        }
    }

    






    @Override
    protected void onStartLoading() {
        if (mCursor != null) {
            deliverResult(mCursor);
        }

        if (takeContentChanged() || mCursor == null) {
            forceLoad();
        }
    }

    


    @Override
    protected void onStopLoading() {
        
        cancelLoad();
    }

    @Override
    public void onCanceled(Cursor cursor) {
        if (cursor != null && !cursor.isClosed()) {
            cursor.close();
        }
    }

    @Override
    protected void onReset() {
        super.onReset();

        
        onStopLoading();

        if (mCursor != null && !mCursor.isClosed()) {
            mCursor.close();
        }

        mCursor = null;
    }
}