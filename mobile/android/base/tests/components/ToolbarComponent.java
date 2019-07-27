



package org.mozilla.gecko.tests.components;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.fAssertEquals;
import static org.mozilla.gecko.tests.helpers.AssertionHelper.fAssertFalse;
import static org.mozilla.gecko.tests.helpers.AssertionHelper.fAssertNotNull;
import static org.mozilla.gecko.tests.helpers.AssertionHelper.fAssertTrue;

import org.mozilla.gecko.NewTabletUI;
import org.mozilla.gecko.R;
import org.mozilla.gecko.tests.StringHelper;
import org.mozilla.gecko.tests.UITestContext;
import org.mozilla.gecko.tests.helpers.DeviceHelper;
import org.mozilla.gecko.tests.helpers.NavigationHelper;
import org.mozilla.gecko.tests.helpers.WaitHelper;

import android.view.View;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.TextView;

import com.jayway.android.robotium.solo.Condition;
import com.jayway.android.robotium.solo.Solo;




public class ToolbarComponent extends BaseComponent {

    private static final String URL_HTTP_PREFIX = "http://";

    public ToolbarComponent(final UITestContext testContext) {
        super(testContext);
    }

    public ToolbarComponent assertIsEditing() {
        fAssertTrue("The toolbar is in the editing state", isEditing());
        return this;
    }

    public ToolbarComponent assertIsNotEditing() {
        fAssertFalse("The toolbar is not in the editing state", isEditing());
        return this;
    }

    public ToolbarComponent assertTitle(final String title, final String url) {
        
        fAssertNotNull("The title argument is not null", title);
        fAssertNotNull("The url argument is not null", url);

        
        final String expected;
        if (!NewTabletUI.isEnabled(mActivity)) {
            expected = title;
        } else {
            final String absoluteURL = NavigationHelper.adjustUrl(url);
            if (StringHelper.ABOUT_HOME_URL.equals(absoluteURL)) {
                expected = StringHelper.ABOUT_HOME_TITLE;
            } else if (absoluteURL.startsWith(URL_HTTP_PREFIX)) {
                expected = absoluteURL.substring(URL_HTTP_PREFIX.length());
            } else {
                expected = absoluteURL;
            }
        }

        fAssertEquals("The Toolbar title is " + expected, expected, getTitle());
        return this;
    }

    public ToolbarComponent assertUrl(final String expected) {
        assertIsEditing();
        fAssertEquals("The Toolbar url is " + expected, expected, getUrlEditText().getText());
        return this;
    }

    


    private View getToolbarView() {
        return mSolo.getView(R.id.browser_toolbar);
    }

    private EditText getUrlEditText() {
        return (EditText) getToolbarView().findViewById(R.id.url_edit_text);
    }

    private View getUrlDisplayLayout() {
        return getToolbarView().findViewById(R.id.display_layout);
    }

    private TextView getUrlTitleText() {
        return (TextView) getToolbarView().findViewById(R.id.url_bar_title);
    }

    private ImageButton getBackButton() {
        DeviceHelper.assertIsTablet();
        return (ImageButton) getToolbarView().findViewById(R.id.back);
    }

    private ImageButton getForwardButton() {
        DeviceHelper.assertIsTablet();
        return (ImageButton) getToolbarView().findViewById(R.id.forward);
    }

    private ImageButton getReloadButton() {
        DeviceHelper.assertIsTablet();
        return (ImageButton) getToolbarView().findViewById(R.id.reload);
    }
    


    private ImageButton getEditCancelButton() {
        return (ImageButton) getToolbarView().findViewById(R.id.edit_cancel);
    }

    private String getTitle() {
        return getTitleHelper(true);
    }

    




    public String getPotentiallyInconsistentTitle() {
        return getTitleHelper(false);
    }

    private String getTitleHelper(final boolean shouldAssertNotEditing) {
        if (shouldAssertNotEditing) {
            assertIsNotEditing();
        }

        return getUrlTitleText().getText().toString();
    }

    private boolean isEditing() {
        return getUrlDisplayLayout().getVisibility() != View.VISIBLE &&
                getUrlEditText().getVisibility() == View.VISIBLE;
    }

    public ToolbarComponent enterEditingMode() {
        assertIsNotEditing();

        mSolo.clickOnView(getUrlTitleText(), true);

        waitForEditing();
        WaitHelper.waitFor("UrlEditText to be input method target", new Condition() {
            @Override
            public boolean isSatisfied() {
                return getUrlEditText().isInputMethodTarget();
            }
        });

        return this;
    }

    public ToolbarComponent commitEditingMode() {
        assertIsEditing();

        WaitHelper.waitForPageLoad(new Runnable() {
            @Override
            public void run() {
                mSolo.sendKey(Solo.ENTER);
            }
        });
        waitForNotEditing();

        return this;
    }

    public ToolbarComponent dismissEditingMode() {
        assertIsEditing();

        mSolo.clickOnView(getEditCancelButton());

        waitForNotEditing();

        return this;
    }

    public ToolbarComponent enterUrl(final String url) {
        fAssertNotNull("url is not null", url);

        assertIsEditing();

        final EditText urlEditText = getUrlEditText();
        fAssertTrue("The UrlEditText is the input method target",
                urlEditText.isInputMethodTarget());

        mSolo.clearEditText(urlEditText);
        mSolo.enterText(urlEditText, url);

        return this;
    }

    public ToolbarComponent pressBackButton() {
        final ImageButton backButton = getBackButton();
        return pressButton(backButton, "back");
    }

    public ToolbarComponent pressForwardButton() {
        final ImageButton forwardButton = getForwardButton();
        return pressButton(forwardButton, "forward");
    }

    public ToolbarComponent pressReloadButton() {
        final ImageButton reloadButton = getReloadButton();
        return pressButton(reloadButton, "reload");
    }

    private ToolbarComponent pressButton(final View view, final String buttonName) {
        fAssertNotNull("The " + buttonName + " button View is not null", view);
        fAssertTrue("The " + buttonName + " button is enabled", view.isEnabled());
        fAssertEquals("The " + buttonName + " button is visible",
                View.VISIBLE, view.getVisibility());
        assertIsNotEditing();

        WaitHelper.waitForPageLoad(new Runnable() {
            @Override
            public void run() {
                mSolo.clickOnView(view);
            }
        });

        return this;
    }

    private void waitForEditing() {
        WaitHelper.waitFor("Toolbar to enter editing mode", new Condition() {
            @Override
            public boolean isSatisfied() {
                return isEditing();
            }
        });
    }

    private void waitForNotEditing() {
        WaitHelper.waitFor("Toolbar to exit editing mode", new Condition() {
            @Override
            public boolean isSatisfied() {
                return !isEditing();
            }
        });
    }
}
