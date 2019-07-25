




package org.mozilla.gecko;

import android.content.Context;
import android.content.Intent;
import android.preference.Preference;
import android.util.AttributeSet;

import org.mozilla.gecko.sync.setup.activities.SetupSyncActivity;
import org.mozilla.gecko.sync.setup.SyncAccounts;

class SyncPreference extends Preference {
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
