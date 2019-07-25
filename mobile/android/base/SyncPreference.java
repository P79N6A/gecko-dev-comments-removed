




































package org.mozilla.gecko;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.preference.Preference;
import android.util.AttributeSet;
import android.util.Log;

import org.mozilla.gecko.sync.setup.activities.SetupSyncActivity;

class SyncPreference extends Preference {
    private static final String FENNEC_ACCOUNT_TYPE = "org.mozilla.firefox_sync";
    private static final String SYNC_SETTINGS = "android.settings.SYNC_SETTINGS";

    private Context mContext;

    public SyncPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
    }

    @Override
    protected void onClick() {
        
        Account[] accounts = AccountManager.get(mContext).getAccountsByType(FENNEC_ACCOUNT_TYPE);
        Intent intent;
        if (accounts.length > 0)
            intent = new Intent(SYNC_SETTINGS);
        else
            intent = new Intent(mContext, SetupSyncActivity.class);
        mContext.startActivity(intent);
    }
}
