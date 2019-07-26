



package org.mozilla.gecko.tests;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.Assert;
import org.mozilla.gecko.Driver;
import org.mozilla.gecko.tests.components.BaseComponent;

import com.jayway.android.robotium.solo.Solo;

import android.app.Activity;
import android.app.Instrumentation;




public interface UITestContext {

    public static enum ComponentType {
        ABOUTHOME,
        TOOLBAR
    }

    public Activity getActivity();
    public Solo getSolo();
    public Assert getAsserter();
    public Driver getDriver();
    public Actions getActions();
    public Instrumentation getInstrumentation();

    public void dumpLog(final String message);
    public void dumpLog(final String message, final Throwable t);

    


    public String getAbsoluteHostnameUrl(final String url);

    


    public String getAbsoluteIpUrl(final String url);

    public BaseComponent getComponent(final ComponentType type);
}
