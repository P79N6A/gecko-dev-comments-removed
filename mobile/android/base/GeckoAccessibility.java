




package org.mozilla.gecko;

import org.mozilla.gecko.gfx.LayerView;

import android.view.accessibility.*;
import android.view.View;
import android.util.Log;
import android.os.Build;
import android.os.Bundle;
import android.content.Context;
import android.graphics.Rect;
import android.app.ActivityManager;
import android.app.ActivityManager.RunningServiceInfo;

import java.util.HashSet;
import java.util.Arrays;
import java.util.List;

import org.json.*;

public class GeckoAccessibility {
    private static final String LOGTAG = "GeckoAccessibility";
    private static final int VIRTUAL_CURSOR_PREVIOUS = 1;
    private static final int VIRTUAL_CURSOR_POSITION = 2;
    private static final int VIRTUAL_CURSOR_NEXT = 3;

    private static boolean sEnabled = false;
    private static JSONObject sEventMessage = null;
    private static AccessibilityNodeInfo sVirtualCursorNode = null;

    private static final HashSet<String> sServiceWhitelist =
        new HashSet<String>(Arrays.asList(new String[] {
                    "com.google.android.marvin.talkback.TalkBackService", 
                    "com.mot.readout.ScreenReader", 
                    "info.spielproject.spiel.SpielService", 
                    "es.codefactory.android.app.ma.MAAccessibilityService" 
                }));

