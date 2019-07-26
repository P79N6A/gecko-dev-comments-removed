




package org.mozilla.gecko;

import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.widget.ArrowPopup;
import org.mozilla.gecko.prompts.PromptInput;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.os.Build;
import android.util.Log;
import android.view.View;
import android.widget.CheckBox;

import java.util.HashSet;
import java.util.List;

public class DoorHangerPopup extends ArrowPopup
                             implements GeckoEventListener,
                                        Tabs.OnTabsChangedListener,
                                        DoorHanger.OnButtonClickListener {
    private static final String LOGTAG = "GeckoDoorHangerPopup";

    
    
    private HashSet<DoorHanger> mDoorHangers;

    
    private boolean mDisabled;

    DoorHangerPopup(GeckoApp activity, View anchor) {
        super(activity, anchor);

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

    




    void disable() {
        mDisabled = true;
        updatePopup();
    }

    


    void enable() {
        mDisabled = false;
        updatePopup();
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

    




    void addDoorHanger(final int tabId, final String value, final String message,
                       final JSONArray buttons, final JSONObject options) {
        
        if (Tabs.getInstance().getTab(tabId) == null) {
            return;
        }

        
        DoorHanger oldDoorHanger = getDoorHanger(tabId, value);
        if (oldDoorHanger != null) {
            removeDoorHanger(oldDoorHanger);
        }

        if (!mInflated) {
            init();
        }

        final DoorHanger newDoorHanger = new DoorHanger(mActivity, tabId, value);
        newDoorHanger.setMessage(message);
        newDoorHanger.setOptions(options);

        for (int i = 0; i < buttons.length(); i++) {
            try {
                JSONObject buttonObject = buttons.getJSONObject(i);
                String label = buttonObject.getString("label");
                String tag = String.valueOf(buttonObject.getInt("callback"));
                newDoorHanger.addButton(label, tag, this);
            } catch (JSONException e) {
                Log.e(LOGTAG, "Error creating doorhanger button", e);
            }
        }

        mDoorHangers.add(newDoorHanger);
        mContent.addView(newDoorHanger);

        
        if (tabId == Tabs.getInstance().getSelectedTab().getId())
            updatePopup();
    }


    


    @Override
    public void onButtonClick(DoorHanger dh, String tag) {
        JSONObject response = new JSONObject();
        try {
            response.put("callback", tag);

            CheckBox checkBox = dh.getCheckBox();
            
            if (checkBox != null) {
                response.put("checked", checkBox.isChecked());
            }

            List<PromptInput> doorHangerInputs = dh.getInputs();
            if (doorHangerInputs != null) {
                JSONObject inputs = new JSONObject();
                for (PromptInput input : doorHangerInputs) {
                    inputs.put(input.getId(), input.getValue());
                }
                response.put("inputs", inputs);
            }
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error creating onClick response", e);
        }

        GeckoEvent e = GeckoEvent.createBroadcastEvent("Doorhanger:Reply", response.toString());
        GeckoAppShell.sendEventToGecko(e);
        removeDoorHanger(dh);
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
            
            if (dh.getTabId() == tabId && dh.shouldRemove(isShowing()))
                doorHangersToRemove.add(dh);
        }

        for (DoorHanger dh : doorHangersToRemove) {
            removeDoorHanger(dh);
        }
    }

    




    private void updatePopup() {
        
        
        
        
        Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab == null || mDoorHangers.size() == 0 || !mInflated || mDisabled) {
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

        
        
        
        if (Build.VERSION.SDK_INT >= 14) {
            setFocusable(true);
        }

        show();

        if (Build.VERSION.SDK_INT < 14) {
            
            setFocusable(true);
        }
    }

    
    private void showDividers() {
        int count = mContent.getChildCount();
        DoorHanger lastVisibleDoorHanger = null;

        for (int i = 0; i < count; i++) {
            DoorHanger dh = (DoorHanger) mContent.getChildAt(i);
            dh.showDivider();
            if (dh.getVisibility() == View.VISIBLE) {
                lastVisibleDoorHanger = dh;
            }
        }
        if (lastVisibleDoorHanger != null) {
            lastVisibleDoorHanger.hideDivider();
        }
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
