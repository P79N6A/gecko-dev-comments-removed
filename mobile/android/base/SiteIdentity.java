




package org.mozilla.gecko;

import org.json.JSONObject;

import android.text.TextUtils;

public class SiteIdentity {
    private SecurityMode mSecurityMode;
    private String mHost;
    private String mOwner;
    private String mSupplemental;
    private String mVerifier;
    private String mEncrypted;

    
    
    public enum SecurityMode {
        UNKNOWN("unknown"),
        IDENTIFIED("identified"),
        VERIFIED("verified"),
        MIXED_CONTENT_BLOCKED("mixed_content_blocked"),
        MIXED_CONTENT_LOADED("mixed_content_loaded");

        private final String mId;

        private SecurityMode(String id) {
            mId = id;
        }

        public static SecurityMode fromString(String id) {
            if (id == null) {
                throw new IllegalArgumentException("Can't convert null String to SiteIdentity");
            }

            for (SecurityMode mode : SecurityMode.values()) {
                if (TextUtils.equals(mode.mId, id.toLowerCase())) {
                    return mode;
                }
            }

            throw new IllegalArgumentException("Could not convert String id to SiteIdentity");
        }

        @Override
        public String toString() {
            return mId;
        }
    }

    public SiteIdentity() {
        reset(SecurityMode.UNKNOWN);
    }

    private void reset(SecurityMode securityMode) {
        mSecurityMode = securityMode;
        mHost = null;
        mOwner = null;
        mSupplemental = null;
        mVerifier = null;
        mEncrypted = null;
    }

    void update(JSONObject identityData) {
        try {
            mSecurityMode = SecurityMode.fromString(identityData.getString("mode"));
        } catch (Exception e) {
            reset(SecurityMode.UNKNOWN);
            return;
        }

        try {
            mHost = identityData.getString("host");
            mOwner = identityData.getString("owner");
            mSupplemental = identityData.optString("supplemental", null);
            mVerifier = identityData.getString("verifier");
            mEncrypted = identityData.getString("encrypted");
        } catch (Exception e) {
            reset(mSecurityMode);
        }
    }

    public SecurityMode getSecurityMode() {
        return mSecurityMode;
    }

    public String getHost() {
        return mHost;
    }

    public String getOwner() {
        return mOwner;
    }

    public String getSupplemental() {
        return mSupplemental;
    }

    public String getVerifier() {
        return mVerifier;
    }

    public String getEncrypted() {
        return mEncrypted;
    }
}