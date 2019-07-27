package com.adjust.sdk;

import org.json.JSONObject;




public interface IAttributionHandler {
    public void init(IActivityHandler activityHandler,
                     ActivityPackage attributionPackage,
                     boolean startPaused);

    public void getAttribution();

    public void checkAttribution(JSONObject jsonResponse);

    public void pauseSending();

    public void resumeSending();
}
