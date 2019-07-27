



package org.mozilla.gecko.tests.components;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.fAssertNotNull;
import static org.mozilla.gecko.tests.helpers.AssertionHelper.fAssertNotSame;
import static org.mozilla.gecko.tests.helpers.AssertionHelper.fAssertTrue;

import org.mozilla.gecko.R;
import org.mozilla.gecko.tests.UITestContext;
import org.mozilla.gecko.tests.helpers.FrameworkHelper;
import org.mozilla.gecko.tests.helpers.WaitHelper;

import android.content.Context;
import android.content.ContextWrapper;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;

import com.robotium.solo.Condition;




public class GeckoViewComponent extends BaseComponent {

    public interface InputConnectionTest {
        public void test(InputConnection ic, EditorInfo info);
    }

    public final TextInput mTextInput;

    public GeckoViewComponent(final UITestContext testContext) {
        super(testContext);
        mTextInput = new TextInput();
    }

    


    private View getView() {
        
        return mSolo.getView(R.id.layer_view);
    }

    private void setContext(final Context newContext) {
        final View geckoView = getView();
        
        mTestContext.getInstrumentation().runOnMainSync(new Runnable() {
            @Override
            public void run() {
                FrameworkHelper.setViewContext(geckoView, newContext);
            }
        });
    }

    public class TextInput {
        private TextInput() {
        }

        private InputMethodManager getInputMethodManager() {
            final InputMethodManager imm = (InputMethodManager)
                mActivity.getSystemService(Context.INPUT_METHOD_SERVICE);
            fAssertNotNull("Must have an InputMethodManager", imm);
            return imm;
        }

        


        private boolean isActive() {
            return getInputMethodManager().isActive(getView());
        }

        public TextInput assertActive() {
            fAssertTrue("Current view should be the active input view", isActive());
            return this;
        }

        public TextInput waitForActive() {
            WaitHelper.waitFor("current view to become the active input view", new Condition() {
                @Override
                public boolean isSatisfied() {
                    return isActive();
                }
            });
            return this;
        }

        





        private boolean hasInputConnection() {
            final InputMethodManager imm = getInputMethodManager();
            return imm.isActive(getView()) && imm.isAcceptingText();
        }

        public TextInput assertInputConnection() {
            fAssertTrue("Current view should have an active InputConnection", hasInputConnection());
            return this;
        }

        public TextInput waitForInputConnection() {
            WaitHelper.waitFor("current view to have an active InputConnection", new Condition() {
                @Override
                public boolean isSatisfied() {
                    return hasInputConnection();
                }
            });
            return this;
        }

        









        public TextInput testInputConnection(final InputConnectionTest test) {

            fAssertNotNull("Test must not be null", test);
            assertInputConnection();

            
            
            final View geckoView = getView();
            final Handler inputConnectionHandler = geckoView.getHandler();
            final Context oldGeckoViewContext = FrameworkHelper.getViewContext(geckoView);

            setContext(new ContextWrapper(oldGeckoViewContext) {
                @Override
                public Object getSystemService(String name) {
                    if (Context.INPUT_METHOD_SERVICE.equals(name)) {
                        return null;
                    }
                    return super.getSystemService(name);
                }
            });

            (new InputConnectionTestRunner(test)).runOnHandler(inputConnectionHandler);

            setContext(oldGeckoViewContext);
            return this;
        }

        private class InputConnectionTestRunner implements Runnable {
            private final InputConnectionTest mTest;
            private boolean mDone;

            public InputConnectionTestRunner(final InputConnectionTest test) {
                mTest = test;
            }

            public synchronized void runOnHandler(final Handler inputConnectionHandler) {
                
                
                
                
                fAssertNotSame("InputConnection should not be running on instrumentation thread",
                    Looper.myLooper(), inputConnectionHandler.getLooper());

                mDone = false;
                inputConnectionHandler.post(this);
                do {
                    try {
                        wait();
                    } catch (InterruptedException e) {
                        
                    }
                } while (!mDone);
            }

            @Override
            public void run() {
                final EditorInfo info = new EditorInfo();
                final InputConnection ic = getView().onCreateInputConnection(info);
                fAssertNotNull("Must have an InputConnection", ic);
                
                ic.clearMetaKeyStates(-1);
                ic.finishComposingText();
                mTest.test(ic, info);
                synchronized (this) {
                    
                    mDone = true;
                    notify();
                }
            }
        }
    }
}
