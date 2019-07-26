




package org.mozilla.gecko;

import android.content.Context;




public class AppNotificationClient extends NotificationClient {
    private final Context mContext;

    public AppNotificationClient(Context context) {
        mContext = context;
    }

    @Override
    protected void bind() {
        super.bind();
        connectHandler(new NotificationHandler(mContext));
    }
}
