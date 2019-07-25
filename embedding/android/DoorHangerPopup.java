





































package org.mozilla.gecko;

import java.util.HashMap;
import java.util.Iterator;

import android.content.Context;
import android.graphics.drawable.BitmapDrawable;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.widget.PopupWindow;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.RelativeLayout;

public class DoorHangerPopup extends PopupWindow {
    private Context mContext;
    private LinearLayout mContent;

    public DoorHangerPopup(Context aContext) {
        super(aContext);
        mContext = aContext;

        setBackgroundDrawable(new BitmapDrawable());
        setWindowLayoutMode(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);

        LayoutInflater inflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        RelativeLayout layout = (RelativeLayout) inflater.inflate(R.layout.doorhangerpopup, null);
        mContent = (LinearLayout) layout.findViewById(R.id.doorhanger_container);
        
        setContentView(layout);
    }

    public DoorHanger addDoorHanger(Tab tab, String value) {
        Log.i("DoorHangerPopup", "Adding a DoorHanger to Tab: " + tab.getId());

        DoorHanger dh = tab.getDoorHanger(value);
        if (dh != null) {
            dh.hidePopup();
            tab.removeDoorHanger(value);
        }

        dh = new DoorHanger(mContent.getContext(), value);
        dh.setTab(tab);
        tab.addDoorHanger(value, dh);
        mContent.addView(dh);
        
        return dh;
    }

    public void removeDoorHanger(Tab tab, String value) {
        Log.i("DoorHangerPopup", "Removing a DoorHanger from Tab: " + tab.getId());
        tab.removeDoorHanger(value);

        if (tab.getDoorHangers().size() == 0)
            hide();
        else
            fixBackgroundForFirst(); 
    }

    public void showDoorHanger(DoorHanger dh) {
        if (dh == null)
            return;

        dh.showPopup();
        show();
    }

    public void hideDoorHanger(DoorHanger dh) {
        if (dh == null)
            return;

        dh.hidePopup();
        show();
    }

    public void hideAllDoorHangers() {
        hideAllDoorHangersExcept(null);
    }

    public void hideAllDoorHangersExcept(Tab tab) {
        for (int i=0; i < mContent.getChildCount(); i++) {
            DoorHanger dh = (DoorHanger) mContent.getChildAt(i);
            if (dh.getTab() != tab)
                dh.hidePopup();
        }

        if (tab == null)
            hide();
    }
    
    public void hide() {
        if (isShowing()) {
            Log.i("DoorHangerPopup", "Dismissing the DoorHangerPopup");
            dismiss();
        }
    }

    public void show() {
        Log.i("DoorHangerPopup", "Showing the DoorHangerPopup");
        fixBackgroundForFirst();

        if (isShowing())
            update();
        else
            showAsDropDown(GeckoApp.mBrowserToolbar.mFavicon);
    }

    public void removeForTab(Tab tab) {
        Log.i("DoorHangerPopup", "Removing all doorhangers for tab: " + tab.getId());
        tab.removeAllDoorHangers();
    }

    public void showForTab(Tab tab) {
        Log.i("DoorHangerPopup", "Showing all doorhangers for tab: " + tab.getId());
        HashMap<String, DoorHanger> doorHangers = tab.getDoorHangers();

        if (doorHangers == null) {
            hide();
            return;
        }

        hideAllDoorHangersExcept(tab);

        Iterator keys = doorHangers.keySet().iterator();
        while (keys.hasNext()) {
            ((DoorHanger) doorHangers.get(keys.next())).showPopup();
        }

        if (doorHangers.size() > 0)
            show();
        else
            hide();
    }

    public void hideForTab(Tab tab) {
        Log.i("DoorHangerPopup", "Hiding all doorhangers for tab: " + tab.getId());
        HashMap<String, DoorHanger> doorHangers = tab.getDoorHangers();

        if (doorHangers == null)
            return;

        Iterator keys = doorHangers.keySet().iterator();
        while (keys.hasNext()) {
            ((DoorHanger) doorHangers.get(keys.next())).hidePopup();
        }
    }

    private void fixBackgroundForFirst() {
        for (int i=0; i < mContent.getChildCount(); i++) {
            DoorHanger dh = (DoorHanger) mContent.getChildAt(i);
            if (dh.isVisible()) {
                dh.setBackgroundResource(R.drawable.doorhanger_bg);
                break;
            }
        }
    }
}
