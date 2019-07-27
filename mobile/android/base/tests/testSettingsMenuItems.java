package org.mozilla.gecko.tests;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.util.HardwareUtils;




public class testSettingsMenuItems extends PixelTest {
    
    String[] PATH_CUSTOMIZE;
    String[][] OPTIONS_CUSTOMIZE;

    
    String[] PATH_HOME;
    String[][] OPTIONS_HOME;

    
    String[] PATH_DISPLAY;
    String[] TITLE_BAR_LABEL_ARR;
    String[][] OPTIONS_DISPLAY;

    
    String[] PATH_PRIVACY;
    String[] TRACKING_PROTECTION_LABEL_ARR;
    String[] MANAGE_LOGINS_ARR;
    String[][] OPTIONS_PRIVACY;

    
    String[] PATH_MOZILLA;
    String[][] OPTIONS_MOZILLA;

    
    String[] PATH_DEVELOPER;
    String[][] OPTIONS_DEVELOPER;

    











    public void setupSettingsMap(Map<String[], List<String[]>> settingsMap) {
        











        PATH_CUSTOMIZE = new String[] { mStringHelper.CUSTOMIZE_SECTION_LABEL };
        OPTIONS_CUSTOMIZE = new String[][] {
                { "Home" },
                { "Search", "", "Show search suggestions", "Installed search engines"},
                { mStringHelper.TABS_LABEL, "Don't restore after quitting " + mStringHelper.BRAND_NAME, "Always restore", "Don't restore after quitting " + mStringHelper.BRAND_NAME },
                { mStringHelper.IMPORT_FROM_ANDROID_LABEL, "", "Bookmarks", "History", "Import" },
        };

        PATH_HOME = new String[] { mStringHelper.CUSTOMIZE_SECTION_LABEL, "Home" };
        OPTIONS_HOME = new String[][] {
                { "Panels" },
                { "Automatic updates", "Enabled", "Enabled", "Only over Wi-Fi" },
        };

        PATH_DISPLAY = new String[] { mStringHelper.DISPLAY_SECTION_LABEL };
        TITLE_BAR_LABEL_ARR = new String[] { mStringHelper.TITLE_BAR_LABEL, mStringHelper.SHOW_PAGE_ADDRESS_LABEL,
                mStringHelper.SHOW_PAGE_TITLE_LABEL, mStringHelper.SHOW_PAGE_ADDRESS_LABEL };
        OPTIONS_DISPLAY = new String[][] {
                { mStringHelper.TEXT_SIZE_LABEL },
                TITLE_BAR_LABEL_ARR,
                { mStringHelper.SCROLL_TITLE_BAR_LABEL, "Hide the " + mStringHelper.BRAND_NAME + " title bar when scrolling down a page" },
                { "Advanced" },
                { mStringHelper.CHARACTER_ENCODING_LABEL, "Don't show menu", "Show menu", "Don't show menu" },
                { mStringHelper.PLUGINS_LABEL, "Tap to play", "Enabled", "Tap to play", "Disabled" },
        };

        PATH_PRIVACY = new String[] { mStringHelper.PRIVACY_SECTION_LABEL };
        TRACKING_PROTECTION_LABEL_ARR = new String[] { mStringHelper.TRACKING_PROTECTION_LABEL };
        MANAGE_LOGINS_ARR = new String[] { mStringHelper.MANAGE_LOGINS_LABEL };
        OPTIONS_PRIVACY = new String[][] {
                TRACKING_PROTECTION_LABEL_ARR,
                { mStringHelper.DNT_LABEL },
                { mStringHelper.COOKIES_LABEL, "Enabled", "Enabled, excluding 3rd party", "Disabled" },
                { mStringHelper.REMEMBER_PASSWORDS_LABEL },
                MANAGE_LOGINS_ARR,
                { mStringHelper.MASTER_PASSWORD_LABEL },
                { mStringHelper.CLEAR_PRIVATE_DATA_LABEL, "", "Browsing history", "Search history", "Downloads", "Form history", "Cookies & active logins", "Saved passwords", "Cache", "Offline website data", "Site settings", "Clear data" },
        };

        PATH_MOZILLA = new String[] { mStringHelper.MOZILLA_SECTION_LABEL };
        OPTIONS_MOZILLA = new String[][] {
                { mStringHelper.ABOUT_LABEL },
                { mStringHelper.FAQS_LABEL },
                { mStringHelper.FEEDBACK_LABEL },
                { "Data choices" },
                { mStringHelper.HEALTH_REPORT_LABEL, "Shares data with Mozilla about your browser health and helps you understand your browser performance" },
                { mStringHelper.MY_HEALTH_REPORT_LABEL },
        };

        PATH_DEVELOPER = new String[] { mStringHelper.DEVELOPER_TOOLS_SECTION_LABEL };
        OPTIONS_DEVELOPER = new String[][] {
                { mStringHelper.PAINT_FLASHING_LABEL },
                { mStringHelper.REMOTE_DEBUGGING_LABEL },
                { mStringHelper.LEARN_MORE_LABEL },
        };

        settingsMap.put(PATH_CUSTOMIZE, new ArrayList<String[]>(Arrays.asList(OPTIONS_CUSTOMIZE)));
        settingsMap.put(PATH_HOME, new ArrayList<String[]>(Arrays.asList(OPTIONS_HOME)));
        settingsMap.put(PATH_DISPLAY, new ArrayList<String[]>(Arrays.asList(OPTIONS_DISPLAY)));
        settingsMap.put(PATH_PRIVACY, new ArrayList<String[]>(Arrays.asList(OPTIONS_PRIVACY)));
        settingsMap.put(PATH_MOZILLA, new ArrayList<String[]>(Arrays.asList(OPTIONS_MOZILLA)));
        settingsMap.put(PATH_DEVELOPER, new ArrayList<String[]>(Arrays.asList(OPTIONS_DEVELOPER)));
    }

