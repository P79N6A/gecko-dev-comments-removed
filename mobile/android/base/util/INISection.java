



package org.mozilla.gecko.util;

import android.text.TextUtils;
import android.util.Log;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.Enumeration;
import java.util.Hashtable;

public class INISection {
    private static final String LOGTAG = "INIParser";

    
    private String mName;
    public String getName() { return mName; }
    public void setName(String name) { mName = name; }

    
    private  boolean mDebug;

    
    protected Hashtable<String, Object> mProperties;

    
    
    public INISection(String name) {
        mName = name;
    }

    
    protected void debug(String msg) {
        if (mDebug) {
            Log.i(LOGTAG, msg);
        }
    }
  
    
    public Object getProperty(String key) {
        getProperties(); 
        return mProperties.get(key);
    }

    
    public int getIntProperty(String key) {
        Object val = getProperty(key);
        if (val == null)
            return -1;

        return Integer.parseInt(val.toString());
    }

    
    public String getStringProperty(String key) {
        Object val = getProperty(key);
        if (val == null)
          return null;

        return val.toString();
    }

    
    public Hashtable<String, Object> getProperties() {
        if (mProperties == null) {
            try {
                parse();
            } catch (IOException e) {
                debug("Error parsing: " + e);
            }
        }
        return mProperties;
    }

    
    protected void parse() throws IOException {
        mProperties = new Hashtable<String, Object>();
    }

    
    public void setProperty(String key, Object value) {
        getProperties(); 
        if (value == null)
            removeProperty(key);
        else
            mProperties.put(key.trim(), value);     
    }   
 
    
    public void removeProperty(String name) {
        
        getProperties();
        mProperties.remove(name);
    }

    public void write(BufferedWriter writer) throws IOException {
        if (!TextUtils.isEmpty(mName)) {
            writer.write("[" + mName + "]");
            writer.newLine();
        }

        if (mProperties != null) {
            for (Enumeration<String> e = mProperties.keys(); e.hasMoreElements();) {
                String key = e.nextElement();
                writeProperty(writer, key, mProperties.get(key));
            }
        }
        writer.newLine();
    }

    
    private void writeProperty(BufferedWriter writer, String key, Object value) {
        try {
            writer.write(key + "=" + value);
            writer.newLine();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
