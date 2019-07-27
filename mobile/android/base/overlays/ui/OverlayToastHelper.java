



package org.mozilla.gecko.overlays.ui;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import org.mozilla.gecko.R;








public class OverlayToastHelper {

    




    public static void showFailureToast(Context context, String failureMessage) {
        showToast(context, failureMessage, false);
    }

    



    public static void showSuccessToast(Context context, String successMessage) {
        showToast(context, successMessage, true);
    }

    private static void showToast(Context context, String message, boolean success) {
        LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        View layout = inflater.inflate(R.layout.overlay_share_toast, null);

        TextView text = (TextView) layout.findViewById(R.id.overlay_toast_message);
        text.setText(message);

        if (!success) {
            
            text.setCompoundDrawables(null, null, null, null);
        }

        Toast toast = new Toast(context);
        toast.setDuration(Toast.LENGTH_SHORT);
        toast.setView(layout);
        toast.show();
    }
}