    public void testSettingsMenuItems() {
        blockForGeckoReady();

        Map<String[], List<String[]>> settingsMenuItems = new HashMap<String[], List<String[]>>();
        setupSettingsMap(settingsMenuItems);

        
        updateConditionalSettings(settingsMenuItems);

        selectMenuItem(mStringHelper.SETTINGS_LABEL);
        mAsserter.ok(mSolo.waitForText(mStringHelper.SETTINGS_LABEL),
                "The Settings menu did not load", mStringHelper.SETTINGS_LABEL);

        
        mActions.sendSpecialKey(Actions.SpecialKey.BACK);

        
        mAsserter.ok(mSolo.waitForText(mStringHelper.TITLE_PLACE_HOLDER), "about:home did not load",
                mStringHelper.TITLE_PLACE_HOLDER);
        verifyUrl(mStringHelper.ABOUT_HOME_URL);

        selectMenuItem(mStringHelper.SETTINGS_LABEL);
        mAsserter.ok(mSolo.waitForText(mStringHelper.SETTINGS_LABEL),
                "The Settings menu did not load", mStringHelper.SETTINGS_LABEL);

        checkForSync(mDevice);

        checkMenuHierarchy(settingsMenuItems);
    }

    





    public void checkForSync(Device device) {
        mAsserter.ok(mSolo.waitForText(mStringHelper.SYNC_LABEL), "Waiting for Sync option",
                mStringHelper.SYNC_LABEL);
    }

    



