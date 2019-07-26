



package org.mozilla.gecko.tests.components;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.fAssertEquals;
import static org.mozilla.gecko.tests.helpers.AssertionHelper.fAssertFalse;
import static org.mozilla.gecko.tests.helpers.AssertionHelper.fAssertTrue;

import java.util.List;

import org.mozilla.gecko.R;
import org.mozilla.gecko.menu.MenuItemActionBar;
import org.mozilla.gecko.menu.MenuItemDefault;
import org.mozilla.gecko.tests.UITestContext;
import org.mozilla.gecko.tests.helpers.WaitHelper;
import org.mozilla.gecko.util.HardwareUtils;

import android.view.View;

import com.jayway.android.robotium.solo.Condition;
import com.jayway.android.robotium.solo.RobotiumUtils;
import com.jayway.android.robotium.solo.Solo;




public class AppMenuComponent extends BaseComponent {
    public enum MenuItem {
        FORWARD(R.string.forward),
        NEW_TAB(R.string.new_tab),
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

    public AppMenuComponent(final UITestContext testContext) {
        super(testContext);
    }

    private void assertMenuIsNotOpen() {
        fAssertFalse("Menu is not open", isMenuOpen());
    }

    private View getOverflowMenuButtonView() {
        return mSolo.getView(R.id.menu);
    }

    






    private View findAppMenuItemView(String text) {
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

    public void pressMenuItem(MenuItem menuItem) {
        openAppMenu();

        final String text = menuItem.getString(mSolo);
        final View menuItemView = findAppMenuItemView(text);

        if (menuItemView != null) {
            fAssertTrue("The menu item is enabled", menuItemView.isEnabled());
            fAssertEquals("The menu item is visible", View.VISIBLE, menuItemView.getVisibility());

            mSolo.clickOnView(menuItemView);
        } else {
            
            
            
            
            
            
            mSolo.clickOnMenuItem(text, true);
        }
    }

    private void openAppMenu() {
        assertMenuIsNotOpen();

        if (HardwareUtils.hasMenuButton()) {
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
        
        
        return mSolo.searchText(MenuItem.NEW_TAB.getString(mSolo));
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
