



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

        blockForGeckoReady();

        
        inputAndLoadUrl(GEO_URL);
        waitForText(StringHelper.GEO_MESSAGE);
        mAsserter.is(mSolo.searchText(StringHelper.GEO_MESSAGE), true, "Geolocation doorhanger has been displayed");

        
        waitForCheckBox();
        mSolo.clickOnCheckBox(0);
        mSolo.clickOnButton(StringHelper.GEO_ALLOW);
        waitForTextDismissed(StringHelper.GEO_MESSAGE);
        mAsserter.is(mSolo.searchText(StringHelper.GEO_MESSAGE), false, "Geolocation doorhanger has been hidden when allowing share");

        
        inputAndLoadUrl(GEO_URL);
        waitForText(StringHelper.GEO_MESSAGE);

        
        waitForCheckBox();
        mSolo.clickOnCheckBox(0);
        mSolo.clickOnButton(StringHelper.GEO_DENY);
        waitForTextDismissed(StringHelper.GEO_MESSAGE);
        mAsserter.is(mSolo.searchText(StringHelper.GEO_MESSAGE), false, "Geolocation doorhanger has been hidden when denying share");

        












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
        waitForText(StringHelper.OFFLINE_MESSAGE);

        
        waitForCheckBox();
        mSolo.clickOnCheckBox(0);
        mSolo.clickOnButton(StringHelper.OFFLINE_DENY);
        waitForTextDismissed(StringHelper.OFFLINE_MESSAGE);
        mAsserter.is(mSolo.searchText(StringHelper.OFFLINE_MESSAGE), false, "Offline storage doorhanger notification is hidden when denying storage");

        
        inputAndLoadUrl(OFFLINE_STORAGE_URL);
        waitForText(StringHelper.OFFLINE_MESSAGE);

        
        mSolo.clickOnButton(StringHelper.OFFLINE_ALLOW);
        waitForTextDismissed(StringHelper.OFFLINE_MESSAGE);
        mAsserter.is(mSolo.searchText(StringHelper.OFFLINE_MESSAGE), false, "Offline storage doorhanger notification is hidden when allowing storage");
        inputAndLoadUrl(OFFLINE_STORAGE_URL);
        mAsserter.is(mSolo.searchText(StringHelper.OFFLINE_MESSAGE), false, "Offline storage doorhanger is no longer triggered");

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
        waitForText(StringHelper.LOGIN_MESSAGE);

        
        mSolo.clickOnButton(StringHelper.LOGIN_DENY);
        waitForTextDismissed(StringHelper.LOGIN_MESSAGE);
        mAsserter.is(mSolo.searchText(StringHelper.LOGIN_MESSAGE), false, "Login doorhanger notification is hidden when denying saving password");

        
        inputAndLoadUrl(LOGIN_URL);
        waitForText(StringHelper.LOGIN_MESSAGE);

        
        mSolo.clickOnButton(StringHelper.LOGIN_ALLOW);
        waitForTextDismissed(StringHelper.LOGIN_MESSAGE);
        mAsserter.is(mSolo.searchText(StringHelper.LOGIN_MESSAGE), false, "Login doorhanger notification is hidden when allowing saving password");

        testPopupBlocking();
    }

    private void testPopupBlocking() {
        String POPUP_URL = getAbsoluteUrl(StringHelper.ROBOCOP_POPUP_URL);

        try {
            JSONObject jsonPref = new JSONObject();
            jsonPref.put("name", "dom.disable_open_during_load");
            jsonPref.put("type", "bool");
            jsonPref.put("value", true);
            setPreferenceAndWaitForChange(jsonPref);
        } catch (JSONException e) {
            mAsserter.ok(false, "exception setting preference", e.toString());
        }

        
        inputAndLoadUrl(POPUP_URL);
        waitForText(StringHelper.POPUP_MESSAGE);
        mAsserter.is(mSolo.searchText(StringHelper.POPUP_MESSAGE), true, "Popup blocker is displayed");

        
        Actions.EventExpecter tabEventExpecter = mActions.expectGeckoEvent("Tab:Added");

        waitForCheckBox();
        mSolo.clickOnCheckBox(0);
        mSolo.clickOnButton(StringHelper.POPUP_ALLOW);
        waitForTextDismissed(StringHelper.POPUP_MESSAGE);
        mAsserter.is(mSolo.searchText(StringHelper.POPUP_MESSAGE), false, "Popup blocker is hidden when popup allowed");

        try {
            final JSONObject data = new JSONObject(tabEventExpecter.blockForEventData());

            
            mAsserter.is("data:text/plain;charset=utf-8,a", data.getString("uri"), "Checking popup URL");

            
            closeTab(data.getInt("tabID"));

        } catch (JSONException e) {
            mAsserter.ok(false, "exception getting event data", e.toString());
        }
        tabEventExpecter.unregisterListener();

        
        inputAndLoadUrl(POPUP_URL);
        waitForText(StringHelper.POPUP_MESSAGE);
        mAsserter.is(mSolo.searchText(StringHelper.POPUP_MESSAGE), true, "Popup blocker is displayed");

        waitForCheckBox();
        mSolo.clickOnCheckBox(0);
        mSolo.clickOnButton(StringHelper.POPUP_DENY);
        waitForTextDismissed(StringHelper.POPUP_MESSAGE);
        mAsserter.is(mSolo.searchText(StringHelper.POPUP_MESSAGE), false, "Popup blocker is hidden when popup denied");

        
        verifyUrl(POPUP_URL);

        try {
            JSONObject jsonPref = new JSONObject();
            jsonPref.put("name", "dom.disable_open_during_load");
            jsonPref.put("type", "bool");
            jsonPref.put("value", false);
            setPreferenceAndWaitForChange(jsonPref);
        } catch (JSONException e) {
            mAsserter.ok(false, "exception setting preference", e.toString());
        }
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
