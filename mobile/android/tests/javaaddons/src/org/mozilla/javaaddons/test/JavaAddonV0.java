




package org.mozilla.javaaddons.test;

import android.os.Handler;
import android.os.Message;
import android.util.Log;

import java.util.Map;

public class JavaAddonV0 implements Handler.Callback {
    public JavaAddonV0(Map<String, Handler.Callback> callbacks) {
        callbacks.put("JavaAddon:V0", this);
    }

    @Override
    public boolean handleMessage(Message message) {
        Log.i("JavaAddon", "handleMessage " + message.toString());
        return true;
    }
}
