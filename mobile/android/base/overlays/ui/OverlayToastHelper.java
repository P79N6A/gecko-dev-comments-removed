



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
    






    public static void showFailureToast(Context context, String failureMessage, View.OnClickListener retryListener) {
        showToast(context, failureMessage, false, retryListener);
    }
    public static void showFailureToast(Context context, String failureMessage) {
        showFailureToast(context, failureMessage, null);
    }

    



    public static void showSuccessToast(Context context, String successMessage) {
        showToast(context, successMessage, true, null);
    }

    private static void showToast(Context context, String message, boolean success, View.OnClickListener retryListener) {
        LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        View layout = inflater.inflate(R.layout.overlay_share_toast, null);

        TextView text = (TextView) layout.findViewById(R.id.overlay_toast_message);
        text.setText(message);

        if (retryListener == null) {
            
            layout.findViewById(R.id.overlay_toast_separator).setVisibility(View.GONE);
            layout.findViewById(R.id.overlay_toast_retry_btn).setVisibility(View.GONE);
        } else {
            
            Button retryBtn = (Button) layout.findViewById(R.id.overlay_toast_retry_btn);
            retryBtn.setOnClickListener(retryListener);
        }

        if (!success) {
            
            text.setCompoundDrawables(null, null, null, null);
        }

        Toast toast = new Toast(context);
        toast.setGravity(Gravity.CENTER_VERTICAL | Gravity.BOTTOM, 0, 0);
        toast.setDuration(Toast.LENGTH_SHORT);
        toast.setView(layout);
        toast.show();
    }
}
