



package org.mozilla.gecko.tests.components;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.tests.UITestContext;

import android.app.Activity;

import com.jayway.android.robotium.solo.Solo;








public abstract class BaseComponent {
    protected final UITestContext mTestContext;
    protected final Activity mActivity;
    protected final Solo mSolo;
    protected final Actions mActions;

    public BaseComponent(final UITestContext testContext) {
        mTestContext = testContext;
        mActivity = mTestContext.getActivity();
        mSolo = mTestContext.getSolo();
        mActions = mTestContext.getActions();
    }
}
