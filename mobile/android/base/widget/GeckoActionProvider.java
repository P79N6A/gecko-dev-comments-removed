




package org.mozilla.gecko.widget;

import org.mozilla.gecko.menu.MenuItemActionView;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.graphics.drawable.Drawable;
import android.view.ActionProvider;
import android.view.MenuItem;
import android.view.MenuItem.OnMenuItemClickListener;
import android.view.SubMenu;
import android.view.View;
import android.view.View.OnClickListener;

public class GeckoActionProvider extends ActionProvider {

    




    public interface OnTargetSelectedListener {
        public void onTargetSelected();
    }

    private final Context mContext;

    public static final String DEFAULT_HISTORY_FILE_NAME = "history.xml";

    
    private String mHistoryFileName = DEFAULT_HISTORY_FILE_NAME;

    private OnTargetSelectedListener mOnTargetListener;

    private final Callbacks mCallbacks = new Callbacks();

    public GeckoActionProvider(Context context) {
        super(context);
        mContext = context;
    }

    @Override
    public View onCreateActionView() {
        
        ActivityChooserModel dataModel = ActivityChooserModel.get(mContext, mHistoryFileName);
        MenuItemActionView view = new MenuItemActionView(mContext, null);
        view.setActionButtonClickListener(mCallbacks);

        if (dataModel.getHistorySize() > 0) {
            PackageManager packageManager = mContext.getPackageManager();
            ResolveInfo defaultActivity = dataModel.getDefaultActivity();
            view.setActionButton(defaultActivity == null ? null : defaultActivity.loadIcon(packageManager));
        }

        return view;
    }

    public View getView() {
        return onCreateActionView();
    }

    @Override
    public boolean hasSubMenu() {
        return true;
    }

    @Override
    public void onPrepareSubMenu(SubMenu subMenu) {
        
        subMenu.clear();

        ActivityChooserModel dataModel = ActivityChooserModel.get(mContext, mHistoryFileName);
        PackageManager packageManager = mContext.getPackageManager();

        
        final int count = dataModel.getActivityCount();
        for (int i = 0; i < count; i++) {
            ResolveInfo activity = dataModel.getActivity(i);
            subMenu.add(0, i, i, activity.loadLabel(packageManager))
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
    }

    public void setOnTargetSelectedListener(OnTargetSelectedListener listener) {
        mOnTargetListener = listener;
    }

    


    private class Callbacks implements OnMenuItemClickListener,
                                       OnClickListener {
        private void chooseActivity(int index) { 
            if (mOnTargetListener != null)
                mOnTargetListener.onTargetSelected();

            ActivityChooserModel dataModel = ActivityChooserModel.get(mContext, mHistoryFileName);
            Intent launchIntent = dataModel.chooseActivity(index);
            if (launchIntent != null) {
                launchIntent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
                mContext.startActivity(launchIntent);
            }
        }

        @Override
        public boolean onMenuItemClick(MenuItem item) {
            chooseActivity(item.getItemId());
            return true;
        }

        @Override
        public void onClick(View view) {
            ActivityChooserModel dataModel = ActivityChooserModel.get(mContext, mHistoryFileName);
            chooseActivity(dataModel.getActivityIndex(dataModel.getDefaultActivity()));
        }
    }
}

