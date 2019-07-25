




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
import org.mozilla.gecko.sync.setup.SyncAccounts;

final class SyncPreference extends Preference {
    private Context mContext;

    public SyncPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
    }

    @Override
    protected void onClick() {
        
        final String accountType = org.mozilla.gecko.sync.setup.Constants.ACCOUNTTYPE_SYNC;

        
        if (SyncAccounts.syncAccountsExist(mContext)) {
            SyncAccounts.openSyncSettings(mContext);
        } else {
            Intent intent = new Intent(mContext, SetupSyncActivity.class);
            mContext.startActivity(intent);
        }
    }
}
