package org.mozilla.gecko.tests;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.*;
import static org.mozilla.gecko.tests.helpers.TextInputHelper.*;

import org.mozilla.gecko.tests.components.GeckoViewComponent.InputConnectionTest;
import org.mozilla.gecko.tests.helpers.*;

import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;




public class testInputConnection extends UITest {

    private static final String INITIAL_TEXT = "foo";

    public void testInputConnection() throws InterruptedException {
        GeckoHelper.blockForReady();

        NavigationHelper.enterAndLoadUrl(StringHelper.ROBOCOP_INPUT_URL + "#" + INITIAL_TEXT);
        mToolbar.assertTitle(StringHelper.ROBOCOP_INPUT_TITLE);

        mGeckoView.mTextInput
            .waitForInputConnection()
            .testInputConnection(new BasicInputConnectionTest());
    }

    private class BasicInputConnectionTest implements InputConnectionTest {
        @Override
        public void test(InputConnection ic, EditorInfo info) {
            
            assertText("Initial text matches URL hash", ic, INITIAL_TEXT);

            
            ic.setSelection(0, 3);
            assertSelection("Can set selection to range", ic, 0, 3);
            ic.setSelection(-3, 6);
            
            assertTextAndSelection("Can handle invalid range", ic, INITIAL_TEXT, 0, 3);
            ic.setSelection(3, 3);
            assertSelectionAt("Can collapse selection", ic, 3);
            ic.setSelection(4, 4);
            assertTextAndSelectionAt("Can handle invalid cursor", ic, INITIAL_TEXT, 3);

            
            ic.commitText("", 10); 
            assertTextAndSelectionAt("Can commit empty text", ic, "foo", 3);
            ic.commitText("bar", 1); 
            assertTextAndSelectionAt("Can commit text (select after)", ic, "foobar", 6);
            ic.commitText("foo", -1); 
            assertTextAndSelectionAt("Can commit text (select before)", ic, "foobarfoo", 5);

            
            ic.deleteSurroundingText(1, 0);
            assertTextAndSelectionAt("Can delete text before", ic, "foobrfoo", 4);
            ic.deleteSurroundingText(1, 1);
            assertTextAndSelectionAt("Can delete text before/after", ic, "foofoo", 3);
            ic.deleteSurroundingText(0, 10);
            assertTextAndSelectionAt("Can delete text after", ic, "foo", 3);
            ic.deleteSurroundingText(0, 0);
            assertTextAndSelectionAt("Can delete empty text", ic, "foo", 3);

            
            ic.setComposingText("foo", 1);
            assertTextAndSelectionAt("Can start composition", ic, "foofoo", 6);
            ic.setComposingText("", 1);
            assertTextAndSelectionAt("Can set empty composition", ic, "foo", 3);
            ic.setComposingText("bar", 1);
            assertTextAndSelectionAt("Can update composition", ic, "foobar", 6);

            
            ic.finishComposingText();
            assertTextAndSelectionAt("Can finish composition", ic, "foobar", 6);

            
            fAssertEquals("Can retrieve text before cursor", "bar", ic.getTextBeforeCursor(3, 0));

            
            fAssertEquals("Can retrieve text after cursor", "", ic.getTextAfterCursor(3, 0));
        }
    }
}
