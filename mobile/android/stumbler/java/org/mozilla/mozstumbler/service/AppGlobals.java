



package org.mozilla.mozstumbler.service;

import java.util.concurrent.ConcurrentLinkedQueue;

public class AppGlobals {
    public static final String LOG_PREFIX = "Stumbler:";

    
    public static final String ACTION_NAMESPACE = "org.mozilla.mozstumbler.intent.action";

    
    public static final String ACTION_GUI_LOG_MESSAGE = AppGlobals.ACTION_NAMESPACE + ".LOG_MESSAGE";
    public static final String ACTION_GUI_LOG_MESSAGE_EXTRA = ACTION_GUI_LOG_MESSAGE + ".MESSAGE";

    



    public static final String ACTION_ARG_TIME = "time";

    
    public static final String LOCATION_ORIGIN_INTERNAL = "internal";

    public enum ActiveOrPassiveStumbling { ACTIVE_STUMBLING, PASSIVE_STUMBLING }

    
    public static final int PASSIVE_MODE_MAX_SCANS_PER_GPS = 3;

    
    public static String appVersionName = "0.0.0";
    public static int appVersionCode = 0;
    public static String appName = "StumblerService";
    public static boolean isDebug;

    

    public static volatile ConcurrentLinkedQueue<String> guiLogMessageBuffer;

    public static void guiLogError(String msg) {
        guiLogInfo(msg, "red", true);
    }

    public static void guiLogInfo(String msg) {
        guiLogInfo(msg, "white", false);
    }

    public static void guiLogInfo(String msg, String color, boolean isBold) {
        if (guiLogMessageBuffer != null) {
            if (isBold) {
                msg = "<b>" + msg + "</b>";
            }
            guiLogMessageBuffer.add("<font color='" + color +"'>" + msg + "</font>");
        }
    }
}

