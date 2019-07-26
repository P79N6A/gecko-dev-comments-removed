




package org.mozilla.gecko;

import android.content.Context;
import android.os.Build;
import android.provider.Settings.Secure;
import android.view.inputmethod.InputMethodInfo;
import android.view.inputmethod.InputMethodManager;

import java.util.Arrays;
import java.util.Collection;
import java.util.Locale;

final class InputMethods {

    public static final String METHOD_ATOK = "com.justsystems.atokmobile.service/.AtokInputMethodService";
    public static final String METHOD_GOOGLE_JAPANESE_INPUT = "com.google.android.inputmethod.japanese/.MozcService";
    public static final String METHOD_IWNN = "jp.co.omronsoft.iwnnime.ml/.standardcommon.IWnnLanguageSwitcher";
    public static final String METHOD_OPENWNN_PLUS = "com.owplus.ime.openwnnplus/.OpenWnnJAJP";
    public static final String METHOD_SIMEJI = "com.adamrocker.android.input.simeji/.OpenWnnSimeji";
    public static final String METHOD_STOCK_LATINIME = "com.google.android.inputmethod.latin/com.android.inputmethod.latin.LatinIME";
    public static final String METHOD_SWYPE = "com.swype.android.inputmethod/.SwypeInputMethod";
    public static final String METHOD_SWYPE_BETA = "com.nuance.swype.input/.IME";

    











    
    private static final Collection<String> sHKBWhiteList = Arrays.asList(new String[] {
                                                            METHOD_ATOK,
                                                            METHOD_GOOGLE_JAPANESE_INPUT,
                                                            METHOD_IWNN,
                                                            METHOD_OPENWNN_PLUS,
                                                            METHOD_SIMEJI,
                                                            });
    private static Boolean sIsPreJellyBeanAsusTransformer;

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

    public static boolean canUseInputMethodOnHKB(String inputMethod) {
        if (sHKBWhiteList.contains(inputMethod)) {
            return true;
        }

        
        
        if (sIsPreJellyBeanAsusTransformer == null) {
            sIsPreJellyBeanAsusTransformer = Build.VERSION.SDK_INT < 16 &&
                                             "asus".equals(Build.BRAND) &&
                                             "EeePad".equals(Build.BOARD);
        }
        
        return sIsPreJellyBeanAsusTransformer && !Locale.getDefault().equals(Locale.US);
    }

    public static boolean needsSoftResetWorkaround(String inputMethod) {
        
        return Build.VERSION.SDK_INT >= 17 &&
               inputMethod.equals(METHOD_STOCK_LATINIME);
    }
}
