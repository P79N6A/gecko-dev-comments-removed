



package org.mozilla.search;

import android.content.AsyncQueryHandler;
import android.content.ContentValues;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.view.View;

import org.mozilla.gecko.db.BrowserContract.SearchHistory;
import org.mozilla.search.autocomplete.AcceptsSearchQuery;
import org.mozilla.search.autocomplete.SearchFragment;









public class MainActivity extends FragmentActivity implements AcceptsSearchQuery {

    enum State {
        START,
        PRESEARCH,
        POSTSEARCH
    }

    private State state = State.START;
    private AsyncQueryHandler queryHandler;

    @Override
    protected void onCreate(Bundle stateBundle) {
        super.onCreate(stateBundle);
        setContentView(R.layout.search_activity_main);

        queryHandler = new AsyncQueryHandler(getContentResolver()) {};
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        queryHandler = null;
    }

    @Override
    protected void onResume() {
        super.onResume();
        
        startPresearch();
    }

    @Override
    public void onSearch(String query) {
        startPostsearch();
        storeQuery(query);

        ((PostSearchFragment) getSupportFragmentManager().findFragmentById(R.id.postsearch))
                .startSearch(query);

        ((SearchFragment) getSupportFragmentManager().findFragmentById(R.id.search_fragment))
                .setSearchTerm(query);
    }

    private void startPresearch() {
        if (state != State.PRESEARCH) {
            state = State.PRESEARCH;
            findViewById(R.id.postsearch).setVisibility(View.INVISIBLE);
            findViewById(R.id.presearch).setVisibility(View.VISIBLE);
        }
    }

    private void startPostsearch() {
        if (state != State.POSTSEARCH) {
            state = State.POSTSEARCH;
            findViewById(R.id.presearch).setVisibility(View.INVISIBLE);
            findViewById(R.id.postsearch).setVisibility(View.VISIBLE);
        }
    }

    @Override
    public void onBackPressed() {
        if (state == State.POSTSEARCH) {
            startPresearch();
        } else {
            super.onBackPressed();
        }
    }

    


    private void storeQuery(String query) {
        final ContentValues cv = new ContentValues();
        cv.put(SearchHistory.QUERY, query);
        
        
        queryHandler.startInsert(0, null, SearchHistory.CONTENT_URI, cv);
    }
}
