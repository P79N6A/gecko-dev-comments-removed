



package org.mozilla.gecko.tests.helpers;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.*;

import org.mozilla.gecko.tests.components.ToolbarComponent;
import org.mozilla.gecko.tests.UITestContext;
import org.mozilla.gecko.tests.UITestContext.ComponentType;

import com.jayway.android.robotium.solo.Solo;

import android.text.TextUtils;





final public class NavigationHelper {
    private static UITestContext sContext;
    private static Solo sSolo;

    private static ToolbarComponent sToolbar;

    public static void init(final UITestContext context) {
        sContext = context;
        sSolo = context.getSolo();

        sToolbar = (ToolbarComponent) context.getComponent(ComponentType.TOOLBAR);
    }

    public static void enterAndLoadUrl(String url) {
        assertNotNull("url is not null", url);

        url = adjustUrl(url);
        sToolbar.enterEditingMode()
                .enterUrl(url)
                .commitEditingMode();
    }

    


    private static String adjustUrl(final String url) {
        assertNotNull("url is not null", url);

        if (!url.startsWith("about:")) {
            return sContext.getAbsoluteHostnameUrl(url);
        }

        return url;
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
                
                throw new UnsupportedOperationException("Not yet implemented.");
            }
        });
    }

    public static void reload() {
        
        
        
        throw new UnsupportedOperationException("Not yet implemented.");
    }
}
