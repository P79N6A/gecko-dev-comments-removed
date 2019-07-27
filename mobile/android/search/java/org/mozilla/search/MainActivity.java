



package org.mozilla.search;

import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.view.View;

import org.mozilla.search.autocomplete.AcceptsSearchQuery;









public class MainActivity extends FragmentActivity implements AcceptsSearchQuery {

    enum State {
        START,
        PRESEARCH,
        POSTSEARCH
    }

    private State state = State.START;

    @Override
    protected void onCreate(Bundle stateBundle) {
        super.onCreate(stateBundle);
        setContentView(R.layout.search_activity_main);
        startPresearch();
    }

    @Override
    public void onSearch(String s) {
        startPostsearch();
        ((PostSearchFragment) getSupportFragmentManager().findFragmentById(R.id.postsearch))
                .startSearch(s);
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
}
