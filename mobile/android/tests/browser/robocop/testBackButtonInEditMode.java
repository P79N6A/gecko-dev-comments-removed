package org.mozilla.gecko.tests;

import org.mozilla.gecko.tests.helpers.GeckoHelper;

import android.view.View;




public class testBackButtonInEditMode extends UITest {
    public void testBackButtonInEditMode() {
        GeckoHelper.blockForReady();

        
        mToolbar.enterEditingMode()
                .assertIsUrlEditTextSelected();
        checkBackPressInEditMode();
        checkExitUsingBackButton();

        
        mToolbar.enterEditingMode()
                .enterUrl("dummy")
                .assertIsUrlEditTextSelected();
        checkBackPressInEditMode();
        checkExitUsingBackButton();

        
        mToolbar.enterEditingMode()
                .assertIsUrlEditTextSelected();
        mAboutHome.swipeToPanelOnLeft();
        mToolbar.assertIsUrlEditTextNotSelected()
                .assertIsEditing();
        checkExitUsingBackButton();
    }

    private void checkBackPressInEditMode() {
        
        getSolo().goBack();
        mToolbar.assertIsUrlEditTextNotSelected()
                .assertIsEditing();
    }

    private void checkExitUsingBackButton() {
        getSolo().goBack();
        mToolbar.assertIsNotEditing();
    }
}
