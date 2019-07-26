package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;

import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.HashMap;




public class testSettingsMenuItems extends PixelTest {
    int mMidWidth;
    int mMidHeight;
    String BRAND_NAME = "(Fennec|Nightly|Aurora|Firefox|Firefox Beta)";

    











    
    String[][] OPTIONS_CUSTOMIZE = {
        { "Search settings", "", "Show search suggestions", "Installed search engines"},
        { "Import from Android", "", "Bookmarks", "History", "Import" },
        { "Tabs", "Don't restore after quitting " + BRAND_NAME, "Always restore", "Don't restore after quitting " + BRAND_NAME },
    };

    
    String[][] OPTIONS_DISPLAY = {
        { "Text size" },
        { "Title bar", "Show page title", "Show page title", "Show page address" },
        { "Advanced" },
        { "Character encoding", "Don't show menu", "Show menu", "Don't show menu" },
        { "Plugins", "Tap to play", "Enabled", "Tap to play", "Disabled" },
    };

    
    String[][] OPTIONS_PRIVACY = {
        { "Tracking", "Do not tell sites anything about my tracking preferences", "Tell sites that I do not want to be tracked", "Tell sites that I want to be tracked", "Do not tell sites anything about my tracking preferences" },
        { "Cookies", "Enabled", "Enabled, excluding 3rd party", "Disabled" },
        { "Remember passwords" },
        { "Use master password" },
        { "Clear private data", "", "Browsing & download history", "Downloaded files", "Form & search history", "Cookies & active logins", "Saved passwords", "Cache", "Offline website data", "Site settings", "Clear data" },
    };

    String[][] OPTIONS_MOZILLA = {
        { "About " + BRAND_NAME },
        { "FAQs" },
        { "Give feedback" },
        { "Show product announcements" },
        { "Data choices" },
        { BRAND_NAME + " Health Report", "Shares data with Mozilla about your browser health and helps you understand your browser performance" },
        { "View my Health Report" },
    };

    










    public void setupSettingsMap(Map<String, List<String[]>> settingsMap) {
        settingsMap.put("Customize", new ArrayList<String[]>(Arrays.asList(OPTIONS_CUSTOMIZE)));
        settingsMap.put("Display", new ArrayList<String[]>(Arrays.asList(OPTIONS_DISPLAY)));
        settingsMap.put("Privacy", new ArrayList<String[]>(Arrays.asList(OPTIONS_PRIVACY)));
        settingsMap.put("Mozilla", new ArrayList<String[]>(Arrays.asList(OPTIONS_MOZILLA)));
    }

    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    public void testSettingsMenuItems() {
        blockForGeckoReady();
        mMidWidth = mDriver.getGeckoWidth()/2;
        mMidHeight = mDriver.getGeckoHeight()/2;

        Map<String, List<String[]>> settingsMenuItems = new HashMap<String, List<String[]>>();
        setupSettingsMap(settingsMenuItems);

        
        addConditionalSettings(settingsMenuItems);

        selectMenuItem("Settings");
        waitForText("Settings");

        
        mActions.sendSpecialKey(Actions.SpecialKey.BACK);

        
        waitForText("Enter Search");
        verifyUrl("about:home");

        selectMenuItem("Settings");
        waitForText("Settings");

        checkForSync(mDevice);

        checkMenuHierarchy(settingsMenuItems);
    }

    





    public void checkForSync(Device device) {
        if (device.type.equals("tablet")) {
            
            String customizeString = "^Customize$";
            waitForEnabledText(customizeString);
            mSolo.clickOnText(customizeString);
        }
        mAsserter.ok(mSolo.waitForText("Sync"), "Waiting for Sync option", "The Sync option is present");
    }

    



