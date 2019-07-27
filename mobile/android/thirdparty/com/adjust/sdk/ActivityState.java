








package com.adjust.sdk;

import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectInputStream.GetField;
import java.io.Serializable;
import java.util.Calendar;
import java.util.Locale;

public class ActivityState implements Serializable, Cloneable {
    private static final long serialVersionUID = 9039439291143138148L;
    private transient String readErrorMessage = "Unable to read '%s' field in migration device with message (%s)";
    private transient ILogger logger;

    
    protected String uuid;
    protected boolean enabled;
    protected boolean askingAttribution;

    
    protected int eventCount;
    protected int sessionCount;

    
    protected int subsessionCount;
    protected long sessionLength;   
    protected long timeSpent;
    protected long lastActivity;    

    protected long lastInterval;

    protected ActivityState() {
        logger = AdjustFactory.getLogger();
        
        uuid = Util.createUuid();
        enabled = true;
        askingAttribution = false;

        eventCount = 0; 
        sessionCount = 0; 
        subsessionCount = -1; 
        sessionLength = -1; 
        timeSpent = -1; 
        lastActivity = -1;
        lastInterval = -1;
    }

    protected void resetSessionAttributes(long now) {
        subsessionCount = 1; 
        sessionLength = 0; 
        timeSpent = 0; 
        lastActivity = now;
        lastInterval = -1;
    }

    @Override
    public String toString() {
        return String.format(Locale.US,
                "ec:%d sc:%d ssc:%d sl:%.1f ts:%.1f la:%s uuid:%s",
                eventCount, sessionCount, subsessionCount,
                sessionLength / 1000.0, timeSpent / 1000.0,
                stamp(lastActivity), uuid);
    }

    @Override
    public ActivityState clone() {
        try {
            return (ActivityState) super.clone();
        } catch (CloneNotSupportedException e) {
            return null;
        }
    }


    private void readObject(ObjectInputStream stream) throws IOException, ClassNotFoundException {
        GetField fields = stream.readFields();

        eventCount = readIntField(fields, "eventCount", 0);
        sessionCount = readIntField(fields, "sessionCount", 0);
        subsessionCount = readIntField(fields, "subsessionCount", -1);
        sessionLength = readLongField(fields, "sessionLength", -1l);
        timeSpent = readLongField(fields, "timeSpent", -1l);
        lastActivity = readLongField(fields, "lastActivity", -1l);
        lastInterval = readLongField(fields, "lastInterval", -1l);

        
        uuid = readStringField(fields, "uuid", null);
        enabled = readBooleanField(fields, "enabled", true);
        askingAttribution = readBooleanField(fields, "askingAttribution", false);

        
        if (uuid == null) {
            uuid = Util.createUuid();
        }
    }

    private String readStringField(GetField fields, String name, String defaultValue) {
        try {
            return (String) fields.get(name, defaultValue);
        } catch (Exception e) {
            logger.debug(readErrorMessage, name, e.getMessage());
            return defaultValue;
        }
    }

    private boolean readBooleanField(GetField fields, String name, boolean defaultValue) {
        try {
            return fields.get(name, defaultValue);
        } catch (Exception e) {
            logger.debug(readErrorMessage, name, e.getMessage());
            return defaultValue;
        }
    }

    private int readIntField(GetField fields, String name, int defaultValue) {
        try {
            return fields.get(name, defaultValue);
        } catch (Exception e) {
            logger.debug(readErrorMessage, name, e.getMessage());
            return defaultValue;
        }
    }

    private long readLongField(GetField fields, String name, long defaultValue) {
        try {
            return fields.get(name, defaultValue);
        } catch (Exception e) {
            logger.debug(readErrorMessage, name, e.getMessage());
            return defaultValue;
        }
    }

    private static String stamp(long dateMillis) {
        Calendar calendar = Calendar.getInstance();
        calendar.setTimeInMillis(dateMillis);
        return String.format(Locale.US,
                "%02d:%02d:%02d",
                calendar.HOUR_OF_DAY,
                calendar.MINUTE,
                calendar.SECOND);
    }
}
