




package org.mozilla.gecko.widget;

import android.view.Menu;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.menu.MenuItemActionView;
import org.mozilla.gecko.menu.QuickShareBarActionView;
import org.mozilla.gecko.overlays.ui.ShareDialog;
import org.mozilla.gecko.util.ThreadUtils;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.view.MenuItem;
import android.view.MenuItem.OnMenuItemClickListener;
import android.view.SubMenu;
import android.view.View;
import android.view.View.OnClickListener;
import android.text.TextUtils;

import java.util.ArrayList;
import java.util.HashMap;

public class GeckoActionProvider {
    private static final int MAX_HISTORY_SIZE_DEFAULT = 2;

    




    public interface OnTargetSelectedListener {
        public void onTargetSelected();
    }

    final Context mContext;

    public final static String DEFAULT_MIME_TYPE = "text/plain";

    public static final String DEFAULT_HISTORY_FILE_NAME = "history.xml";

    
    String mHistoryFileName = DEFAULT_HISTORY_FILE_NAME;

    OnTargetSelectedListener mOnTargetListener;

    private final Callbacks mCallbacks = new Callbacks();

    private static final HashMap<String, GeckoActionProvider> mProviders = new HashMap<String, GeckoActionProvider>();

    private static String getFilenameFromMimeType(String mimeType) {
        String[] mime = mimeType.split("/");

        
        if ("text".equals(mime[0])) {
            return DEFAULT_HISTORY_FILE_NAME;
        }

        return "history-" + mime[0] + ".xml";
    }

    
    public static GeckoActionProvider getForType(String mimeType, Context context) {
        if (!mProviders.keySet().contains(mimeType)) {
            GeckoActionProvider provider = new GeckoActionProvider(context);

            
            if (TextUtils.isEmpty(mimeType)) {
                return provider;
            }

            provider.setHistoryFileName(getFilenameFromMimeType(mimeType));
            mProviders.put(mimeType, provider);
        }
        return mProviders.get(mimeType);
    }

    public GeckoActionProvider(Context context) {
        mContext = context;
    }

    


    public View onCreateActionView() {
        return onCreateActionView(MAX_HISTORY_SIZE_DEFAULT, false);
    }

    public View onCreateActionView(final int maxHistorySize, final boolean isForQuickShareBar) {
        
        ActivityChooserModel dataModel = ActivityChooserModel.get(mContext, mHistoryFileName);
        final MenuItemActionView view;
        if (isForQuickShareBar) {
            view = new QuickShareBarActionView(mContext, null);
        } else {
            view = new MenuItemActionView(mContext, null);
        }
        view.addActionButtonClickListener(mCallbacks);

        final PackageManager packageManager = mContext.getPackageManager();
        int historySize = dataModel.getDistinctActivityCountInHistory();
        if (historySize > maxHistorySize) {
            historySize = maxHistorySize;
        }

        
        
        
        
        if (historySize > dataModel.getActivityCount()) {
            return view;
        }

        for (int i = 0; i < historySize; i++) {
            view.addActionButton(dataModel.getActivity(i).loadIcon(packageManager), 
                                 dataModel.getActivity(i).loadLabel(packageManager));
        }

        return view;
    }

    public boolean hasSubMenu() {
        return true;
    }

    public void onPrepareSubMenu(SubMenu subMenu) {
        
        subMenu.clear();

        ActivityChooserModel dataModel = ActivityChooserModel.get(mContext, mHistoryFileName);
        PackageManager packageManager = mContext.getPackageManager();

        
        final String shareDialogClassName = ShareDialog.class.getCanonicalName();
        final String sendTabLabel = mContext.getResources().getString(R.string.overlay_share_send_other);
        final int count = dataModel.getActivityCount();
        for (int i = 0; i < count; i++) {
            ResolveInfo activity = dataModel.getActivity(i);
            final CharSequence activityLabel = activity.loadLabel(packageManager);

            
            
            final int order;
            if (shareDialogClassName.equals(activity.activityInfo.name) &&
                    sendTabLabel.equals(activityLabel)) {
                order = Menu.FIRST + i;
            } else {
                order = Menu.FIRST + (i | Menu.CATEGORY_SECONDARY);
            }

            subMenu.add(0, i, order, activityLabel)
                .setIcon(activity.loadIcon(packageManager))
                .setOnMenuItemClickListener(mCallbacks);
        }
    }

    public void setHistoryFileName(String historyFile) {
        mHistoryFileName = historyFile;
    }

    public Intent getIntent() {
        ActivityChooserModel dataModel = ActivityChooserModel.get(mContext, mHistoryFileName);
        return dataModel.getIntent();
    }

    public void setIntent(Intent intent) {
        ActivityChooserModel dataModel = ActivityChooserModel.get(mContext, mHistoryFileName);
        dataModel.setIntent(intent);

        
        if (mOnTargetListener != null) {
            mOnTargetListener.onTargetSelected();
        }
    }

    public void setOnTargetSelectedListener(OnTargetSelectedListener listener) {
        mOnTargetListener = listener;
    }

    public ArrayList<ResolveInfo> getSortedActivities() {
        ArrayList<ResolveInfo> infos = new ArrayList<ResolveInfo>();

        ActivityChooserModel dataModel = ActivityChooserModel.get(mContext, mHistoryFileName);

        
        final int count = dataModel.getActivityCount();
        for (int i = 0; i < count; i++) {
            infos.add(dataModel.getActivity(i));
        }

        return infos;
    }

    public void chooseActivity(int position) {
        mCallbacks.chooseActivity(position);
    }

    


    private class Callbacks implements OnMenuItemClickListener,
                                       OnClickListener {
        void chooseActivity(int index) {
            final ActivityChooserModel dataModel = ActivityChooserModel.get(mContext, mHistoryFileName);
            final Intent launchIntent = dataModel.chooseActivity(index);
            if (launchIntent != null) {
                
                ThreadUtils.postToBackgroundThread(new Runnable() {
                    @Override
                    public void run() {
                        
                        String type = launchIntent.getType();
                        if (Intent.ACTION_SEND.equals(launchIntent.getAction()) && type != null && type.startsWith("image/")) {
                            GeckoAppShell.downloadImageForIntent(launchIntent);
                        }

                        launchIntent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
                        mContext.startActivity(launchIntent);
                    }
                });
            }

            if (mOnTargetListener != null) {
                mOnTargetListener.onTargetSelected();
            }
        }

        @Override
        public boolean onMenuItemClick(MenuItem item) {
            chooseActivity(item.getItemId());

            
            Telemetry.sendUIEvent(TelemetryContract.Event.SHARE, TelemetryContract.Method.LIST);
            return true;
        }

        @Override
        public void onClick(View view) {
            Integer index = (Integer) view.getTag();
            chooseActivity(index);

            
            Telemetry.sendUIEvent(TelemetryContract.Event.SHARE, TelemetryContract.Method.BUTTON);
        }
    }
}
