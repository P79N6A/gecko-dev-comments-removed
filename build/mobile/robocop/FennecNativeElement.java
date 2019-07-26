



package org.mozilla.gecko;

import android.app.Activity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.TextSwitcher;
import android.app.Instrumentation;
import com.jayway.android.robotium.solo.Solo;
import java.util.List;

public class FennecNativeElement implements Element {
    private final Activity mActivity;
    private Integer mId;
    private Solo mSolo;
    
    private static final int MAX_WAIT_MS = 60000;

    public FennecNativeElement(Integer id, Activity activity, Solo solo) {
        mId = id;
        mActivity = activity;
        mSolo = solo;
    }

    public Integer getId() {
        return mId;
    }

    private boolean mClickSuccess;

    public boolean click() {
        mClickSuccess = false;
        RobocopUtils.runOnUiThreadSync(mActivity,
            new Runnable() {
                public void run() {
                    View view = (View)mActivity.findViewById(mId);
                    if (view != null) {
                        if (view.performClick()) {
                            mClickSuccess = true;
                        } else {
                            FennecNativeDriver.log(FennecNativeDriver.LogLevel.WARN,
                                "Robocop called click on an element with no listener");
                        }
                    } else {
                        FennecNativeDriver.log(FennecNativeDriver.LogLevel.ERROR,
                            "click: unable to find view "+mId);
                    }
                }
            });
        return mClickSuccess;
    }

    private Object mText;

    public String getText() {
        mText = null;
        RobocopUtils.runOnUiThreadSync(mActivity,
            new Runnable() {
                public void run() {
                    View v = mActivity.findViewById(mId);
                    if (v instanceof EditText) {
                        EditText et = (EditText)v;
                        mText = et.getEditableText();
                    } else if (v instanceof TextSwitcher) {
                        TextSwitcher ts = (TextSwitcher)v;
                        ts.getNextView();
                        mText = ((TextView)ts.getCurrentView()).getText();
                    } else if (v instanceof ViewGroup) {
                        ViewGroup vg = (ViewGroup)v;
                        for (int i = 0; i < vg.getChildCount(); i++) {
                            if (vg.getChildAt(i) instanceof TextView) {
                                mText = ((TextView)vg.getChildAt(i)).getText();
                            }
                        }
                    } else if (v instanceof TextView) {
                        mText = ((TextView)v).getText(); 
                    } else if (v == null) {
                        FennecNativeDriver.log(FennecNativeDriver.LogLevel.ERROR,
                            "getText: unable to find view "+mId);
                    } else {
                        FennecNativeDriver.log(FennecNativeDriver.LogLevel.ERROR,
                            "getText: unhandled type for view "+mId);
                    }
                } 
            } 
        );
        if (mText == null) {
            FennecNativeDriver.log(FennecNativeDriver.LogLevel.WARN,
                "getText: Text is null for view "+mId);
            return null;
        }
        return mText.toString();
    }

    private boolean mDisplayed;

    public boolean isDisplayed() {
        mDisplayed = false;
        RobocopUtils.runOnUiThreadSync(mActivity,
            new Runnable() {
                public void run() {
                    View view = (View)mActivity.findViewById(mId);
                    if (view != null) {
                        mDisplayed = true;
                    }
                }
            });
        return mDisplayed;
    }
}
