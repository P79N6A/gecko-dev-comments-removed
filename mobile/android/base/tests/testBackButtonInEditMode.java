package org.mozilla.gecko.tests;

import org.mozilla.gecko.tests.helpers.GeckoHelper;

import android.view.View;




public class testBackButtonInEditMode extends UITest {
    public void testBackButtonInEditMode() {
        GeckoHelper.blockForReady();

        
        mToolbar.enterEditingMode()
                .assertIsUrlEditTextSelected();
        testBackPressInEditMode();
        testExitUsingBackButton();

        
        mToolbar.enterEditingMode()
                .enterUrl("dummy")
                .assertIsUrlEditTextSelected();
        testBackPressInEditMode();
        testExitUsingBackButton();

        
        mToolbar.enterEditingMode()
                .assertIsUrlEditTextSelected();
        mAboutHome.swipeToPanelOnLeft();
        mToolbar.assertIsUrlEditTextNotSelected()
                .assertIsEditing();
        testExitUsingBackButton();
    }

    public void testBackPressInEditMode() {
        
        getSolo().goBack();
        mToolbar.assertIsUrlEditTextNotSelected()
                .assertIsEditing();
    }

    public void testExitUsingBackButton() {
        getSolo().goBack();
        mToolbar.assertIsNotEditing();
    }
}
