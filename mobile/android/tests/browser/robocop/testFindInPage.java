



package org.mozilla.gecko.tests;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.fFail;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.Element;
import org.mozilla.gecko.R;

import org.mozilla.gecko.EventDispatcher;
import org.mozilla.gecko.util.GeckoEventListener;

import org.json.JSONObject;

import com.jayway.android.robotium.solo.Condition;

public class testFindInPage extends JavascriptTest implements GeckoEventListener {
    private static final int WAIT_FOR_CONDITION_MS = 3000;

    protected Element next, close;

    public testFindInPage() {
        super("testFindInPage.js");
    }

    @Override
    public void handleMessage(String event, final JSONObject message) {
        if (event.equals("Test:FindInPage")) {
            try {
                final String text = message.getString("text");
                final int nrOfMatches = Integer.parseInt(message.getString("nrOfMatches"));
                findText(text, nrOfMatches);
            } catch (Exception e) {
                fFail("Can't extract find query from JSON");
            }
        }

        if (event.equals("Test:CloseFindInPage")) {
            try {
                close.click();
            } catch (Exception e) {
                fFail("FindInPage prompt not opened");
            }
        }
    }

    @Override
    public void setUp() throws Exception {
        super.setUp();

        EventDispatcher.getInstance().registerGeckoThreadListener(this,
                "Test:FindInPage",
                "Test:CloseFindInPage");
    }

    @Override
    public void tearDown() throws Exception {
        super.tearDown();

        EventDispatcher.getInstance().unregisterGeckoThreadListener(this,
                "Test:FindInPage",
                "Test:CloseFindInPage");
    }

    public void findText(String text, int nrOfMatches){
        selectMenuItem(mStringHelper.FIND_IN_PAGE_LABEL);
        close = mDriver.findElement(getActivity(), R.id.find_close);
        boolean success = waitForCondition ( new Condition() {
            @Override
            public boolean isSatisfied() {
                next = mDriver.findElement(getActivity(), R.id.find_next);
                if (next != null) {
                    return true;
                } else {
                    return false;
                }
            }
        }, WAIT_FOR_CONDITION_MS);
        mAsserter.ok(success, "Looking for the next search match button in the Find in Page UI", "Found the next match button");

        
        
        mSolo.sleep(500);

        mActions.sendKeys(text);
        mActions.sendSpecialKey(Actions.SpecialKey.ENTER);

        
        for (int i=1;i < nrOfMatches;i++) {
            success = waitForCondition ( new Condition() {
                @Override
                public boolean isSatisfied() {
                    if (next.click()) {
                        return true;
                    } else {
                        return false;
                    }
                }
            }, WAIT_FOR_CONDITION_MS);
            mSolo.sleep(500); 
            mAsserter.ok(success, "Checking if the next button was clicked", "button was clicked");
        }
    }
}