




































package org.mozilla.gecko;

import android.app.Notification;
import android.app.NotificationManager;
import android.content.Context;
import android.widget.RemoteViews;

public class AlertNotification
    extends Notification
{
    Context mContext;
    int mId;
    int mIcon;
    String mTitle;
    boolean mProgressStyle;
    NotificationManager mNotificationManager;

    public AlertNotification(Context aContext, int aNotificationId, int aIcon, String aTitle, long aWhen) {
        super(aIcon, aTitle, aWhen);

        mContext = aContext;
        mIcon = aIcon;
        mTitle = aTitle;
        mProgressStyle = false;
        mId = aNotificationId;

        mNotificationManager = (NotificationManager)
            mContext.getSystemService(Context.NOTIFICATION_SERVICE);
    }

    public boolean isProgressStyle() {
        return mProgressStyle;
    }

    public void show() {
        mNotificationManager.notify(mId, this);
    }

    public void updateProgress(String aAlertText, long aProgress, long aProgressMax) {
        if (!mProgressStyle) {
            
            int layout = aAlertText.length() > 0 ? R.layout.notification_progress_text : R.layout.notification_progress;

            RemoteViews view = new RemoteViews("org.mozilla." + GeckoApp.mAppContext.getAppName(), layout);
            view.setImageViewResource(R.id.notificationImage, mIcon);
            view.setTextViewText(R.id.notificationTitle, mTitle);
            contentView = view;
            flags |= FLAG_ONGOING_EVENT;

            mProgressStyle = true;
        }

        String text;
        if (aAlertText.length() > 0) {
            text = aAlertText;
        } else {
            int percent = 0;
            if (aProgressMax > 0)
                percent = (int)(100 * aProgress / aProgressMax);
            text = percent + "%";
        }

        contentView.setTextViewText(R.id.notificationText, text);
        contentView.setProgressBar(R.id.notificationProgressbar, (int)aProgressMax, (int)aProgress, false);

        
        mNotificationManager.notify(mId, this);
    }
}
