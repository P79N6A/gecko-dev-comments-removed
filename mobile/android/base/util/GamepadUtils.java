




package org.mozilla.gecko.util;

import android.os.Build;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;

public final class GamepadUtils {
    private static View.OnKeyListener sClickDispatcher;
    private static View.OnKeyListener sListItemClickDispatcher;

    private GamepadUtils() {
    }

    private static boolean isGamepadKey(KeyEvent event) {
        if (Build.VERSION.SDK_INT >= 9) {
            return (event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD;
        }
        return false;
    }

    public static boolean isActionKey(KeyEvent event) {
        return (isGamepadKey(event) && (event.getKeyCode() == KeyEvent.KEYCODE_BUTTON_A));
    }

    public static boolean isActionKeyDown(KeyEvent event) {
        return isActionKey(event) && event.getAction() == KeyEvent.ACTION_DOWN;
    }

    public static boolean isBackKey(KeyEvent event) {
        return (isGamepadKey(event) && (event.getKeyCode() == KeyEvent.KEYCODE_BUTTON_B));
    }

    public static View.OnKeyListener getClickDispatcher() {
        if (sClickDispatcher == null) {
            sClickDispatcher = new View.OnKeyListener() {
                @Override
                public boolean onKey(View v, int keyCode, KeyEvent event) {
                    if (isActionKeyDown(event)) {
                        return v.performClick();
                    }
                    return false;
                }
            };
        }
        return sClickDispatcher;
    }

    public static View.OnKeyListener getListItemClickDispatcher() {
        if (sListItemClickDispatcher == null) {
            sListItemClickDispatcher = new View.OnKeyListener() {
                @Override
                public boolean onKey(View v, int keyCode, KeyEvent event) {
                    if (isActionKeyDown(event) && (v instanceof ListView)) {
                        ListView view = (ListView)v;
                        AdapterView.OnItemClickListener listener = view.getOnItemClickListener();
                        if (listener != null) {
                            listener.onItemClick(view, view.getSelectedView(), view.getSelectedItemPosition(), view.getSelectedItemId());
                            return true;
                        }
                    }
                    return false;
                }
            };
        }
        return sListItemClickDispatcher;
    }
}
