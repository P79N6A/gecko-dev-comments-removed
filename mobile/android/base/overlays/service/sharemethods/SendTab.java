



package org.mozilla.gecko.overlays.service.sharemethods;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.os.Bundle;
import android.os.Parcelable;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;

import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.RemoteClient;
import org.mozilla.gecko.db.TabsAccessor;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.fxa.FxAccountConstants;
import org.mozilla.gecko.fxa.activities.FxAccountGetStartedActivity;
import org.mozilla.gecko.fxa.activities.FxAccountStatusActivity;
import org.mozilla.gecko.fxa.authenticator.AndroidFxAccount;
import org.mozilla.gecko.fxa.login.State;
import org.mozilla.gecko.overlays.OverlayConstants;
import org.mozilla.gecko.overlays.service.ShareData;
import org.mozilla.gecko.sync.CommandProcessor;
import org.mozilla.gecko.sync.CommandRunner;
import org.mozilla.gecko.sync.GlobalSession;
import org.mozilla.gecko.sync.SyncConfiguration;
import org.mozilla.gecko.sync.SyncConstants;
import org.mozilla.gecko.sync.setup.SyncAccounts;
import org.mozilla.gecko.sync.syncadapter.SyncAdapter;

import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;






public class SendTab extends ShareMethod {
    private static final String LOGTAG = "GeckoSendTab";

    
    public static final String SEND_TAB_TARGET_DEVICES = "SEND_TAB_TARGET_DEVICES";

    
    public static final String EXTRA_REMOTE_CLIENT_RECORDS = "RECORDS";

    
    
    public static final String OVERRIDE_INTENT = "OVERRIDE_INTENT";

    private Set<String> validGUIDs;

    
    private TabSender tabSender;

    @Override
    public Result handle(ShareData shareData) {
        if (shareData.extra == null) {
            Log.e(LOGTAG, "No target devices specified!");

            
            return Result.PERMANENT_FAILURE;
        }

        String[] targetGUIDs = ((Bundle) shareData.extra).getStringArray(SEND_TAB_TARGET_DEVICES);

        
        if (!validGUIDs.containsAll(Arrays.asList(targetGUIDs))) {
            
            Log.e(LOGTAG, "Not all provided GUIDs are real devices:");
            for (String targetGUID : targetGUIDs) {
                if (!validGUIDs.contains(targetGUID)) {
                    Log.e(LOGTAG, "Invalid GUID: " + targetGUID);
                }
            }

            return Result.PERMANENT_FAILURE;
        }

        Log.i(LOGTAG, "Send tab handler invoked.");

        final CommandProcessor processor = CommandProcessor.getProcessor();

        final String accountGUID = tabSender.getAccountGUID();
        Log.d(LOGTAG, "Retrieved local account GUID '" + accountGUID + "'.");

        if (accountGUID == null) {
            Log.e(LOGTAG, "Cannot determine account GUID");

            
            
            return Result.TRANSIENT_FAILURE;
        }

        
        
        
        for (int i = 0; i < targetGUIDs.length; i++) {
            processor.sendURIToClientForDisplay(shareData.url, targetGUIDs[i], shareData.title, accountGUID, context);
        }

        
        Log.i(LOGTAG, "Requesting immediate clients stage sync.");
        tabSender.sync();

        return Result.SUCCESS;
        
    }

    



    private Intent getUIStateIntent() {
        Intent uiStateIntent = new Intent(OverlayConstants.SHARE_METHOD_UI_EVENT);
        uiStateIntent.putExtra(OverlayConstants.EXTRA_SHARE_METHOD, (Parcelable) Type.SEND_TAB);
        return uiStateIntent;
    }

    


    private void broadcastUIState(Intent uiStateIntent) {
        LocalBroadcastManager.getInstance(context).sendBroadcast(uiStateIntent);
    }

    



    public SendTab(Context aContext) {
        super(aContext);
        

        
        
        final AccountManager accountManager = AccountManager.get(context);
        final Account[] fxAccounts = accountManager.getAccountsByType(FxAccountConstants.ACCOUNT_TYPE);

        if (fxAccounts.length > 0) {
            final AndroidFxAccount fxAccount = new AndroidFxAccount(context, fxAccounts[0]);
            if (fxAccount.getState().getNeededAction() != State.Action.None) {
                
                
                Log.w(LOGTAG, "Firefox Account named like " + fxAccount.getObfuscatedEmail() +
                              " needs action before it can send a tab; redirecting to status activity.");

                setOverrideIntent(FxAccountStatusActivity.class);
                return;
            }

            tabSender = new FxAccountTabSender(fxAccount);

            updateClientList(tabSender);

            Log.i(LOGTAG, "Allowing tab send for Firefox Account.");
            registerDisplayURICommand();
            return;
        }

        final Account[] syncAccounts = accountManager.getAccountsByType(SyncConstants.ACCOUNTTYPE_SYNC);
        if (syncAccounts.length > 0) {
            tabSender = new Sync11TabSender(context, syncAccounts[0], accountManager);

            updateClientList(tabSender);

            Log.i(LOGTAG, "Allowing tab send for Sync account.");
            registerDisplayURICommand();
            return;
        }

        
        setOverrideIntent(FxAccountGetStartedActivity.class);
    }

    


