package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;

import android.app.Activity;
import android.view.View;
import android.widget.EditText;






public final class testInputUrlBar extends BaseTest {
    private Element mUrlBarEditElement;
    private EditText mUrlBarEditView;

    public void testInputUrlBar() {
        blockForGeckoReady();

        startEditingMode();
        assertUrlBarText("about:home");

        mActions.sendKeys("ab");
        assertUrlBarText("ab");

        mActions.sendKeys("cd");
        assertUrlBarText("abcd");

        mActions.sendSpecialKey(Actions.SpecialKey.LEFT);
        mActions.sendSpecialKey(Actions.SpecialKey.LEFT);

        
        mActions.sendKeys("");
        assertUrlBarText("abcd");

        mActions.sendKeys("ef");
        assertUrlBarText("abefcd");

        mActions.sendSpecialKey(Actions.SpecialKey.RIGHT);
        mActions.sendKeys("gh");
        assertUrlBarText("abefcghd");

        final EditText editText = mUrlBarEditView;
        runOnUiThreadSync(new Runnable() {
            public void run() {
                
                editText.setSelection(2);
            }
        });
        mActions.sendKeys("op");
        assertUrlBarText("abopefcghd");

        runOnUiThreadSync(new Runnable() {
            public void run() {
                
                editText.setSelection(6, 8);
            }
        });
        mActions.sendKeys("qr");
        assertUrlBarText("abopefqrhd");

        runOnUiThreadSync(new Runnable() {
            public void run() {
                
                editText.setSelection(4,2);
            }
        });
        mActions.sendKeys("st");
        assertUrlBarText("abstefqrhd");

        runOnUiThreadSync(new Runnable() {
            public void run() {
                editText.selectAll();
            }
        });
        mActions.sendKeys("uv");
        assertUrlBarText("uv");

        
        mActions.sendSpecialKey(Actions.SpecialKey.BACK);

        
        mActions.sendSpecialKey(Actions.SpecialKey.BACK);

        waitForText("Enter Search or Address");

        
        startEditingMode();
        assertUrlBarText("about:home");

        int width = mDriver.getGeckoWidth() / 2;
        int y = mDriver.getGeckoHeight() / 2;

        
        mActions.drag(width, 0, y, y);

        
        mSolo.clickOnView(mUrlBarEditView);
        mActions.sendKeys("yz");

        String yz = getUrlBarText();
        mAsserter.ok("yz".equals(yz), "Is the URL bar text \"yz\"?", yz);
    }

    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    private void startEditingMode() {
        focusUrlBar();

        mUrlBarEditElement = mDriver.findElement(getActivity(), URL_EDIT_TEXT_ID);
        final int id = mUrlBarEditElement.getId();
        mUrlBarEditView = (EditText) getActivity().findViewById(id);
    }

    private String getUrlBarText() {
        final String elementText = mUrlBarEditElement.getText();
        final String editText = mUrlBarEditView.getText().toString();
        mAsserter.is(editText, elementText, "Does URL bar editText == elementText?");

        return editText;
    }

    private void assertUrlBarText(String expectedText) {
        String actualText = getUrlBarText();
        mAsserter.is(actualText, expectedText, "Does URL bar actualText == expectedText?");
    }
}
