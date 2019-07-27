



package org.mozilla.mozstumbler.service;

import java.util.concurrent.ConcurrentLinkedQueue;

public class AppGlobals {
    public static final String LOG_PREFIX = "Stumbler_";

    
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

    public static String makeLogTag(String name) {
        final int maxLen = 23 - LOG_PREFIX.length();
        if (name.length() > maxLen) {
            name = name.substring(name.length() - maxLen, name.length());
        }
        return LOG_PREFIX + name;
    }

    public static final String ACTION_TEST_SETTING_ENABLED = "stumbler-test-setting-enabled";
    public static final String ACTION_TEST_SETTING_DISABLED = "stumbler-test-setting-disabled";

    
    public static final String TELEMETRY_TIME_BETWEEN_UPLOADS_SEC = "STUMBLER_TIME_BETWEEN_UPLOADS_SEC";
    public static final String TELEMETRY_BYTES_UPLOADED_PER_SEC = "STUMBLER_VOLUME_BYTES_UPLOADED_PER_SEC";
    public static final String TELEMETRY_TIME_BETWEEN_STARTS_SEC = "STUMBLER_TIME_BETWEEN_START_SEC";
    public static final String TELEMETRY_BYTES_PER_UPLOAD = "STUMBLER_UPLOAD_BYTES";
    public static final String TELEMETRY_OBSERVATIONS_PER_UPLOAD = "STUMBLER_UPLOAD_OBSERVATION_COUNT";
    public static final String TELEMETRY_CELLS_PER_UPLOAD = "STUMBLER_UPLOAD_CELL_COUNT";
    public static final String TELEMETRY_WIFIS_PER_UPLOAD = "STUMBLER_UPLOAD_WIFI_AP_COUNT";
    public static final String TELEMETRY_OBSERVATIONS_PER_DAY = "STUMBLER_OBSERVATIONS_PER_DAY";
    public static final String TELEMETRY_TIME_BETWEEN_RECEIVED_LOCATIONS_SEC = "STUMBLER_TIME_BETWEEN_RECEIVED_LOCATIONS_SEC";
}

