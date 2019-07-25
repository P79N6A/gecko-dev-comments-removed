




package org.mozilla.gecko;

import android.content.Context;
import android.provider.Settings.Secure;
import android.view.inputmethod.InputMethodInfo;
import android.view.inputmethod.InputMethodManager;

import java.util.Collection;

final class InputMethods {

    public static final String METHOD_SWYPE = "com.swype.android.inputmethod/.SwypeInputMethod";
    public static final String METHOD_SWYPE_BETA = "com.nuance.swype.input/.IME";

    














    private InputMethods() {}

    public static String getCurrentInputMethod(Context context) {
        return Secure.getString(context.getContentResolver(), Secure.DEFAULT_INPUT_METHOD);
    }

    public static InputMethodInfo getInputMethodInfo(Context context, String inputMethod) {
        InputMethodManager imm = getInputMethodManager(context);
        Collection<InputMethodInfo> infos = imm.getEnabledInputMethodList();
        for (InputMethodInfo info : infos) {
            if (info.getId().equals(inputMethod)) {
                return info;
            }
        }
        return null;
    }

    public static InputMethodManager getInputMethodManager(Context context) {
        return (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
    }
}
