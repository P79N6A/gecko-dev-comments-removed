




package org.mozilla.gecko.preferences;

import org.mozilla.gecko.fxa.activities.FxAccountGetStartedActivity;
import org.mozilla.gecko.sync.setup.SyncAccounts;
import org.mozilla.gecko.sync.setup.activities.SetupSyncActivity;

import android.content.Context;
import android.content.Intent;
import android.preference.Preference;
import android.util.AttributeSet;

class SyncPreference extends Preference {
    private Context mContext;

    public SyncPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
    }

    private void openSync11Settings() {
        
        if (SyncAccounts.syncAccountsExist(mContext)) {
            SyncAccounts.openSyncSettings(mContext);
            return;
        }
        Intent intent = new Intent(mContext, SetupSyncActivity.class);
        mContext.startActivity(intent);
    }

    private void launchFxASetup() {
        Intent intent = new Intent(mContext, FxAccountGetStartedActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mContext.startActivity(intent);
    }

    @Override
    protected void onClick() {
        openSync11Settings();
    }
}
