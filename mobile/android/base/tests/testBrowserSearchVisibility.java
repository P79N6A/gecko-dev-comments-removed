package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;
import android.app.Activity;
import android.content.Context;
import android.support.v4.app.Fragment;
import android.view.View;
import android.view.ViewGroup;
import android.view.KeyEvent;
import android.widget.TextView;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.RuntimeException;
import java.util.ArrayList;
import java.util.HashMap;






public class testBrowserSearchVisibility extends BaseTest {
    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

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
        waitForTest(new BooleanTest() {
            @Override
            public boolean test() {
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

