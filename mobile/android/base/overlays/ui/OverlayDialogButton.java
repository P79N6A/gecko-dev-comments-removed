




package org.mozilla.gecko.overlays.ui;

import android.util.AttributeSet;
import org.mozilla.gecko.R;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;








public class OverlayDialogButton extends LinearLayout {
    private static final String LOGTAG = "GeckoOverlayDialogButton";

    
    private ImageView icon;
    private TextView label;

    
    private String enabledLabel;
    private Drawable enabledIcon;

    
    private String disabledLabel;
    private Drawable disabledIcon;

    
    
    private OnClickListener enabledOnClickListener;
    private OnClickListener disabledOnClickListener;

    private boolean isEnabled = true;

    public OverlayDialogButton(Context context) {
        super(context);
        init(context);
    }

    public OverlayDialogButton(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    public OverlayDialogButton(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init(context);
    }

    private void init(Context context) {
        setOrientation(HORIZONTAL);
        setPadding(0, 0, 0, 0);
        setBackgroundResource(R.drawable.overlay_share_button_background);

        LayoutInflater.from(context).inflate(R.layout.overlay_share_button, this);
        icon = (ImageView) findViewById(R.id.overlaybtn_icon);
        label = (TextView) findViewById(R.id.overlaybtn_label);
    }

    public void setEnabledLabelAndIcon(String s, Drawable d) {
        enabledLabel = s;
        enabledIcon = d;

        if (isEnabled) {
            updateViews();
        }
    }

    public void setDisabledLabelAndIcon(String s, Drawable d) {
        disabledLabel = s;
        disabledIcon = d;

        if (!isEnabled) {
            updateViews();
        }
    }

    



    private void updateViews() {
        label.setEnabled(isEnabled);
        if (isEnabled) {
            label.setText(enabledLabel);
            icon.setImageDrawable(enabledIcon);
            super.setOnClickListener(enabledOnClickListener);
        } else {
            label.setText(disabledLabel);
            icon.setImageDrawable(disabledIcon);
            super.setOnClickListener(getPopListener());
        }
    }

    




    private OnClickListener getPopListener() {
        if (disabledOnClickListener == null) {
            disabledOnClickListener = new OnClickListener() {
                @Override
                public void onClick(View view) {
                    Animation anim = AnimationUtils.loadAnimation(getContext(), R.anim.overlay_pop);
                    icon.startAnimation(anim);
                }
            };
        }

        return disabledOnClickListener;
    }

    @Override
    public void setOnClickListener(OnClickListener l) {
        enabledOnClickListener = l;

        if (isEnabled) {
            updateViews();
        }
    }

    



    @Override
    public void setEnabled(boolean enabled) {
        if (enabled == isEnabled) {
            return;
        }

        isEnabled = enabled;
        updateViews();
    }
}
