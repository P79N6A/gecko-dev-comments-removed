




































package com.mozilla.SUTAgentAndroid;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class SUTStartupIntentReceiver extends BroadcastReceiver
{
    @Override
    public void onReceive(Context context, Intent intent)
        {
        Intent mySUTAgentIntent = new Intent(context, SUTAgentAndroid.class);
        mySUTAgentIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(mySUTAgentIntent);
        }
}
