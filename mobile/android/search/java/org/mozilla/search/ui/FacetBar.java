



package org.mozilla.search.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.widget.RadioButton;
import android.widget.RadioGroup;

import org.mozilla.search.R;

public class FacetBar extends RadioGroup {

    
    
    
    private static final RadioGroup.LayoutParams FACET_LAYOUT_PARAMS =
            new RadioGroup.LayoutParams(0, LayoutParams.MATCH_PARENT, 1.0f);

    
    private int underlineColor = Color.RED;

    
    private int nextButtonId = 0;

    public FacetBar(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    




    public void addFacet(String facetName) {
        addFacet(facetName, false);
    }

    






    public void addFacet(String facetName, boolean checked) {
        final FacetButton button = new FacetButton(getContext(), facetName, underlineColor);

        
        
        
        button.setId(nextButtonId++);

        
        button.setLayoutParams(FACET_LAYOUT_PARAMS);

        
        button.setChecked(checked);

        addView(button);
    }

    


    public void setUnderlineColor(int underlineColor) {
        this.underlineColor = underlineColor;

        if (getChildCount() > 0) {
            for (int i = 0; i < getChildCount(); i++) {
                ((FacetButton) getChildAt(i)).setUnderlineColor(underlineColor);
            }
        }
    }

    



    private static class FacetButton extends RadioButton {

        private final Paint underlinePaint = new Paint();

        public FacetButton(Context context, String text, int color) {
            super(context, null, R.attr.facetButtonStyle);

            setText(text);

            underlinePaint.setStyle(Paint.Style.STROKE);
            underlinePaint.setStrokeWidth(getResources().getDimension(R.dimen.facet_button_underline_thickness));
            underlinePaint.setColor(color);
        }

        @Override
        public void setChecked(boolean checked) {
            super.setChecked(checked);

            
            invalidate();
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            if (isChecked()) {
                
                
                
                
                final float yPos = getHeight() - underlinePaint.getStrokeWidth() / 2;
                canvas.drawLine(0, yPos, getWidth(), yPos, underlinePaint);
            }
        }

        public void setUnderlineColor(int color) {
            underlinePaint.setColor(color);
        }
    }
}
