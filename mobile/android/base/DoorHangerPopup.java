




package org.mozilla.gecko;

import org.mozilla.gecko.util.GeckoEventListener;

import org.json.JSONArray;
import org.json.JSONObject;

import android.graphics.drawable.BitmapDrawable;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.RelativeLayout;

import java.util.HashSet;

public class DoorHangerPopup extends PopupWindow
                             implements GeckoEventListener, Tabs.OnTabsChangedListener {
    private static final String LOGTAG = "GeckoDoorHangerPopup";

    private GeckoApp mActivity;
    private View mAnchor;
    private LinearLayout mContent;

    private boolean mInflated; 
    private ImageView mArrow;
    private int mArrowWidth;
    private int mYOffset;

    
    
    private HashSet<DoorHanger> mDoorHangers;

    DoorHangerPopup(GeckoApp aActivity, View aAnchor) {
        super(aActivity);
        mActivity = aActivity;
        mAnchor = aAnchor;

        mInflated = false;
        mArrowWidth = aActivity.getResources().getDimensionPixelSize(R.dimen.menu_popup_arrow_width);
        mYOffset = aActivity.getResources().getDimensionPixelSize(R.dimen.menu_popup_offset);
        mDoorHangers = new HashSet<DoorHanger>();

        registerEventListener("Doorhanger:Add");
        registerEventListener("Doorhanger:Remove");
        Tabs.registerOnTabsChangedListener(this);
    }

    void destroy() {
        unregisterEventListener("Doorhanger:Add");
        unregisterEventListener("Doorhanger:Remove");
        Tabs.unregisterOnTabsChangedListener(this);
    }

    void setAnchor(View aAnchor) {
        mAnchor = aAnchor;
    }

    @Override
    public void handleMessage(String event, JSONObject geckoObject) {
        try {
            if (event.equals("Doorhanger:Add")) {
                final int tabId = geckoObject.getInt("tabID");
                final String value = geckoObject.getString("value");
                final String message = geckoObject.getString("message");
                final JSONArray buttons = geckoObject.getJSONArray("buttons");
                final JSONObject options = geckoObject.getJSONObject("options");

                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        addDoorHanger(tabId, value, message, buttons, options);
                    }
                });
            } else if (event.equals("Doorhanger:Remove")) {
                final int tabId = geckoObject.getInt("tabID");
                final String value = geckoObject.getString("value");

                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        DoorHanger doorHanger = getDoorHanger(tabId, value);
                        if (doorHanger == null)
                            return;

                        removeDoorHanger(doorHanger);
                        updatePopup();
                    }
                });
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    
    @Override
    public void onTabChanged(final Tab tab, final Tabs.TabEvents msg, final Object data) {
        switch(msg) {
            case CLOSED:
                
                
                HashSet<DoorHanger> doorHangersToRemove = new HashSet<DoorHanger>();
                for (DoorHanger dh : mDoorHangers) {
                    if (dh.getTabId() == tab.getId())
                        doorHangersToRemove.add(dh);
                }
                for (DoorHanger dh : doorHangersToRemove) {
                    removeDoorHanger(dh);
                }
                break;

            case LOCATION_CHANGE:
                
                if (!isShowing() || !data.equals(tab.getURL()))
                    removeTransientDoorHangers(tab.getId());

                
                if (Tabs.getInstance().isSelectedTab(tab))
                    updatePopup();
                break;

            case SELECTED:
                
                
                updatePopup();
                break;
        }
    }

    private void init() {
        setBackgroundDrawable(new BitmapDrawable());
        setOutsideTouchable(true);
        setWindowLayoutMode(mActivity.isTablet() ? ViewGroup.LayoutParams.WRAP_CONTENT : ViewGroup.LayoutParams.FILL_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT);

        LayoutInflater inflater = LayoutInflater.from(mActivity);
        RelativeLayout layout = (RelativeLayout) inflater.inflate(R.layout.doorhangerpopup, null);
        mArrow = (ImageView) layout.findViewById(R.id.doorhanger_arrow);
        mContent = (LinearLayout) layout.findViewById(R.id.doorhanger_container);
        
        setContentView(layout);
        mInflated = true;
    }

    




    void addDoorHanger(final int tabId, final String value, final String message,
                       final JSONArray buttons, final JSONObject options) {
        
        if (Tabs.getInstance().getTab(tabId) == null)
            return;

        
        DoorHanger oldDoorHanger = getDoorHanger(tabId, value);
        if (oldDoorHanger != null)
            removeDoorHanger(oldDoorHanger);

        final DoorHanger newDoorHanger = new DoorHanger(mActivity, this, tabId, value);
        mDoorHangers.add(newDoorHanger);

        if (!mInflated)
            init();

        newDoorHanger.init(message, buttons, options);
        mContent.addView(newDoorHanger);

        
        if (tabId == Tabs.getInstance().getSelectedTab().getId())
            updatePopup();
    }

    




    DoorHanger getDoorHanger(int tabId, String value) {
        for (DoorHanger dh : mDoorHangers) {
            if (dh.getTabId() == tabId && dh.getValue().equals(value))
                return dh;
        }

        
        return null;
    }

    




    void removeDoorHanger(final DoorHanger doorHanger) {
        mDoorHangers.remove(doorHanger);
        mContent.removeView(doorHanger);
    }

    




    void removeTransientDoorHangers(int tabId) {
        
        HashSet<DoorHanger> doorHangersToRemove = new HashSet<DoorHanger>();
        for (DoorHanger dh : mDoorHangers) {
            
            if (dh.getTabId() == tabId && dh.shouldRemove())
                doorHangersToRemove.add(dh);
        }

        for (DoorHanger dh : doorHangersToRemove) {
            removeDoorHanger(dh);
        }
    }

    




    void updatePopup() {
        
        
        
        Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab == null || mDoorHangers.size() == 0 || !mInflated) {
            dismiss();
            return;
        }

        
        int tabId = tab.getId();
        boolean shouldShowPopup = false;
        for (DoorHanger dh : mDoorHangers) {
            if (dh.getTabId() == tabId) {
                dh.setVisibility(View.VISIBLE);
                shouldShowPopup = true;
            } else {
                dh.setVisibility(View.GONE);
            }
        }
 
        
        if (!shouldShowPopup) {
            dismiss();
            return;
        }

        showDividers();
        if (isShowing()) {
            update();
            return;
        }

        
        if (mAnchor == null) {
            showAtLocation(mActivity.getView(), Gravity.TOP, 0, 0);
            return;
        }

        
        
        
        int offset = mActivity.isTablet() ? mAnchor.getWidth()/2 - mArrowWidth/2 -
                     ((RelativeLayout.LayoutParams) mArrow.getLayoutParams()).leftMargin : 0;
        showAsDropDown(mAnchor, offset, -mYOffset);
        
        setFocusable(true);
    }

    private void showDividers() {
        int count = mContent.getChildCount();

        for (int i = 0; i < count; i++) {
            DoorHanger dh = (DoorHanger) mContent.getChildAt(i);
            dh.showDivider();
        }

        ((DoorHanger) mContent.getChildAt(count-1)).hideDivider();
    }

    private void registerEventListener(String event) {
        GeckoAppShell.getEventDispatcher().registerEventListener(event, this);
    }

    private void unregisterEventListener(String event) {
        GeckoAppShell.getEventDispatcher().unregisterEventListener(event, this);
    }

    @Override
    public void dismiss() {
        
        
        setFocusable(false);
        super.dismiss();
    }
}
