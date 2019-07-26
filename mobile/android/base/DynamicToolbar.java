package org.mozilla.gecko;

import java.util.EnumSet;

import org.mozilla.gecko.PrefsHelper.PrefHandlerBase;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.util.ThreadUtils;

import android.os.Bundle;

public class DynamicToolbar {
    private static final String STATE_ENABLED = "dynamic_toolbar";
    private static final String CHROME_PREF = "browser.chrome.dynamictoolbar";

    
    
    
    private volatile boolean prefEnabled;
    private boolean accessibilityEnabled;

    private final int prefObserverId;
    private final EnumSet<PinReason> pinFlags = EnumSet.noneOf(PinReason.class);
    private LayerView layerView;
    private OnEnabledChangedListener enabledChangedListener;

    public enum PinReason {
        RELAYOUT,
        ACTION_MODE
    }

    public enum VisibilityTransition {
        IMMEDIATE,
        ANIMATE
    }

    


    public interface OnEnabledChangedListener {
        


        public void onEnabledChanged(boolean enabled);
    }

    public DynamicToolbar() {
        
        prefObserverId = PrefsHelper.getPref(CHROME_PREF, new PrefHandler());
    }

    public void destroy() {
        PrefsHelper.removeObserver(prefObserverId);
    }

    public void setLayerView(LayerView layerView) {
        ThreadUtils.assertOnUiThread();

        this.layerView = layerView;
    }

    public void setEnabledChangedListener(OnEnabledChangedListener listener) {
        ThreadUtils.assertOnUiThread();

        enabledChangedListener = listener;
    }

    public void onSaveInstanceState(Bundle outState) {
        ThreadUtils.assertOnUiThread();

        outState.putBoolean(STATE_ENABLED, prefEnabled);
    }

    public void onRestoreInstanceState(Bundle savedInstanceState) {
        ThreadUtils.assertOnUiThread();

        if (savedInstanceState != null) {
            prefEnabled = savedInstanceState.getBoolean(STATE_ENABLED);
        }
    }

    public boolean isEnabled() {
        ThreadUtils.assertOnUiThread();

        return prefEnabled && !accessibilityEnabled;
    }

    public void setAccessibilityEnabled(boolean enabled) {
        ThreadUtils.assertOnUiThread();

        if (accessibilityEnabled == enabled) {
            return;
        }

        
        
        accessibilityEnabled = enabled;
        if (prefEnabled) {
            triggerEnabledListener();
        }
    }

    public void setVisible(boolean visible, VisibilityTransition transition) {
        ThreadUtils.assertOnUiThread();

        if (layerView == null) {
            return;
        }

        final boolean immediate = transition.equals(VisibilityTransition.ANIMATE);
        if (visible) {
            layerView.getLayerMarginsAnimator().showMargins(immediate);
        } else {
            layerView.getLayerMarginsAnimator().hideMargins(immediate);
        }
    }

    public void setPinned(boolean pinned, PinReason reason) {
        ThreadUtils.assertOnUiThread();

        if (layerView == null) {
            return;
        }

        if (pinned) {
            pinFlags.add(reason);
        } else {
            pinFlags.remove(reason);
        }

        layerView.getLayerMarginsAnimator().setMarginsPinned(!pinFlags.isEmpty());
    }

    private void triggerEnabledListener() {
        if (enabledChangedListener != null) {
            enabledChangedListener.onEnabledChanged(isEnabled());
        }
    }

    private class PrefHandler extends PrefHandlerBase {
        @Override
        public void prefValue(String pref, boolean value) {
            if (value == prefEnabled) {
                return;
            }

            prefEnabled = value;

            ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    
                    
                    if (!accessibilityEnabled) {
                        triggerEnabledListener();
                    }
                }
            });
        }

        @Override
        public boolean isObserver() {
            
            
            return true;
        }
    }
}