    public static void updateAccessibilitySettings () {
        GeckoAppShell.getHandler().post(new Runnable() {
                @Override
                public void run() {
                    JSONObject ret = new JSONObject();
                    sEnabled = false;
                    AccessibilityManager accessibilityManager =
                        (AccessibilityManager) GeckoApp.mAppContext.getSystemService(Context.ACCESSIBILITY_SERVICE);
                    if (accessibilityManager.isEnabled()) {
                        ActivityManager activityManager =
                            (ActivityManager) GeckoApp.mAppContext.getSystemService(Context.ACTIVITY_SERVICE);
                        List<RunningServiceInfo> runningServices = activityManager.getRunningServices(Integer.MAX_VALUE);

                        for (RunningServiceInfo runningServiceInfo : runningServices) {
                            sEnabled = sServiceWhitelist.contains(runningServiceInfo.service.getClassName());
                            if (sEnabled)
                                break;
                        }
                    }

                    try {
                        ret.put("enabled", sEnabled);
                    } catch (Exception ex) {
                        Log.e(LOGTAG, "Error building JSON arguments for Accessibility:Settings:", ex);
                    }

                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Accessibility:Settings",
                                                                                   ret.toString()));
                }
            });
    }

    private static void populateEventFromJSON (AccessibilityEvent event, JSONObject message) {
        final JSONArray textArray = message.optJSONArray("text");
        if (textArray != null) {
            for (int i = 0; i < textArray.length(); i++)
                event.getText().add(textArray.optString(i));
        }

        event.setContentDescription(message.optString("description"));
        event.setEnabled(message.optBoolean("enabled", true));
        event.setChecked(message.optBoolean("checked"));
        event.setPassword(message.optBoolean("password"));
        event.setAddedCount(message.optInt("addedCount", -1));
        event.setRemovedCount(message.optInt("removedCount", -1));
        event.setFromIndex(message.optInt("fromIndex", -1));
        event.setItemCount(message.optInt("itemCount", -1));
        event.setCurrentItemIndex(message.optInt("currentItemIndex", -1));
        event.setBeforeText(message.optString("beforeText"));
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
            event.setToIndex(message.optInt("toIndex", -1));
            event.setScrollable(message.optBoolean("scrollable"));
            event.setScrollX(message.optInt("scrollX", -1));
            event.setScrollY(message.optInt("scrollY", -1));
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH_MR1) {
            event.setMaxScrollX(message.optInt("maxScrollX", -1));
            event.setMaxScrollY(message.optInt("maxScrollY", -1));
        }
    }

    private static void sendDirectAccessibilityEvent(int eventType, JSONObject message) {
        final AccessibilityEvent accEvent = AccessibilityEvent.obtain(eventType);
        accEvent.setClassName(GeckoAccessibility.class.getName());
        accEvent.setPackageName(GeckoApp.mAppContext.getPackageName());
        populateEventFromJSON(accEvent, message);
        AccessibilityManager accessibilityManager =
            (AccessibilityManager) GeckoApp.mAppContext.getSystemService(Context.ACCESSIBILITY_SERVICE);
        try {
            accessibilityManager.sendAccessibilityEvent(accEvent);
        } catch (IllegalStateException e) {
            
        }
    }

    public static void sendAccessibilityEvent (final JSONObject message) {
        if (!sEnabled)
            return;

        final int eventType = message.optInt("eventType", -1);
        if (eventType < 0) {
            Log.e(LOGTAG, "No accessibility event type provided");
            return;
        }

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN) {
            
            
            GeckoAppShell.getHandler().post(new Runnable() {
                    @Override
                    public void run() {
                        sendDirectAccessibilityEvent(eventType, message);
                }
            });
        } else {
            
            
            final LayerView view = GeckoApp.mAppContext.getLayerView();
            if (view == null)
                return;

            if (sVirtualCursorNode == null)
                sVirtualCursorNode = AccessibilityNodeInfo.obtain(view, VIRTUAL_CURSOR_POSITION);
            sVirtualCursorNode.setEnabled(message.optBoolean("enabled", true));
            sVirtualCursorNode.setChecked(message.optBoolean("checked"));
            sVirtualCursorNode.setPassword(message.optBoolean("password"));
            JSONObject bounds = message.optJSONObject("bounds");
            if (bounds != null) {
                Rect relativeBounds = new Rect(bounds.optInt("left"), bounds.optInt("top"),
                                               bounds.optInt("right"), bounds.optInt("bottom"));
                sVirtualCursorNode.setBoundsInParent(relativeBounds);
                int[] locationOnScreen = new int[2];
                view.getLocationOnScreen(locationOnScreen);
                Rect screenBounds = new Rect(relativeBounds);
                screenBounds.offset(locationOnScreen[0], locationOnScreen[1]);
                sVirtualCursorNode.setBoundsInScreen(screenBounds);
            }

            
            sEventMessage = message;
            GeckoApp.mAppContext.mMainHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        
                        
                        switch (eventType) {
                        case AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUSED:
                            view.performAccessibilityAction(AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS, null);
                            break;
                        case AccessibilityEvent.TYPE_ANNOUNCEMENT:
                        case AccessibilityEvent.TYPE_VIEW_SCROLLED:
                            final AccessibilityEvent accEvent = AccessibilityEvent.obtain(eventType);
                            view.onInitializeAccessibilityEvent(accEvent);
                            populateEventFromJSON(accEvent, message);
                            view.getParent().requestSendAccessibilityEvent(view, accEvent);
                            break;
                        default:
                            view.sendAccessibilityEvent(eventType);
                            break;
                        }
                    }
                });

        }
    }

    public static void setDelegate(LayerView layerview) {
        
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            layerview.setAccessibilityDelegate(new GeckoAccessibilityDelegate());
            layerview.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_YES);
        }
    }

    public static void onLayerViewFocusChanged(LayerView layerview, boolean gainFocus) {
        if (sEnabled)
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Accessibility:Focus",
                                                                           gainFocus ? "true" : "false"));
    }

    public static class GeckoAccessibilityDelegate extends View.AccessibilityDelegate {
        AccessibilityNodeProvider mAccessibilityNodeProvider;

        @Override
        public void onPopulateAccessibilityEvent (View host, AccessibilityEvent event) {
            super.onPopulateAccessibilityEvent(host, event);
            if (sEventMessage != null)
                populateEventFromJSON(event, sEventMessage);
            
            if (event.getEventType() != AccessibilityEvent.TYPE_VIEW_HOVER_ENTER)
                sEventMessage = null;
            
            event.setSource(host, VIRTUAL_CURSOR_POSITION);
        }

        @Override
        public AccessibilityNodeProvider getAccessibilityNodeProvider(final View host) {
            if (mAccessibilityNodeProvider == null)
                
                
                
                
                
                mAccessibilityNodeProvider = new AccessibilityNodeProvider() {
                        @Override
                        public AccessibilityNodeInfo createAccessibilityNodeInfo(int virtualDescendantId) {
                            AccessibilityNodeInfo info = (virtualDescendantId == VIRTUAL_CURSOR_POSITION && sVirtualCursorNode != null) ?
                                AccessibilityNodeInfo.obtain(sVirtualCursorNode) :
                                AccessibilityNodeInfo.obtain(host, virtualDescendantId);


                            switch (virtualDescendantId) {
                            case View.NO_ID:
                                
                                onInitializeAccessibilityNodeInfo(host, info);
                                info.addChild(host, VIRTUAL_CURSOR_PREVIOUS);
                                info.addChild(host, VIRTUAL_CURSOR_POSITION);
                                info.addChild(host, VIRTUAL_CURSOR_NEXT);
                                break;
                            default:
                                info.setParent(host);
                                info.setSource(host, virtualDescendantId);
                                info.setVisibleToUser(true);
                                info.setPackageName(GeckoApp.mAppContext.getPackageName());
                                info.setClassName(host.getClass().getName());
                                info.addAction(AccessibilityNodeInfo.ACTION_CLEAR_ACCESSIBILITY_FOCUS);
                                info.addAction(AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS);
                                info.addAction(AccessibilityNodeInfo.ACTION_CLICK);
                                break;
                            }

                            return info;
                        }

                        @Override
                        public boolean performAction (int virtualViewId, int action, Bundle arguments) {
                            if (action == AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS) {
                                
                                
                                

                                switch (virtualViewId) {
                                case VIRTUAL_CURSOR_PREVIOUS:
                                    GeckoAppShell.
                                        sendEventToGecko(GeckoEvent.createBroadcastEvent("Accessibility:PreviousObject", null));
                                    return true;
                                case VIRTUAL_CURSOR_NEXT:
                                    GeckoAppShell.
                                        sendEventToGecko(GeckoEvent.createBroadcastEvent("Accessibility:NextObject", null));
                                    return true;
                                default:
                                    break;
                                }
                            }
                            return host.performAccessibilityAction(action, arguments);
                        }
                    };

            return mAccessibilityNodeProvider;
        }
    }
}