    public void addConditionalSettings(Map<String, List<String[]>> settingsMap) {
        try {
            ClassLoader classLoader = getActivity().getClassLoader();
            Class appConstants = classLoader.loadClass("org.mozilla.gecko.AppConstants");

            
            Field releaseBuildField = appConstants.getField("RELEASE_BUILD");
            boolean releaseBuild = releaseBuildField.getBoolean(appConstants);
            if (!releaseBuild) {
                
                String[] textReflowUi = { "Text reflow" };
                settingsMap.get("Display").add(textReflowUi);

                
                String[] networkReportingUi = { "Mozilla location services", "Help improve geolocation services for the Open Web by letting " + BRAND_NAME + " collect and send anonymous cellular tower data" };
                settingsMap.get("Mozilla").add(networkReportingUi);

            }

            
            Field autoUpdateField = appConstants.getField("MOZ_UPDATER");
            boolean autoUpdate = autoUpdateField.getBoolean(appConstants);
            if (autoUpdate) {
                String[] autoUpdateUi = { "Automatic updates", "Only over Wi-Fi", "Enabled", "Only over Wi-Fi", "Disabled" };
                settingsMap.get("Customize").add(autoUpdateUi);
            }

            
            Field crashReportingField = appConstants.getField("MOZ_CRASHREPORTER");
            boolean crashReporter = crashReportingField.getBoolean(appConstants);
            if (crashReporter) {
                String[] crashReporterUi = { "Crash Reporter", BRAND_NAME + " submits crash reports to help Mozilla make your browser more stable and secure" };
                settingsMap.get("Mozilla").add(crashReporterUi);
            }

            
            Field telemetryField = appConstants.getField("MOZ_TELEMETRY_REPORTING");
            boolean telemetry = telemetryField.getBoolean(appConstants);
            if (telemetry) {
                String[] telemetryUi = { "Telemetry", "Shares performance, usage, hardware and customization data about your browser with Mozilla to help us make " + BRAND_NAME + " better" };
                settingsMap.get("Mozilla").add(telemetryUi);
            }
        } catch (ClassNotFoundException e) {
            mAsserter.ok(false, "Class not found in setting conditional settings", e.toString());

        } catch (NoSuchFieldException e) {
            mAsserter.ok(false, "Field not found in setting conditional settings", e.toString());

        } catch (IllegalAccessException e) {
            mAsserter.ok(false, "Field cannot be accessed in setting conditional settings", e.toString());

        }
    }

    public void checkMenuHierarchy(Map<String, List<String[]>> settingsMap) {
        
        for (Entry<String, List<String[]>> e : settingsMap.entrySet()) {
            String section = "^" + e.getKey() + "$";
            List<String[]> sectionItems = e.getValue();

            waitForEnabledText(section);
            mSolo.clickOnText(section);

            
            for (String[] item : sectionItems) {
                int itemLen = item.length;

                
                mAsserter.ok(item.length > 0, "Section-item", "Each item must at least have a title");

                
                String itemTitle = "^" + item[0] + "$";
                boolean foundText = waitExtraForText(itemTitle);
                mAsserter.ok(foundText, "Waiting for settings item " + itemTitle + " in section " + section,
                             "The " + itemTitle + " option is present in section " + section);
                
                if (itemLen > 1) {
                    String itemDefault = "^" + item[1] + "$";
                    foundText = waitExtraForText(itemDefault);
                    mAsserter.ok(foundText, "Waiting for settings item default " + itemDefault
                                 + " in section " + section,
                                 "The " + itemDefault + " default is present in section " + section);
                }
                
                if (itemLen > 2) {
                    waitForEnabledText(itemTitle);
                    mSolo.clickOnText(itemTitle);
                    for (int i = 2; i < itemLen; i++) {
                        String itemChoice = "^" + item[i] + "$";
                        foundText = waitExtraForText(itemChoice);
                        mAsserter.ok(foundText, "Waiting for settings item choice " + itemChoice
                                     + " in section " + section,
                                     "The " + itemChoice + " choice is present in section " + section);
                    }
                    
                    if (waitForText("^Cancel$")) {
                        mSolo.clickOnText("^Cancel$");
                    } else {
                        
                        mActions.sendSpecialKey(Actions.SpecialKey.BACK);
                    }
                }
            }
            
            if (mDevice.type.equals("phone")) {
                
                mActions.sendSpecialKey(Actions.SpecialKey.BACK);
            }
        }
    }

    
    
    
    private boolean waitExtraForText(String txt) {
        boolean foundText = waitForText(txt);
        if (!foundText) {
            
            
            MotionEventHelper meh = new MotionEventHelper(getInstrumentation(), mDriver.getGeckoLeft(), mDriver.getGeckoTop());
            meh.dragSync(mMidWidth, mMidHeight+100, mMidWidth, mMidHeight-100);

            foundText = mSolo.waitForText(txt);
        }
        return foundText;
    }
}
