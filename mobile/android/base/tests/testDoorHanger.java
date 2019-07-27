package org.mozilla.gecko.tests;

import android.widget.CheckBox;
import android.view.View;
import com.jayway.android.robotium.solo.Condition;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.Actions;








public class testDoorHanger extends BaseTest {
    public void testDoorHanger() {
        String GEO_URL = getAbsoluteUrl(StringHelper.ROBOCOP_GEOLOCATION_URL);
        String BLANK_URL = getAbsoluteUrl(StringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        String OFFLINE_STORAGE_URL = getAbsoluteUrl(StringHelper.ROBOCOP_OFFLINE_STORAGE_URL);
        String LOGIN_URL = getAbsoluteUrl(StringHelper.ROBOCOP_LOGIN_URL);

        
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

        
        waitForCheckBox();
        mSolo.clickOnCheckBox(0);
        mSolo.clickOnButton(GEO_ALLOW);
        waitForTextDismissed(GEO_MESSAGE);
        mAsserter.is(mSolo.searchText(GEO_MESSAGE), false, "Geolocation doorhanger has been hidden when allowing share");

        
        inputAndLoadUrl(GEO_URL);
        waitForText(GEO_MESSAGE);

        
        waitForCheckBox();
        mSolo.clickOnCheckBox(0);
        mSolo.clickOnButton(GEO_DENY);
        waitForTextDismissed(GEO_MESSAGE);
        mAsserter.is(mSolo.searchText(GEO_MESSAGE), false, "Geolocation doorhanger has been hidden when denying share");

        












        boolean offlineAllowedByDefault = true;
        
        final String[] prefNames = { "offline-apps.allow_by_default" };
        final int ourRequestId = 0x7357;
        final Actions.RepeatedEventExpecter eventExpecter = mActions.expectGeckoEvent("Preferences:Data");
        mActions.sendPreferencesGetEvent(ourRequestId, prefNames);
        try {
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
            setPreferenceAndWaitForChange(jsonPref);
        } catch (JSONException e) {
            mAsserter.ok(false, "exception getting preference", e.toString());
        }

        
        inputAndLoadUrl(OFFLINE_STORAGE_URL);
        waitForText(OFFLINE_MESSAGE);

        
        waitForCheckBox();
        mSolo.clickOnCheckBox(0);
        mSolo.clickOnButton(OFFLINE_DENY);
        waitForTextDismissed(OFFLINE_MESSAGE);
        mAsserter.is(mSolo.searchText(OFFLINE_MESSAGE), false, "Offline storage doorhanger notification is hidden when denying storage");

        
        inputAndLoadUrl(OFFLINE_STORAGE_URL);
        waitForText(OFFLINE_MESSAGE);

        
        mSolo.clickOnButton(OFFLINE_ALLOW);
        waitForTextDismissed(OFFLINE_MESSAGE);
        mAsserter.is(mSolo.searchText(OFFLINE_MESSAGE), false, "Offline storage doorhanger notification is hidden when allowing storage");
        inputAndLoadUrl(OFFLINE_STORAGE_URL);
        mAsserter.is(mSolo.searchText(OFFLINE_MESSAGE), false, "Offline storage doorhanger is no longer triggered");

        try {
            
            JSONObject jsonPref = new JSONObject();
            jsonPref.put("name", "offline-apps.allow_by_default");
            jsonPref.put("type", "bool");
            jsonPref.put("value", offlineAllowedByDefault);
            setPreferenceAndWaitForChange(jsonPref);
        } catch (JSONException e) {
            mAsserter.ok(false, "exception setting preference", e.toString());
        }


        
        inputAndLoadUrl(LOGIN_URL);
        waitForText(LOGIN_MESSAGE);

        
        mSolo.clickOnButton(LOGIN_DENY);
        waitForTextDismissed(LOGIN_MESSAGE);
        mAsserter.is(mSolo.searchText(LOGIN_MESSAGE), false, "Login doorhanger notification is hidden when denying saving password");

        
        inputAndLoadUrl(LOGIN_URL);
        waitForText(LOGIN_MESSAGE);

        
        mSolo.clickOnButton(LOGIN_ALLOW);
        waitForTextDismissed(LOGIN_MESSAGE);
        mAsserter.is(mSolo.searchText(LOGIN_MESSAGE), false, "Login doorhanger notification is hidden when allowing saving password");
    }

    
    private void waitForCheckBox() {
        waitForCondition(new Condition() {
            @Override
            public boolean isSatisfied() {
                for (CheckBox view : mSolo.getCurrentViews(CheckBox.class)) {
                    
                    
                    
                    if (view.isClickable() &&
                        view.getVisibility() == View.VISIBLE &&
                        view.getWidth() > 0 &&
                        view.getHeight() > 0) {
                        return true;
                    }
                }
                return false;
            }
        }, MAX_WAIT_MS);
    }

    
    private void waitForTextDismissed(final String text) {
        waitForCondition(new Condition() {
            @Override
            public boolean isSatisfied() {
                return !mSolo.searchText(text);
            }
        }, MAX_WAIT_MS);
    }
}
