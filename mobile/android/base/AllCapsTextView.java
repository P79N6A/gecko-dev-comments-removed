



package org.mozilla.gecko;

import android.content.Context;
import android.util.AttributeSet;

public class AllCapsTextView extends GeckoTextView {

    public AllCapsTextView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void setText(CharSequence text, BufferType type) {
        super.setText(text.toString().toUpperCase(), type);
    }
}
