package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;

import android.app.Activity;
import android.content.SharedPreferences;
import java.lang.reflect.Method;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;








public class testDoorHanger extends BaseTest {

    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    public void testDoorHanger() {
        String GEO_URL = getAbsoluteUrl("/robocop/robocop_geolocation.html");
        String BLANK_URL = getAbsoluteUrl("/robocop/robocop_blank_01.html");
        String OFFLINE_STORAGE_URL = getAbsoluteUrl("/robocop/robocop_offline_storage.html");
        String LOGIN_URL = getAbsoluteUrl("/robocop/robocop_login.html");

        
        String GEO_MESSAGE = "Share your location with";
        String GEO_ALLOW = "Share";
        String GEO_DENY = "Don't share";

        String OFFLINE_MESSAGE = "to store data on your device for offline use";
        String OFFLINE_ALLOW = "Allow";
        String OFFLINE_DENY = "Don't allow";

        String LOGIN_MESSAGE = "Save password";
        String LOGIN_ALLOW = "Save";
        String LOGIN_DENY = "Don't save";

        blockForGeckoReady();

        
        inputAndLoadUrl(GEO_URL);
        waitForText(GEO_MESSAGE);
        mAsserter.is(mSolo.searchText(GEO_MESSAGE), true, "Geolocation doorhanger has been displayed");

        
        mSolo.clickOnCheckBox(0);
        mSolo.clickOnButton(GEO_ALLOW);
        mAsserter.is(mSolo.searchText(GEO_MESSAGE), false, "Geolocation doorhanger has been hidden when allowing share");

        
        inputAndLoadUrl(GEO_URL);
        waitForText(GEO_MESSAGE);

        
        mSolo.clickOnCheckBox(0);
        mSolo.clickOnButton(GEO_DENY);
        mAsserter.is(mSolo.searchText(GEO_MESSAGE), false, "Geolocation doorhanger has been hidden when denying share");

        












        boolean offlineAllowedByDefault = true;
        try {
            
            final String[] prefNames = { "offline-apps.allow_by_default" };
            final int ourRequestId = 0x7357;

            Actions.RepeatedEventExpecter eventExpecter = mActions.expectGeckoEvent("Preferences:Data");
            mActions.sendPreferencesGetEvent(ourRequestId, prefNames);

            JSONObject data = null;
            int requestId = -1;

            
            while (requestId != ourRequestId) {
                data = new JSONObject(eventExpecter.blockForEventData());
                requestId = data.getInt("requestId");
            }
            eventExpecter.unregisterListener();

            JSONArray preferences = data.getJSONArray("preferences");
            if (preferences.length() > 0) {
                JSONObject pref = (JSONObject) preferences.get(0);
                offlineAllowedByDefault = pref.getBoolean("value");
            }

            
            JSONObject jsonPref = new JSONObject();
            jsonPref.put("name", "offline-apps.allow_by_default");
            jsonPref.put("type", "bool");
            jsonPref.put("value", false);
            mActions.sendGeckoEvent("Preferences:Set", jsonPref.toString());
        } catch (JSONException e) {
            mAsserter.ok(false, "exception getting preference", e.toString());
        }

        
        inputAndLoadUrl(OFFLINE_STORAGE_URL);
        waitForText(OFFLINE_MESSAGE);

        
        mSolo.clickOnCheckBox(0);
        mSolo.clickOnButton(OFFLINE_DENY);
        mAsserter.is(mSolo.searchText(OFFLINE_MESSAGE), false, "Offline storage doorhanger notification is hidden when denying storage");

        
        inputAndLoadUrl(OFFLINE_STORAGE_URL);
        waitForText(OFFLINE_MESSAGE);

        
        mSolo.clickOnButton(OFFLINE_ALLOW);
        mAsserter.is(mSolo.searchText(OFFLINE_MESSAGE), false, "Offline storage doorhanger notification is hidden when allowing storage");
        inputAndLoadUrl(OFFLINE_STORAGE_URL);
        mAsserter.is(mSolo.searchText(OFFLINE_MESSAGE), false, "Offline storage doorhanger is no longer triggered");

        try {
            
            JSONObject jsonPref = new JSONObject();
            jsonPref.put("name", "offline-apps.allow_by_default");
            jsonPref.put("type", "boolean");
            jsonPref.put("value", offlineAllowedByDefault);
            mActions.sendGeckoEvent("Preferences:Set", jsonPref.toString());
        } catch (JSONException e) {
            mAsserter.ok(false, "exception setting preference", e.toString());
        }


        
        inputAndLoadUrl(LOGIN_URL);
        waitForText(LOGIN_MESSAGE);

        
        mSolo.clickOnButton(LOGIN_DENY);
        mAsserter.is(mSolo.searchText(LOGIN_MESSAGE), false, "Login doorhanger notification is hidden when denying saving password");

        
        inputAndLoadUrl(LOGIN_URL);
        waitForText(LOGIN_MESSAGE);

        
        mSolo.clickOnButton(LOGIN_ALLOW);
        mAsserter.is(mSolo.searchText(LOGIN_MESSAGE), false, "Login doorhanger notification is hidden when allowing saving password");

        
        inputAndLoadUrl(LOGIN_URL);
        mAsserter.is(mSolo.searchText(LOGIN_MESSAGE), false, "Login doorhanger is not re-triggered");
    }
}