    public void updateConditionalSettings(Map<String[], List<String[]>> settingsMap) {
        
        if (!AppConstants.RELEASE_BUILD) {
            
            String[] textReflowUi = { mStringHelper.TEXT_REFLOW_LABEL };
            settingsMap.get(PATH_DISPLAY).add(textReflowUi);

            if (AppConstants.MOZ_STUMBLER_BUILD_TIME_ENABLED) {
                
                String[] networkReportingUi = { "Mozilla Location Service", "Help Mozilla map the world! Share the approximate Wi-Fi and cellular location of your device to improve our geolocation service." };
                settingsMap.get(PATH_MOZILLA).add(networkReportingUi);

                String[] learnMoreUi = { "Learn more" };
                settingsMap.get(PATH_MOZILLA).add(learnMoreUi);
            }
        }

        if (!AppConstants.NIGHTLY_BUILD) {
            final List<String[]> privacy = settingsMap.get(PATH_PRIVACY);
            privacy.remove(TRACKING_PROTECTION_LABEL_ARR);
            privacy.remove(MANAGE_LOGINS_ARR);
        }

        
        if (AppConstants.MOZ_UPDATER) {
            String[] autoUpdateUi = { "Download updates automatically", "Only over Wi-Fi", "Always", "Only over Wi-Fi", "Never" };
            settingsMap.get(PATH_CUSTOMIZE).add(autoUpdateUi);
        }

        
        if (AppConstants.NIGHTLY_BUILD && AppConstants.MOZ_ANDROID_TAB_QUEUE) {
            String[] tabQueue = { mStringHelper.TAB_QUEUE_LABEL, "Prevent tabs from opening immediately, but open all queued tabs the next time " + mStringHelper.BRAND_NAME + " loads." };
            settingsMap.get(PATH_CUSTOMIZE).add(tabQueue);
        }

        
        if (AppConstants.MOZ_CRASHREPORTER) {
            String[] crashReporterUi = { "Crash Reporter", mStringHelper.BRAND_NAME + " submits crash reports to help Mozilla make your browser more stable and secure" };
            settingsMap.get(PATH_MOZILLA).add(crashReporterUi);
        }

        
        if (AppConstants.MOZ_TELEMETRY_REPORTING) {
            String[] telemetryUi = { "Telemetry", "Shares performance, usage, hardware and customization data about your browser with Mozilla to help us make " + mStringHelper.BRAND_NAME + " better" };
            settingsMap.get(PATH_MOZILLA).add(telemetryUi);
        }

        
        if (HardwareUtils.isTablet()) {
            settingsMap.get(PATH_DISPLAY).remove(TITLE_BAR_LABEL_ARR);
        }
    }

    public void checkMenuHierarchy(Map<String[], List<String[]>> settingsMap) {
        
        String section = null;
        for (Entry<String[], List<String[]>> e : settingsMap.entrySet()) {
            final String[] menuPath = e.getKey();

            for (String menuItem : menuPath) {
                section = "^" + menuItem + "$";

                waitForEnabledText(section);
                mSolo.clickOnText(section);
            }

            List<String[]> sectionItems = e.getValue();

            
            for (String[] item : sectionItems) {
                int itemLen = item.length;

                
                mAsserter.ok(item.length > 0, "Section-item", "Each item must at least have a title");

                
                String itemTitle = "^" + item[0] + "$";
                boolean foundText = waitForPreferencesText(itemTitle);

                mAsserter.ok(foundText, "Waiting for settings item " + itemTitle + " in section " + section,
                             "The " + itemTitle + " option is present in section " + section);
                
                if (itemLen > 1) {
                    String itemDefault = "^" + item[1] + "$";
                    foundText = waitForPreferencesText(itemDefault);
                    mAsserter.ok(foundText, "Waiting for settings item default " + itemDefault
                                 + " in section " + section,
                                 "The " + itemDefault + " default is present in section " + section);
                }
                
                if (itemLen > 2) {
                    waitForEnabledText(itemTitle);
                    mSolo.clickOnText(itemTitle);
                    for (int i = 2; i < itemLen; i++) {
                        String itemChoice = "^" + item[i] + "$";
                        foundText = waitForPreferencesText(itemChoice);
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
                int menuDepth = menuPath.length;
                while (menuDepth > 0) {
                    mActions.sendSpecialKey(Actions.SpecialKey.BACK);
                    menuDepth--;
                    
                    mSolo.sleep(150);
                }
            }
        }
    }
}
