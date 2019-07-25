




































package org.mozilla.gecko;

import java.util.ArrayList;

import android.util.Log;
import android.content.Context;
import android.widget.PopupWindow;
import android.widget.LinearLayout;

public class DoorHanger {
    private Context mContext;
    public ArrayList<DoorHangerPopup> mPopups;

    private final int POPUP_VERTICAL_SPACING = 10;
    private final int POPUP_VERTICAL_OFFSET = 10;

    public DoorHanger(Context aContext) {
        mContext = aContext;
        mPopups = new ArrayList<DoorHangerPopup>();
    }

    public DoorHangerPopup getPopup(String value) {
        
        for (DoorHangerPopup dhp : mPopups) {
            if (dhp.getValue().equals(value)) {
                
                dhp = new DoorHangerPopup(mContext, value);
                return dhp;
            }
        }
        
        final DoorHangerPopup dhp = new DoorHangerPopup(mContext, value);
        mPopups.add(dhp);
        return dhp;
    }

    public void removeForTab(int tabId) {
        Log.i("DoorHanger", "removeForTab: " + tabId);
        ArrayList<DoorHangerPopup> removeThis = new ArrayList<DoorHangerPopup>();
        for (DoorHangerPopup dhp : mPopups) {
            if (dhp.mTabId == tabId) {
                removeThis.add(dhp);
            }
        }
        for (DoorHangerPopup dhp : removeThis) {
            removePopup(dhp);
        }
    }

    public void removePopup(DoorHangerPopup dhp) {
        dhp.setOnDismissListener(null);
        dhp.dismiss();
        mPopups.remove(dhp);
    }


    public void updateForTab(int tabId) {
        Log.i("DoorHanger", "updateForTab: " + tabId);
        int yOffset = POPUP_VERTICAL_OFFSET;
        for (final DoorHangerPopup dhp : mPopups) {
            if (dhp.mTabId == tabId) {
                dhp.setOnDismissListener(new PopupWindow.OnDismissListener() {
                    @Override
                    public void onDismiss() {
                        removePopup(dhp);
                    }
                });
                dhp.showAtHeight(POPUP_VERTICAL_SPACING + yOffset);
                yOffset += dhp.getHeight() + POPUP_VERTICAL_SPACING;
            } else {
                dhp.setOnDismissListener(null);
                dhp.dismiss();
            }
        }
    }
}
