




package org.mozilla.gecko.home;

import java.util.HashMap;

import org.mozilla.gecko.R;
import org.mozilla.gecko.fxa.AccountLoader;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.fxa.FxAccountConstants;
import org.mozilla.gecko.fxa.login.State;
import org.mozilla.gecko.fxa.login.State.Action;
import org.mozilla.gecko.sync.SyncConstants;

import android.accounts.Account;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.Loader;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;










public class RemoteTabsPanel extends HomeFragment {
    private static final String LOGTAG = "GeckoRemoteTabsPanel";

    
    private static final int LOADER_ID_ACCOUNT = 0;

    
    private AccountLoaderCallbacks mAccountLoaderCallbacks;

    
    
    
    private Fragment mCurrentFragment;

    
    
    
    
    private final HashMap<Action, Fragment> mFragmentCache = new HashMap<Action, Fragment>();

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.home_remote_tabs_panel, container, false);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        
        mAccountLoaderCallbacks = new AccountLoaderCallbacks();
        loadIfVisible();
    }

    @Override
    public void load() {
        getLoaderManager().initLoader(LOADER_ID_ACCOUNT, null, mAccountLoaderCallbacks);
    }

    private void showSubPanel(Fragment subPanel) {
        if (mCurrentFragment == subPanel) {
            return;
        }
        mCurrentFragment = subPanel;

        Bundle args = subPanel.getArguments();
        if (args == null) {
            args = new Bundle();
        }
        args.putBoolean(HomePager.CAN_LOAD_ARG, getCanLoadHint());
        subPanel.setArguments(args);

        getChildFragmentManager()
            .beginTransaction()
            .addToBackStack(null)
            .replace(R.id.remote_tabs_container, subPanel)
            .commitAllowingStateLoss();
    }

    














    private Action getActionNeeded(Account account) {
        if (account == null) {
            return null;
        }

        if (SyncConstants.ACCOUNTTYPE_SYNC.equals(account.type)) {
            return Action.None;
        }

        if (!FxAccountConstants.ACCOUNT_TYPE.equals(account.type)) {
            Log.wtf(LOGTAG, "Non Sync, non Firefox Android Account returned by AccountLoader; returning null.");
            return null;
        }

        final State state = FirefoxAccounts.getFirefoxAccountState(getActivity());
        if (state == null) {
            Log.wtf(LOGTAG, "Firefox Account with null state found; offering needs password.");
            return Action.NeedsPassword;
        }

        final Action actionNeeded = state.getNeededAction();
        if (actionNeeded == null) {
            Log.wtf(LOGTAG, "Firefox Account with non-null state but null action needed; offering needs password.");
            return Action.NeedsPassword;
        }

        return actionNeeded;
    }

    private Fragment makeFragmentForAction(Action action) {
        if (action == null) {
            
            return RemoteTabsStaticFragment.newInstance(R.layout.remote_tabs_setup);
        }

        switch (action) {
        case None:
            return new RemoteTabsExpandableListFragment();
        case NeedsVerification:
            return RemoteTabsStaticFragment.newInstance(R.layout.remote_tabs_needs_verification);
        case NeedsPassword:
            return RemoteTabsStaticFragment.newInstance(R.layout.remote_tabs_needs_password);
        case NeedsUpgrade:
            return RemoteTabsStaticFragment.newInstance(R.layout.remote_tabs_needs_upgrade);
        default:
            
            
            
            Log.wtf(LOGTAG, "Got unexpected action needed; offering needs password.");
            return RemoteTabsStaticFragment.newInstance(R.layout.remote_tabs_needs_password);
        }
    }

    








    private Fragment getFragmentNeeded(Account account) {
        final Action actionNeeded = getActionNeeded(account);

        
        if (!mFragmentCache.containsKey(actionNeeded)) {
            final Fragment fragment = makeFragmentForAction(actionNeeded);
            mFragmentCache.put(actionNeeded, fragment);
        }
        return mFragmentCache.get(actionNeeded);
    }

    







    protected void updateUiFromAccount(Account account) {
        if (getView() == null) {
            
            
            
            
            
            
            return;
        }
        showSubPanel(getFragmentNeeded(account));
    }

    private class AccountLoaderCallbacks implements LoaderCallbacks<Account> {
        @Override
        public Loader<Account> onCreateLoader(int id, Bundle args) {
            return new AccountLoader(getActivity());
        }

        @Override
        public void onLoadFinished(Loader<Account> loader, Account account) {
            updateUiFromAccount(account);
        }

        @Override
        public void onLoaderReset(Loader<Account> loader) {
            updateUiFromAccount(null);
        }
    }
}