    private void updateClientList(TabSender tabSender) {
        Collection<RemoteClient> otherClients = getOtherClients(tabSender);

        
        RemoteClient[] records = new RemoteClient[otherClients.size()];
        records = otherClients.toArray(records);

        validGUIDs = new HashSet<>();

        for (RemoteClient client : otherClients) {
            validGUIDs.add(client.guid);
        }

        if (validGUIDs.isEmpty()) {
            
            
            setOverrideIntent(FxAccountGetStartedActivity.class);
            return;
        }

        Intent uiStateIntent = getUIStateIntent();
        uiStateIntent.putExtra(EXTRA_REMOTE_CLIENT_RECORDS, records);
        broadcastUIState(uiStateIntent);
    }

    









    protected void setOverrideIntent(Class<? extends Activity> activityClass) {
        Intent intent = new Intent(context, activityClass);
        
        
        intent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        Intent uiStateIntent = getUIStateIntent();
        uiStateIntent.putExtra(OVERRIDE_INTENT, intent);

        broadcastUIState(uiStateIntent);
    }

    private static void registerDisplayURICommand() {
        final CommandProcessor processor = CommandProcessor.getProcessor();
        processor.registerCommand("displayURI", new CommandRunner(3) {
            @Override
            public void executeCommand(final GlobalSession session, List<String> args) {
                CommandProcessor.displayURI(args, session.getContext());
            }
        });
    }

    


    protected Collection<RemoteClient> getOtherClients(final TabSender sender) {
        if (sender == null) {
            Log.w(LOGTAG, "No tab sender when fetching other client IDs.");
            return Collections.emptyList();
        }

        final BrowserDB browserDB = GeckoProfile.get(context).getDB();
        final TabsAccessor tabsAccessor = browserDB.getTabsAccessor();
        final Cursor remoteTabsCursor = tabsAccessor.getRemoteClientsByRecencyCursor(context);
        try {
            if (remoteTabsCursor.getCount() == 0) {
                return Collections.emptyList();
            }
            return tabsAccessor.getClientsWithoutTabsByRecencyFromCursor(remoteTabsCursor);
        } finally {
            remoteTabsCursor.close();
        }
    }

    @Override
    public String getSuccessMessage() {
        return context.getResources().getString(R.string.sync_text_tab_sent);
    }

    @Override
    public String getFailureMessage() {
        return context.getResources().getString(R.string.overlay_share_tab_not_sent);
    }

    



    private interface TabSender {
        public static final String[] STAGES_TO_SYNC = new String[] { "clients", "tabs" };

        



        String getAccountGUID();

        


        void sync();
    }

    private static class FxAccountTabSender implements TabSender {
        private final AndroidFxAccount fxAccount;

        public FxAccountTabSender(AndroidFxAccount fxa) {
            fxAccount = fxa;
        }

        @Override
        public String getAccountGUID() {
            try {
                final SharedPreferences prefs = fxAccount.getSyncPrefs();
                return prefs.getString(SyncConfiguration.PREF_ACCOUNT_GUID, null);
            } catch (Exception e) {
                Log.w(LOGTAG, "Could not get Firefox Account parameters or preferences; aborting.");
                return null;
            }
        }

        @Override
        public void sync() {
            fxAccount.requestSync(FirefoxAccounts.FORCE, STAGES_TO_SYNC, null);
        }
    }

    private static class Sync11TabSender implements TabSender {
        private final Account account;
        private final AccountManager accountManager;
        private final Context context;

        private Sync11TabSender(Context aContext, Account syncAccount, AccountManager manager) {
            context = aContext;
            account = syncAccount;
            accountManager = manager;
        }

        @Override
        public String getAccountGUID() {
            try {
                SharedPreferences prefs = SyncAccounts.blockingPrefsFromDefaultProfileV0(context, accountManager, account);
                return prefs.getString(SyncConfiguration.PREF_ACCOUNT_GUID, null);
            } catch (Exception e) {
                Log.w(LOGTAG, "Could not get Sync account parameters or preferences; aborting.");
                return null;
            }
        }

        @Override
        public void sync() {
            SyncAdapter.requestImmediateSync(account, STAGES_TO_SYNC);
        }
    }
}
