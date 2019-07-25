




































package com.mozilla.SUTAgentAndroid.service;

import java.util.TimerTask;

import android.content.Context;
import android.content.ContextWrapper;
import android.media.Ringtone;
import android.media.RingtoneManager;
import android.widget.Toast;

class DoAlert extends TimerTask
    {
    int    lcv = 0;
    Toast toast = null;
    Ringtone rt = null;

    DoAlert(ContextWrapper contextWrapper)
        {
        Context    ctx = contextWrapper.getApplicationContext();
        this.toast = Toast.makeText(ctx, "Help me!", Toast.LENGTH_LONG);
        rt = RingtoneManager.getRingtone(ctx, RingtoneManager.getDefaultUri(RingtoneManager.TYPE_ALARM));
        }

    public void term()
        {
        if (rt != null)
            {
            if (rt.isPlaying())
                rt.stop();
            }

        if (toast != null)
            toast.cancel();
        }

    public void run ()
        {
        String sText =(((lcv++ % 2) == 0)  ? "Help me!" : "I've fallen down!" );
        toast.setText(sText);
        toast.show();
        if (rt != null)
            rt.play();
        }
    }
