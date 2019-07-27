




package org.mozilla.gecko.home;

import org.mozilla.gecko.GeckoScreenOrientation;
import org.mozilla.gecko.R;
import org.mozilla.gecko.fxa.AccountLoader;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.fxa.FxAccountConstants;
import org.mozilla.gecko.fxa.login.State;
import org.mozilla.gecko.fxa.login.State.Action;
import org.mozilla.gecko.sync.SyncConstants;
import org.mozilla.gecko.util.HardwareUtils;

import android.accounts.Account;
import android.content.res.Configuration;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.Loader;
import android.support.v4.util.Pair;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;










public class RemoteTabsPanel extends HomeFragment {
    private static final String LOGTAG = "GeckoRemoteTabsPanel";

    
    private static final int LOADER_ID_ACCOUNT = 0;
    private static final String FRAGMENT_ACTION = "FRAGMENT_ACTION";
    private static final String FRAGMENT_ORIENTATION = "FRAGMENT_ORIENTATION";
    private static final String FRAGMENT_TAG = "FRAGMENT_TAG";
    private static final String NO_ACCOUNT = "NO_ACCOUNT";

    
    private AccountLoaderCallbacks mAccountLoaderCallbacks;

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
    protected void loadIfVisible() {
        
        Pair<String, Integer> actionOrientationPair;
        if (canLoad() && HardwareUtils.isTablet() && (actionOrientationPair = getActionAndOrientationForFragmentInBackStack()) != null) {
            if (actionOrientationPair.first.equals(Action.None.name()) && actionOrientationPair.second != GeckoScreenOrientation.getInstance().getAndroidOrientation()) {
                
                
                
                getChildFragmentManager()
                        .beginTransaction()
                        .addToBackStack(null)
                        .remove(getChildFragmentManager().findFragmentByTag(FRAGMENT_TAG))
                        .commitAllowingStateLoss();
                getChildFragmentManager().executePendingTransactions();

                load();
                return;
            }
        }
        super.loadIfVisible();
    }

    @Override
    public void load() {
        getLoaderManager().initLoader(LOADER_ID_ACCOUNT, null, mAccountLoaderCallbacks);
    }

    private void showSubPanel(Account account) {
        final Action actionNeeded = getActionNeeded(account);
        final String actionString = actionNeeded != null ? actionNeeded.name() : NO_ACCOUNT;
        final int orientation = HardwareUtils.isTablet() ? GeckoScreenOrientation.getInstance().getAndroidOrientation()
                : Configuration.ORIENTATION_UNDEFINED;

        
        final Pair<String, Integer> actionOrientationPair = getActionAndOrientationForFragmentInBackStack();
        if (actionOrientationPair != null && actionOrientationPair.first.equals(actionString) && (actionOrientationPair.second == orientation)) {
            return;
        }

        
        Fragment subPanel = makeFragmentForAction(actionNeeded);
        final Bundle args = new Bundle();
        args.putBoolean(HomePager.CAN_LOAD_ARG, getCanLoadHint());
        args.putString(FRAGMENT_ACTION, actionString);
        args.putInt(FRAGMENT_ORIENTATION, orientation);
        subPanel.setArguments(args);

        
        getChildFragmentManager()
            .beginTransaction()
            .addToBackStack(null)
            .replace(R.id.remote_tabs_container, subPanel, FRAGMENT_TAG)
            .commitAllowingStateLoss();
    }

    private Pair<String, Integer> getActionAndOrientationForFragmentInBackStack() {
        final Fragment currentFragment = getChildFragmentManager().findFragmentByTag(FRAGMENT_TAG);
        if (currentFragment != null && currentFragment.getArguments() != null) {
            final String fragmentAction  = currentFragment.getArguments().getString(FRAGMENT_ACTION);
            final int fragmentOrientation = currentFragment.getArguments().getInt(FRAGMENT_ORIENTATION);
            return Pair.create(fragmentAction, fragmentOrientation);
        }
        return null;
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
            if (HardwareUtils.isTablet() && GeckoScreenOrientation.getInstance().getAndroidOrientation() == Configuration.ORIENTATION_LANDSCAPE) {
                return new RemoteTabsSplitPlaneFragment();
            } else {
                return new RemoteTabsExpandableListFragment();
            }
        case NeedsVerification:
            return RemoteTabsStaticFragment.newInstance(R.layout.remote_tabs_needs_verification);
        case NeedsPassword:
            return RemoteTabsStaticFragment.newInstance(R.layout.remote_tabs_needs_password);
        case NeedsUpgrade:
            return RemoteTabsStaticFragment.newInstance(R.layout.remote_tabs_needs_upgrade);
        case NeedsFinishMigrating:
            return RemoteTabsStaticFragment.newInstance(R.layout.remote_tabs_needs_finish_migrating);
        default:
            
            
            
            Log.wtf(LOGTAG, "Got unexpected action needed; offering needs password.");
            return RemoteTabsStaticFragment.newInstance(R.layout.remote_tabs_needs_password);
        }
    }

    







    protected void updateUiFromAccount(Account account) {
        if (getView() == null) {
            
            
            
            
            
            
            return;
        }
        showSubPanel(account);
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
