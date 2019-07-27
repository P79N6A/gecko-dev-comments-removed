



package org.mozilla.gecko.overlays.ui;

import android.content.Context;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import org.mozilla.gecko.R;








public class OverlayToastHelper {
    






    public static void showFailureToast(Context context, String failureMessage, boolean isTransient, View.OnClickListener retryListener) {
        showToast(context, failureMessage, isTransient, retryListener);
    }
    public static void showFailureToast(Context context, String failureMessage, boolean isTransient) {
        showFailureToast(context, failureMessage, isTransient, null);
    }

    



    public static void showSuccessToast(Context context, String successMesssage) {
        showToast(context, successMesssage, false, null);
    }

    private static void showToast(Context context, String message, boolean withRetry, View.OnClickListener retryListener) {
    }
}
