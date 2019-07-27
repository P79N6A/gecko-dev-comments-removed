



package org.mozilla.search;

import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;

import org.mozilla.search.autocomplete.AcceptsSearchQuery;
import org.mozilla.search.autocomplete.AutoCompleteFragment;
import org.mozilla.search.stream.CardStreamFragment;










public class MainActivity extends FragmentActivity implements AcceptsSearchQuery,
        FragmentManager.OnBackStackChangedListener {

    private DetailActivity detailActivity;

    @Override
    protected void onCreate(Bundle stateBundle) {
        super.onCreate(stateBundle);

        
        setContentView(R.layout.search_activity_main);

        
        FragmentManager localFragmentManager = getSupportFragmentManager();

        
        if (null == stateBundle) {

            
            FragmentTransaction localFragmentTransaction = localFragmentManager.beginTransaction();

            localFragmentTransaction.add(R.id.header_fragments, new AutoCompleteFragment(),
                    Constants.AUTO_COMPLETE_FRAGMENT);

            localFragmentTransaction.add(R.id.presearch_fragments, new CardStreamFragment(),
                    Constants.CARD_STREAM_FRAGMENT);

            
            localFragmentTransaction.commit();

            
        }
    }

    @Override
    protected void onStart() {
        super.onStart();


        if (null == detailActivity) {
            detailActivity = new DetailActivity();
        }

        if (null == getSupportFragmentManager().findFragmentByTag(Constants.GECKO_VIEW_FRAGMENT)) {
            FragmentTransaction txn = getSupportFragmentManager().beginTransaction();
            txn.add(R.id.gecko_fragments, detailActivity, Constants.GECKO_VIEW_FRAGMENT);
            txn.hide(detailActivity);

            txn.commit();
        }
    }

    @Override
    public void onSearch(String s) {
        FragmentManager localFragmentManager = getSupportFragmentManager();
        FragmentTransaction localFragmentTransaction = localFragmentManager.beginTransaction();

        localFragmentTransaction
                .hide(localFragmentManager.findFragmentByTag(Constants.CARD_STREAM_FRAGMENT))
                .addToBackStack(null);

        localFragmentTransaction
                .show(localFragmentManager.findFragmentByTag(Constants.GECKO_VIEW_FRAGMENT))
                .addToBackStack(null);

        localFragmentTransaction.commit();


        ((DetailActivity) getSupportFragmentManager()
                .findFragmentByTag(Constants.GECKO_VIEW_FRAGMENT))
                .setUrl("https://search.yahoo.com/search?p=" + Uri.encode(s));
    }

    @Override
    public void onBackStackChanged() {

    }
}
