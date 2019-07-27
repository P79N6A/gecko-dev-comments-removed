



package org.mozilla.gecko.tests.components;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.fAssertEquals;
import static org.mozilla.gecko.tests.helpers.AssertionHelper.fAssertFalse;
import static org.mozilla.gecko.tests.helpers.AssertionHelper.fAssertNotNull;
import static org.mozilla.gecko.tests.helpers.AssertionHelper.fAssertTrue;

import java.util.List;

import org.mozilla.gecko.R;
import org.mozilla.gecko.menu.MenuItemActionBar;
import org.mozilla.gecko.menu.MenuItemDefault;
import org.mozilla.gecko.tests.UITestContext;
import org.mozilla.gecko.tests.helpers.DeviceHelper;
import org.mozilla.gecko.tests.helpers.WaitHelper;
import org.mozilla.gecko.util.HardwareUtils;

import android.view.View;

import com.jayway.android.robotium.solo.Condition;
import com.jayway.android.robotium.solo.RobotiumUtils;
import com.jayway.android.robotium.solo.Solo;




public class AppMenuComponent extends BaseComponent {
    private static final long MAX_WAITTIME_FOR_MENU_UPDATE_IN_MS = 1000L;

    private Boolean hasLegacyMenu = null;

    public enum MenuItem {
        FORWARD(R.string.forward),
        NEW_TAB(R.string.new_tab),
        PAGE(R.string.page),
        RELOAD(R.string.reload);

        private final int resourceID;
        private String stringResource;

        MenuItem(final int resourceID) {
            this.resourceID = resourceID;
        }

        public String getString(final Solo solo) {
            if (stringResource == null) {
                stringResource = solo.getString(resourceID);
            }

            return stringResource;
        }
    };

    public enum PageMenuItem {
        SAVE_AS_PDF(R.string.save_as_pdf);

        private static final MenuItem PARENT_MENU = MenuItem.PAGE;

        private final int resourceID;
        private String stringResource;

        PageMenuItem(final int resourceID) {
            this.resourceID = resourceID;
        }

        public String getString(final Solo solo) {
            if (stringResource == null) {
                stringResource = solo.getString(resourceID);
            }

            return stringResource;
        }
    };

    public AppMenuComponent(final UITestContext testContext) {
        super(testContext);
    }

    private void assertMenuIsNotOpen() {
        fAssertFalse("Menu is not open", isMenuOpen());
    }

    







    private boolean hasLegacyMenu() {
        if (hasLegacyMenu == null) {
            hasLegacyMenu = findAppMenuItemView(MenuItem.PAGE.getString(mSolo)) == null;
        }

        return hasLegacyMenu;
    }

    public void assertMenuItemIsDisabledAndVisible(PageMenuItem pageMenuItem) {
        openAppMenu();

        if (!hasLegacyMenu()) {
            
            final View parentMenuItemView = findAppMenuItemView(MenuItem.PAGE.getString(mSolo));
            if (parentMenuItemView.isEnabled()) {
                fAssertTrue("The parent 'page' menu item is enabled", parentMenuItemView.isEnabled());
                fAssertEquals("The parent 'page' menu item is visible", View.VISIBLE,
                        parentMenuItemView.getVisibility());

                
                pressMenuItem(MenuItem.PAGE.getString(mSolo));

                final View pageMenuItemView = findAppMenuItemView(pageMenuItem.getString(mSolo));
                fAssertNotNull("The page menu item is not null", pageMenuItemView);
                fAssertFalse("The page menu item is not enabled", pageMenuItemView.isEnabled());
                fAssertEquals("The page menu item is visible", View.VISIBLE, pageMenuItemView.getVisibility());
            } else {
                fAssertFalse("The parent 'page' menu item is not enabled", parentMenuItemView.isEnabled());
                fAssertEquals("The parent 'page' menu item is visible", View.VISIBLE, parentMenuItemView.getVisibility());
            }
        } else {
            
            final View pageMenuItemView = findAppMenuItemView(pageMenuItem.getString(mSolo));
            fAssertFalse("The page menu item is not enabled", pageMenuItemView.isEnabled());
            fAssertEquals("The page menu item is visible", View.VISIBLE, pageMenuItemView.getVisibility());
        }

        
        mSolo.goBack();
    }

