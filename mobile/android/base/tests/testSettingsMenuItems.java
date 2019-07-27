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
    











    
    String[][] PATH_CUSTOMIZE = { { StringHelper.CUSTOMIZE_SECTION_LABEL, "Home, search, tabs, import"} };
    String[][] OPTIONS_CUSTOMIZE = {
        { "Home" },
        { "Search", "", "Show search suggestions", "Installed search engines"},
        { StringHelper.TABS_LABEL, "Don't restore after quitting " + StringHelper.BRAND_NAME, "Always restore", "Don't restore after quitting " + StringHelper.BRAND_NAME },
        { StringHelper.IMPORT_FROM_ANDROID_LABEL, "", "Bookmarks", "History", "Import" },
    };

    
    String[][] PATH_HOME = { { StringHelper.CUSTOMIZE_SECTION_LABEL }, { "Home", "Customize your homepage" } };
    String[][] OPTIONS_HOME = {
      { "Panels" },
      { "Automatic updates", "Enabled", "Enabled", "Only over Wi-Fi" },
    };

    
    String[][] PATH_DISPLAY = { { StringHelper.DISPLAY_SECTION_LABEL, "Text, title bar, full-screen browsing" } };
    final String[] TITLE_BAR_LABEL_ARR = { StringHelper.TITLE_BAR_LABEL, StringHelper.SHOW_PAGE_TITLE_LABEL,
        StringHelper.SHOW_PAGE_TITLE_LABEL, StringHelper.SHOW_PAGE_ADDRESS_LABEL };
    String[][] OPTIONS_DISPLAY = {
        { StringHelper.TEXT_SIZE_LABEL },
        TITLE_BAR_LABEL_ARR,
        { StringHelper.SCROLL_TITLE_BAR_LABEL, "Hide the " + StringHelper.BRAND_NAME + " title bar when scrolling down a page" },
        { "Advanced" },
        { StringHelper.CHARACTER_ENCODING_LABEL, "Don't show menu", "Show menu", "Don't show menu" },
        { StringHelper.PLUGINS_LABEL, "Tap to play", "Enabled", "Tap to play", "Disabled" },
    };

    
    String[][] PATH_PRIVACY = { { StringHelper.PRIVACY_SECTION_LABEL, "Control passwords, cookies, tracking, data" } };
    final String[] TRACKING_PROTECTION_LABEL_ARR = { StringHelper.TRACKING_PROTECTION_LABEL };
    String[][] OPTIONS_PRIVACY = {
        TRACKING_PROTECTION_LABEL_ARR,
        { StringHelper.DNT_LABEL },
        { StringHelper.COOKIES_LABEL, "Enabled", "Enabled, excluding 3rd party", "Disabled" },
        { StringHelper.REMEMBER_PASSWORDS_LABEL },
        { StringHelper.MASTER_PASSWORD_LABEL },
        { StringHelper.CLEAR_PRIVATE_DATA_LABEL, "", "Browsing history", "Downloads", "Form & search history", "Cookies & active logins", "Saved passwords", "Cache", "Offline website data", "Site settings", "Clear data" },
    };

    
    String[][] PATH_MOZILLA = { { StringHelper.MOZILLA_SECTION_LABEL, "About " + StringHelper.BRAND_NAME + ", FAQs, data choices" } };
    String[][] OPTIONS_MOZILLA = {
        { StringHelper.ABOUT_LABEL },
        { StringHelper.FAQS_LABEL },
        { StringHelper.FEEDBACK_LABEL },
        { "Data choices" },
        { StringHelper.HEALTH_REPORT_LABEL, "Shares data with Mozilla about your browser health and helps you understand your browser performance" },
        { StringHelper.MY_HEALTH_REPORT_LABEL },
    };

    
    String[][] PATH_DEVELOPER = { { StringHelper.DEVELOPER_TOOLS_SECTION_LABEL, "Remote debugging, paint flashing" } };
    String[][] OPTIONS_DEVELOPER = {
        { StringHelper.PAINT_FLASHING_LABEL },
        { StringHelper.REMOTE_DEBUGGING_LABEL },
        { StringHelper.LEARN_MORE_LABEL },
    };

    











    public void setupSettingsMap(Map<String[][], List<String[]>> settingsMap) {
        settingsMap.put(PATH_CUSTOMIZE, new ArrayList<String[]>(Arrays.asList(OPTIONS_CUSTOMIZE)));
        settingsMap.put(PATH_HOME, new ArrayList<String[]>(Arrays.asList(OPTIONS_HOME)));
        settingsMap.put(PATH_DISPLAY, new ArrayList<String[]>(Arrays.asList(OPTIONS_DISPLAY)));
        settingsMap.put(PATH_PRIVACY, new ArrayList<String[]>(Arrays.asList(OPTIONS_PRIVACY)));
        settingsMap.put(PATH_MOZILLA, new ArrayList<String[]>(Arrays.asList(OPTIONS_MOZILLA)));
        settingsMap.put(PATH_DEVELOPER, new ArrayList<String[]>(Arrays.asList(OPTIONS_DEVELOPER)));
    }

    public void testSettingsMenuItems() {
        blockForGeckoReady();

        Map<String[][], List<String[]>> settingsMenuItems = new HashMap<String[][], List<String[]>>();
        setupSettingsMap(settingsMenuItems);

        
        updateConditionalSettings(settingsMenuItems);

        selectMenuItem(StringHelper.SETTINGS_LABEL);
        mAsserter.ok(mSolo.waitForText(StringHelper.SETTINGS_LABEL),
                "The Settings menu did not load", StringHelper.SETTINGS_LABEL);

        
        mActions.sendSpecialKey(Actions.SpecialKey.BACK);

        
        mAsserter.ok(mSolo.waitForText(StringHelper.TITLE_PLACE_HOLDER), "about:home did not load",
                StringHelper.TITLE_PLACE_HOLDER);
        verifyUrl(StringHelper.ABOUT_HOME_URL);

        selectMenuItem(StringHelper.SETTINGS_LABEL);
        mAsserter.ok(mSolo.waitForText(StringHelper.SETTINGS_LABEL),
                "The Settings menu did not load", StringHelper.SETTINGS_LABEL);

        checkForSync(mDevice);

        checkMenuHierarchy(settingsMenuItems);
    }

    





    public void checkForSync(Device device) {
        mAsserter.ok(mSolo.waitForText(StringHelper.SYNC_LABEL), "Waiting for Sync option",
                StringHelper.SYNC_LABEL);
    }

    



    public void updateConditionalSettings(Map<String[][], List<String[]>> settingsMap) {
        
        if (!AppConstants.RELEASE_BUILD) {
            
            String[] textReflowUi = { StringHelper.TEXT_REFLOW_LABEL };
            settingsMap.get(PATH_DISPLAY).add(textReflowUi);

            if (AppConstants.MOZ_STUMBLER_BUILD_TIME_ENABLED) {
                
                String[] networkReportingUi = { "Mozilla Location Service", "Help Mozilla map the world! Share approximate Wi-Fi and cellular location of your device to improve our geolocation service" };
                settingsMap.get(PATH_MOZILLA).add(networkReportingUi);

                String[] learnMoreUi = { "Learn more" };
                settingsMap.get(PATH_MOZILLA).add(learnMoreUi);
            }
        }

        if (!AppConstants.NIGHTLY_BUILD) {
            settingsMap.get(PATH_PRIVACY).remove(TRACKING_PROTECTION_LABEL_ARR);
        }

        
        if (AppConstants.MOZ_UPDATER) {
            String[] autoUpdateUi = { "Download updates automatically", "Only over Wi-Fi", "Always", "Only over Wi-Fi", "Never" };
            settingsMap.get(PATH_CUSTOMIZE).add(autoUpdateUi);
        }

        
        if (AppConstants.NIGHTLY_BUILD && AppConstants.MOZ_ANDROID_TAB_QUEUE) {
            String[] tabQueue = { StringHelper.TAB_QUEUE_LABEL, "Prevent tabs from opening immediately, but open all queued tabs the next time " + StringHelper.BRAND_NAME + " loads." };
            settingsMap.get(PATH_CUSTOMIZE).add(tabQueue);
            final List<String[]> list = settingsMap.remove(PATH_CUSTOMIZE);
            PATH_CUSTOMIZE[0][1] = "Home, search, tabs, open later, import";
            settingsMap.put(PATH_CUSTOMIZE, list);
        }

        
        if (AppConstants.MOZ_CRASHREPORTER) {
            String[] crashReporterUi = { "Crash Reporter", StringHelper.BRAND_NAME + " submits crash reports to help Mozilla make your browser more stable and secure" };
            settingsMap.get(PATH_MOZILLA).add(crashReporterUi);
        }

        
        if (AppConstants.MOZ_TELEMETRY_REPORTING) {
            String[] telemetryUi = { "Telemetry", "Shares performance, usage, hardware and customization data about your browser with Mozilla to help us make " + StringHelper.BRAND_NAME + " better" };
            settingsMap.get(PATH_MOZILLA).add(telemetryUi);
        }

        
        if (HardwareUtils.isTablet()) {
            settingsMap.get(PATH_DISPLAY).remove(TITLE_BAR_LABEL_ARR);
        }
    }

    public void checkMenuHierarchy(Map<String[][], List<String[]>> settingsMap) {
        
        String section = null;
        for (Entry<String[][], List<String[]>> e : settingsMap.entrySet()) {
            final String[][] menuPath = e.getKey();

            for (String[] menuItem : menuPath) {
                final String item = menuItem[0];
                section = "^" + item + "$";

                waitForEnabledText(section);

                if(menuItem.length > 1) {
                    final String subtitle = menuItem[1];
                    boolean foundText = waitForEnabledText("^" + subtitle + "$");
                    mAsserter.ok(foundText, "Waiting for subtitle item " + subtitle + " for section " + item,
                            "Found the " + item + " subtitle " + subtitle);
                }

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
