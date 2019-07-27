




package org.mozilla.gecko.firstrun;

import android.support.v4.app.Fragment;

public class FirstrunPanel extends Fragment {

    public static final int TITLE_RES = -1;
    protected FirstrunPane.OnFinishListener onFinishListener;

    public void setOnFinishListener(FirstrunPane.OnFinishListener listener) {
        this.onFinishListener = listener;
    }

    protected void close() {
        if (onFinishListener != null) {
            onFinishListener.onFinish();
        }
    }
}
