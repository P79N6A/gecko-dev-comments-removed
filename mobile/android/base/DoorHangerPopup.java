





































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

import org.json.JSONArray;
import org.json.JSONObject;
import org.json.JSONException;

public class DoorHangerPopup extends PopupWindow {
    private static final String LOGTAG = "GeckoDoorHangerPopup";

    private Context mContext;
    private LinearLayout mContent;

    public DoorHangerPopup(Context aContext) {
        super(aContext);
        mContext = aContext;

        setBackgroundDrawable(new BitmapDrawable());
        setOutsideTouchable(true);
        setWindowLayoutMode(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);

        LayoutInflater inflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        RelativeLayout layout = (RelativeLayout) inflater.inflate(R.layout.doorhangerpopup, null);
        mContent = (LinearLayout) layout.findViewById(R.id.doorhanger_container);
        
        setContentView(layout);
    }

    public void addDoorHanger(String message, String value, JSONArray buttons,
                              Tab tab, JSONObject options) {
        Log.i(LOGTAG, "Adding a DoorHanger to Tab: " + tab.getId());

        
        DoorHanger dh = tab.getDoorHanger(value);
        if (dh != null) {
            tab.removeDoorHanger(value);
        }
        dh = new DoorHanger(mContent.getContext(), value);
 
        
        dh.setText(message);
        for (int i = 0; i < buttons.length(); i++) {
            try {
                JSONObject buttonObject = buttons.getJSONObject(i);
                String label = buttonObject.getString("label");
                int callBackId = buttonObject.getInt("callback");
                dh.addButton(label, callBackId);
            } catch (JSONException e) {
                Log.i(LOGTAG, "JSON throws " + e);
            }
         }
        dh.setOptions(options);

        dh.setTab(tab);
        tab.addDoorHanger(value, dh);
        mContent.addView(dh);

        updatePopup();
    }

    
    public void updatePopup() {
        Tab tab = Tabs.getInstance().getSelectedTab();
        Log.i(LOGTAG, "Showing all doorhangers for tab: " + tab.getId());
 
        HashMap<String, DoorHanger> doorHangers = tab.getDoorHangers();
        
        if (doorHangers == null || doorHangers.size() == 0) {
            hidePopup();
            return;
        }

        
        for (int i = 0; i < mContent.getChildCount(); i++) {
            DoorHanger dh = (DoorHanger) mContent.getChildAt(i);
            dh.hide();
        }

        
        for (DoorHanger dh : doorHangers.values()) {
            dh.show();
        }

        showPopup();
    }

    public void hidePopup() {
        if (isShowing()) {
            Log.i(LOGTAG, "Hiding the DoorHangerPopup");
            dismiss();
        }
    }

    public void showPopup() {
        Log.i(LOGTAG, "Showing the DoorHangerPopup");
        fixBackgroundForFirst();

        if (isShowing())
            update();
        else
            showAsDropDown(GeckoApp.mBrowserToolbar.mFavicon);
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
