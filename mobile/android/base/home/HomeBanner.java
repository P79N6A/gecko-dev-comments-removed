




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageButton;
import android.widget.LinearLayout;

public class HomeBanner extends LinearLayout {

    public HomeBanner(Context context) {
        this(context, null);
    }

    public HomeBanner(Context context, AttributeSet attrs) {
        super(context, attrs);

        LayoutInflater.from(context).inflate(R.layout.home_banner, this);
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        
        
        final ImageButton closeButton = (ImageButton) findViewById(R.id.close);

        
        closeButton.getDrawable().setAlpha(127);

        closeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                HomeBanner.this.setVisibility(View.GONE);
            }
        });
    }

    public boolean isDismissed() {
        return (getVisibility() == View.GONE);
    }
}
