




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

    
    
    private HashSet<DoorHanger> mDoorHangers;

    DoorHangerPopup(GeckoApp aActivity, View aAnchor) {
        super(aActivity);
        mActivity = aActivity;
        mAnchor = aAnchor;

        mInflated = false;
        mArrowWidth = aActivity.getResources().getDimensionPixelSize(R.dimen.doorhanger_arrow_width);
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

    public void handleMessage(String event, JSONObject geckoObject) {
        try {
            if (event.equals("Doorhanger:Add")) {
                int tabId = geckoObject.getInt("tabID");
                String value = geckoObject.getString("value");
                String message = geckoObject.getString("message");
                JSONArray buttons = geckoObject.getJSONArray("buttons");
                JSONObject options = geckoObject.getJSONObject("options");

                addDoorHanger(tabId, value, message, buttons, options);
            } else if (event.equals("Doorhanger:Remove")) {
                int tabId = geckoObject.getInt("tabID");
                String value = geckoObject.getString("value");
                DoorHanger doorHanger = getDoorHanger(tabId, value);
                if (doorHanger == null)
                    return;

                removeDoorHanger(doorHanger);
                mActivity.runOnUiThread(new Runnable() {
                    public void run() {
                        updatePopup();
                    }
                });
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    public void onTabChanged(Tab tab, Tabs.TabEvents msg, Object data) {
        switch(msg) {
            case CLOSED:
                
                for (DoorHanger dh : mDoorHangers) {
                    if (dh.getTabId() == tab.getId())
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
        setFocusable(true);
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

        
        mActivity.runOnUiThread(new Runnable() {
            public void run() {
                if (!mInflated)
                    init();

                newDoorHanger.init(message, buttons, options);
                mContent.addView(newDoorHanger);

                
                if (tabId == Tabs.getInstance().getSelectedTab().getId())
                    updatePopup();
            }
        });
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

        
        mActivity.runOnUiThread(new Runnable() {
            public void run() {
                mContent.removeView(doorHanger);
            }
        });        
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

        fixBackgroundForFirst();
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
        showAsDropDown(mAnchor, offset, 0);
    }

    private void fixBackgroundForFirst() {
        for (int i = 0; i < mContent.getChildCount(); i++) {
            DoorHanger dh = (DoorHanger) mContent.getChildAt(i);
            if (dh.getVisibility() == View.VISIBLE) {
                dh.setBackgroundResource(R.drawable.doorhanger_bg);
                break;
            }
        }
    }

    private void registerEventListener(String event) {
        GeckoAppShell.getEventDispatcher().registerEventListener(event, this);
    }

    private void unregisterEventListener(String event) {
        GeckoAppShell.getEventDispatcher().unregisterEventListener(event, this);
    }
}
