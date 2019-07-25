




package org.mozilla.gecko;

import org.json.JSONException;
import org.json.JSONObject;

public final class ZoomConstraints {
    private final boolean mAllowZoom;
    private final float mDefaultZoom;
    private final float mMinZoom;
    private final float mMaxZoom;

    public ZoomConstraints(boolean allowZoom) {
        mAllowZoom = allowZoom;
        mDefaultZoom = 0.0f;
        mMinZoom = 0.0f;
        mMaxZoom = 0.0f;
    }

    ZoomConstraints(JSONObject message) throws JSONException {
        mAllowZoom = message.getBoolean("allowZoom");
        mDefaultZoom = (float)message.getDouble("defaultZoom");
        mMinZoom = (float)message.getDouble("minZoom");
        mMaxZoom = (float)message.getDouble("maxZoom");
    }

    public final boolean getAllowZoom() {
        return mAllowZoom;
    }

    public final float getDefaultZoom() {
        return mDefaultZoom;
    }

    public final float getMinZoom() {
        return mMinZoom;
    }

    public final float getMaxZoom() {
        return mMaxZoom;
    }
}
