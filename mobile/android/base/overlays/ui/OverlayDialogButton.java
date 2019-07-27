




package org.mozilla.gecko.overlays.ui;

import org.mozilla.gecko.R;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;








public class OverlayDialogButton extends LinearLayout {
    private static final String LOGTAG = "GeckoOverlayDialogButton";

    
    private boolean isEnabled = true;

    private final ImageView iconView;
    private final TextView labelView;

    private String enabledText = "";
    private String disabledText = "";

    private OnClickListener enabledOnClickListener;

    public OverlayDialogButton(Context context) {
        this(context, null);
    }

    public OverlayDialogButton(Context context, AttributeSet attrs) {
        super(context, attrs);

        setOrientation(LinearLayout.HORIZONTAL);

        LayoutInflater.from(context).inflate(R.layout.overlay_share_button, this);

        iconView = (ImageView) findViewById(R.id.overlaybtn_icon);
        labelView = (TextView) findViewById(R.id.overlaybtn_label);

        super.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {

                if (isEnabled) {
                    if (enabledOnClickListener != null) {
                        enabledOnClickListener.onClick(v);
                    } else {
                        Log.e(LOGTAG, "enabledOnClickListener is null.");
                    }
                } else {
                    Animation anim = AnimationUtils.loadAnimation(getContext(), R.anim.overlay_pop);
                    iconView.startAnimation(anim);
                }
            }
        });

        final TypedArray typedArray = context.obtainStyledAttributes(attrs, R.styleable.OverlayDialogButton);

        Drawable drawable = typedArray.getDrawable(R.styleable.OverlayDialogButton_drawable);
        if (drawable != null) {
            setDrawable(drawable);
        }

        String disabledText = typedArray.getString(R.styleable.OverlayDialogButton_disabledText);
        if (disabledText != null) {
            this.disabledText = disabledText;
        }

        String enabledText = typedArray.getString(R.styleable.OverlayDialogButton_enabledText);
        if (enabledText != null) {
            this.enabledText = enabledText;
        }

        typedArray.recycle();

        setEnabled(true);
    }

    public void setDrawable(Drawable drawable) {
        iconView.setImageDrawable(drawable);
    }

    public void setText(String text) {
        labelView.setText(text);
    }

    @Override
    public void setOnClickListener(OnClickListener listener) {
        enabledOnClickListener = listener;
    }

    



    @Override
    public void setEnabled(boolean enabled) {
        isEnabled = enabled;
        iconView.setEnabled(enabled);
        labelView.setEnabled(enabled);

        if (enabled) {
            setText(enabledText);
        } else {
            setText(disabledText);
        }
    }

}
