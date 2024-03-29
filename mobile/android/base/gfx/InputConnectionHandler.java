



package org.mozilla.gecko.gfx;

import android.os.Handler;
import android.view.KeyEvent;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;

public interface InputConnectionHandler
{
    Handler getHandler(Handler defHandler);
    InputConnection onCreateInputConnection(EditorInfo outAttrs);
    boolean onKeyPreIme(int keyCode, KeyEvent event);
    boolean onKeyDown(int keyCode, KeyEvent event);
    boolean onKeyLongPress(int keyCode, KeyEvent event);
    boolean onKeyMultiple(int keyCode, int repeatCount, KeyEvent event);
    boolean onKeyUp(int keyCode, KeyEvent event);
    boolean isIMEEnabled();
}
