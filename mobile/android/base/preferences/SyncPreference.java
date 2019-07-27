




package org.mozilla.gecko.preferences;

import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.TelemetryContract.Method;
import org.mozilla.gecko.fxa.activities.FxAccountGetStartedActivity;
import org.mozilla.gecko.sync.setup.SyncAccounts;
import org.mozilla.gecko.sync.setup.activities.SetupSyncActivity;
import org.mozilla.gecko.util.HardwareUtils;

import android.content.Context;
import android.content.Intent;
import android.preference.Preference;
import android.util.AttributeSet;

class SyncPreference extends Preference {
    private static final boolean DEFAULT_TO_FXA = true;

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

        if (HardwareUtils.IS_KINDLE_DEVICE) {
            intent.setFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
        }

        mContext.startActivity(intent);
    }

    @Override
    protected void onClick() {
        
        if (!DEFAULT_TO_FXA) {
            openSync11Settings();
            return;
        }

        
        
        if (SyncAccounts.syncAccountsExist(mContext)) {
            if (SyncAccounts.openSyncSettings(mContext) != null) {
                Telemetry.sendUIEvent(TelemetryContract.Event.ACTION, Method.SETTINGS, "sync_settings");
                return;
            }
        }

        
        
        launchFxASetup();
        Telemetry.sendUIEvent(TelemetryContract.Event.ACTION, Method.SETTINGS, "sync_setup");
    }
}