    private View getOverflowMenuButtonView() {
        return mSolo.getView(R.id.menu);
    }

    






    private View findAppMenuItemView(String text) {
        mSolo.waitForText(text, 1, MAX_WAITTIME_FOR_MENU_UPDATE_IN_MS);

        final List<View> views = mSolo.getViews();

        final List<MenuItemActionBar> menuItemActionBarList = RobotiumUtils.filterViews(MenuItemActionBar.class, views);
        for (MenuItemActionBar menuItem : menuItemActionBarList) {
            if (menuItem.getContentDescription().equals(text)) {
                return menuItem;
            }
        }

        final List<MenuItemDefault> menuItemDefaultList = RobotiumUtils.filterViews(MenuItemDefault.class, views);
        for (MenuItemDefault menuItem : menuItemDefaultList) {
            if (menuItem.getText().equals(text)) {
                return menuItem;
            }
        }

        return null;
    }

    






    private void pressLegacyMenuItem(final String menuItemTitle) {
        mSolo.clickOnMenuItem(menuItemTitle, true);
    }

    private void pressMenuItem(final String menuItemTitle) {
        fAssertTrue("Menu is open", isMenuOpen(menuItemTitle));

        if (!hasLegacyMenu()) {
            final View menuItemView = findAppMenuItemView(menuItemTitle);

            fAssertTrue(String.format("The menu item %s is enabled", menuItemTitle), menuItemView.isEnabled());
            fAssertEquals(String.format("The menu item %s is visible", menuItemTitle), View.VISIBLE,
                    menuItemView.getVisibility());

            mSolo.clickOnView(menuItemView);
        } else {
            pressLegacyMenuItem(menuItemTitle);
        }
    }

    private void pressSubMenuItem(final String parentMenuItemTitle, final String childMenuItemTitle) {
        openAppMenu();

        if (!hasLegacyMenu()) {
            pressMenuItem(parentMenuItemTitle);

            
            pressMenuItem(childMenuItemTitle);
        } else {
            pressLegacyMenuItem(childMenuItemTitle);
        }
    }

    public void pressMenuItem(MenuItem menuItem) {
        openAppMenu();
        pressMenuItem(menuItem.getString(mSolo));
    }

    public void pressMenuItem(final PageMenuItem pageMenuItem) {
        pressSubMenuItem(PageMenuItem.PARENT_MENU.getString(mSolo), pageMenuItem.getString(mSolo));
    }

    private void openAppMenu() {
        assertMenuIsNotOpen();

        
        
        if (HardwareUtils.hasMenuButton() || DeviceHelper.isTablet()) {
            mSolo.sendKey(Solo.MENU);
        } else {
            pressOverflowMenuButton();
        }

        waitForMenuOpen();
    }

    private void pressOverflowMenuButton() {
        final View overflowMenuButton = getOverflowMenuButtonView();

        fAssertTrue("The overflow menu button is enabled", overflowMenuButton.isEnabled());
        fAssertEquals("The overflow menu button is visible", View.VISIBLE, overflowMenuButton.getVisibility());

        mSolo.clickOnView(overflowMenuButton, true);
    }

    




    private boolean isMenuOpen() {
        return isMenuOpen(MenuItem.NEW_TAB.getString(mSolo));
    }

    






    private boolean isMenuOpen(String menuItemTitle) {
        return mSolo.searchText(menuItemTitle);
    }

    private void waitForMenuOpen() {
        WaitHelper.waitFor("menu to open", new Condition() {
            @Override
            public boolean isSatisfied() {
                return isMenuOpen();
            }
        });
    }
}
