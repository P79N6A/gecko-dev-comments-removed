



package org.mozilla.gecko.tests.helpers;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.fAssertNotNull;

import org.mozilla.gecko.tests.UITestContext;
import org.mozilla.gecko.tests.UITestContext.ComponentType;
import org.mozilla.gecko.tests.components.AppMenuComponent;
import org.mozilla.gecko.tests.components.ToolbarComponent;

import com.jayway.android.robotium.solo.Solo;





final public class NavigationHelper {
    private static UITestContext sContext;
    private static Solo sSolo;

    private static AppMenuComponent sAppMenu;
    private static ToolbarComponent sToolbar;

    protected static void init(final UITestContext context) {
        sContext = context;
        sSolo = context.getSolo();

        sAppMenu = (AppMenuComponent) context.getComponent(ComponentType.APPMENU);
        sToolbar = (ToolbarComponent) context.getComponent(ComponentType.TOOLBAR);
    }

    public static void enterAndLoadUrl(String url) {
        fAssertNotNull("url is not null", url);

        url = adjustUrl(url);
        sToolbar.enterEditingMode()
                .enterUrl(url)
                .commitEditingMode();
    }

    


    private static String adjustUrl(final String url) {
        fAssertNotNull("url is not null", url);

        if (url.startsWith("about:") || url.startsWith("chrome:")) {
            return url;
        }

        return sContext.getAbsoluteHostnameUrl(url);
    }

    public static void goBack() {
        if (DeviceHelper.isTablet()) {
            sToolbar.pressBackButton(); 
            return;
        }

        sToolbar.assertIsNotEditing();
        WaitHelper.waitForPageLoad(new Runnable() {
            @Override
            public void run() {
                
                
                
                sSolo.goBack();
            }
        });
    }

    public static void goForward() {
        if (DeviceHelper.isTablet()) {
            sToolbar.pressForwardButton(); 
            return;
        }

        sToolbar.assertIsNotEditing();
        WaitHelper.waitForPageLoad(new Runnable() {
            @Override
            public void run() {
                sAppMenu.pressMenuItem(AppMenuComponent.MenuItem.FORWARD);
            }
        });
    }

    public static void reload() {
        if (DeviceHelper.isTablet()) {
            sToolbar.pressReloadButton(); 
            return;
        }

        sToolbar.assertIsNotEditing();
        WaitHelper.waitForPageLoad(new Runnable() {
            @Override
            public void run() {
                sAppMenu.pressMenuItem(AppMenuComponent.MenuItem.RELOAD);
            }
        });
    }
}
