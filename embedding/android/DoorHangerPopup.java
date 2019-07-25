





































package org.mozilla.gecko;

import java.util.HashMap;
import java.util.Iterator;

import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.widget.PopupWindow;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.ScrollView;

public class DoorHangerPopup extends PopupWindow {
    private Context mContext;
    private LinearLayout mContent;

    public DoorHangerPopup(Context aContext) {
        super(aContext);
        mContext = aContext;

        setWindowLayoutMode(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);

        LayoutInflater inflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        ScrollView scrollContent = (ScrollView) inflater.inflate(R.layout.doorhangerpopup, null);
        mContent = (LinearLayout) scrollContent.findViewById(R.id.doorhanger_container);
        
        setContentView(scrollContent);
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
        for (int i=0; i < mContent.getChildCount(); i++) {
            DoorHanger dh = (DoorHanger) mContent.getChildAt(i);
            dh.hidePopup();
        }

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
        if (isShowing())
            update();
        else
            showAsDropDown(GeckoApp.mBrowserToolbar);
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

        hideAllDoorHangers();

        DoorHanger dh;
        Iterator keys = doorHangers.keySet().iterator();
        while (keys.hasNext()) {
            dh = (DoorHanger) doorHangers.get(keys.next());
            dh.showPopup();
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

        DoorHanger dh;
        Iterator keys = doorHangers.keySet().iterator();
        while (keys.hasNext()) {
            dh = (DoorHanger) doorHangers.get(keys.next());
            dh.hidePopup();
        }
    }
}
