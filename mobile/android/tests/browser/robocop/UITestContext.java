



package org.mozilla.gecko.tests;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.Assert;
import org.mozilla.gecko.Driver;
import org.mozilla.gecko.tests.components.BaseComponent;

import android.app.Activity;
import android.app.Instrumentation;

import com.robotium.solo.Solo;




public interface UITestContext {

    public static enum ComponentType {
        ABOUTHOME,
        APPMENU,
        GECKOVIEW,
        TOOLBAR
    }

    public Activity getActivity();
    public Solo getSolo();
    public Assert getAsserter();
    public Driver getDriver();
    public Actions getActions();
    public Instrumentation getInstrumentation();
    public StringHelper getStringHelper();

    public void dumpLog(final String logtag, final String message);
    public void dumpLog(final String logtag, final String message, final Throwable t);

    


    public String getAbsoluteHostnameUrl(final String url);

    


    public String getAbsoluteIpUrl(final String url);

    public BaseComponent getComponent(final ComponentType type);
}
