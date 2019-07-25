




package org.mozilla.gecko;

import java.util.HashMap;

import android.content.Context;
import android.graphics.drawable.BitmapDrawable;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.view.View;
import android.widget.PopupWindow;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;

import org.json.JSONArray;
import org.json.JSONObject;
import org.json.JSONException;

public class DoorHangerPopup extends PopupWindow {
    private static final String LOGTAG = "GeckoDoorHangerPopup";

    private Context mContext;
    private LinearLayout mContent;

    private boolean mInflated; 

    public DoorHangerPopup(Context aContext) {
        super(aContext);
        mContext = aContext;
        mInflated = false;
   }

    private void init() {
        setBackgroundDrawable(new BitmapDrawable());
        setOutsideTouchable(true);
        setWindowLayoutMode(GeckoApp.mAppContext.isTablet() ? ViewGroup.LayoutParams.WRAP_CONTENT : ViewGroup.LayoutParams.FILL_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT);

        LayoutInflater inflater = LayoutInflater.from(mContext);
        RelativeLayout layout = (RelativeLayout) inflater.inflate(R.layout.doorhangerpopup, null);
        mContent = (LinearLayout) layout.findViewById(R.id.doorhanger_container);
        
        setContentView(layout);
        mInflated = true;
    }

    public void addDoorHanger(String message, String value, JSONArray buttons,
                              Tab tab, JSONObject options, View v) {
        Log.i(LOGTAG, "Adding a DoorHanger to Tab: " + tab.getId());

        if (!mInflated)
            init();

        
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

        
        if (tab.equals(Tabs.getInstance().getSelectedTab()))
            updatePopup(v);
    }

    
    public void updatePopup() {
      updatePopup(null);
    }

    public void updatePopup(View v) {
        Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab == null) {
            hidePopup();
            return;
        }
        
        Log.i(LOGTAG, "Showing all doorhangers for tab: " + tab.getId());
 
        HashMap<String, DoorHanger> doorHangers = tab.getDoorHangers();
        
        if (doorHangers == null || doorHangers.size() == 0) {
            hidePopup();
            return;
        }

        if (!mInflated)
            init();

        
        for (int i = 0; i < mContent.getChildCount(); i++) {
            DoorHanger dh = (DoorHanger) mContent.getChildAt(i);
            dh.hide();
        }

        
        for (DoorHanger dh : doorHangers.values()) {
            dh.show();
        }

        if (v == null)
            showAtLocation(((GeckoApp)mContext).getView(), Gravity.TOP, 0, 0);
        else
            showPopup(v);
    }

    public void hidePopup() {
        if (isShowing()) {
            dismiss();
        }
    }

    public void showPopup(View v) {
        fixBackgroundForFirst();

        if (isShowing())
            update();
        else
            showAsDropDown(v);
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
