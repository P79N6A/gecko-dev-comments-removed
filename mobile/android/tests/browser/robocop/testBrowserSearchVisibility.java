



package org.mozilla.gecko.tests;

import android.support.v4.app.Fragment;
import android.view.KeyEvent;
import android.view.View;

import com.jayway.android.robotium.solo.Condition;






public class testBrowserSearchVisibility extends BaseTest {
    public void testSearchSuggestions() {
        blockForGeckoReady();

        focusUrlBar();

        
        assertBrowserSearchVisibility(false);

        mActions.sendKeys("a");

        
        assertBrowserSearchVisibility(true);

        mActions.sendKeys("b");

        
        assertBrowserSearchVisibility(true);

        mActions.sendKeyCode(KeyEvent.KEYCODE_DEL);

        
        assertBrowserSearchVisibility(true);

        mActions.sendKeyCode(KeyEvent.KEYCODE_DEL);

        
        assertBrowserSearchVisibility(false);
    }

    private void assertBrowserSearchVisibility(final boolean isVisible) {
        waitForCondition(new Condition() {
            @Override
            public boolean isSatisfied() {
                final Fragment browserSearch = getBrowserSearch();

                
                
                
                if (browserSearch == null)
                    return !isVisible;

                final View v = browserSearch.getView();
                if (isVisible && v != null && v.getVisibility() == View.VISIBLE)
                    return true;

                return false;
            }
        }, 5000);
    }
}

