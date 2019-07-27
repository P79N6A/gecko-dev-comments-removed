




package org.mozilla.gecko.toolbar;

import java.util.Arrays;

import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.util.AttributeSet;
import android.widget.ImageView;





abstract class BrowserToolbarPhoneBase extends BrowserToolbar {

    protected final ImageView urlBarTranslatingEdge;

    private final Path roundCornerShape;
    private final Paint roundCornerPaint;

    public BrowserToolbarPhoneBase(final Context context, final AttributeSet attrs) {
        super(context, attrs);
        final Resources res = context.getResources();

        urlBarTranslatingEdge = (ImageView) findViewById(R.id.url_bar_translating_edge);

        
        urlBarTranslatingEdge.getDrawable().setLevel(6000);

        focusOrder.add(this);
        focusOrder.addAll(urlDisplayLayout.getFocusOrder());
        focusOrder.addAll(Arrays.asList(tabsButton, menuButton));

        roundCornerShape = new Path();
        roundCornerShape.moveTo(0, 0);
        roundCornerShape.lineTo(30, 0);
        roundCornerShape.cubicTo(0, 0, 0, 0, 0, 30);
        roundCornerShape.lineTo(0, 0);

        roundCornerPaint = new Paint();
        roundCornerPaint.setAntiAlias(true);
        roundCornerPaint.setColor(res.getColor(R.color.background_tabs));
        roundCornerPaint.setStrokeWidth(0.0f);
    }

    @Override
    protected void updateNavigationButtons(final Tab tab) {
        
    }

    @Override
    public void draw(final Canvas canvas) {
        super.draw(canvas);

        if (uiMode == UIMode.DISPLAY) {
            canvas.drawPath(roundCornerShape, roundCornerPaint);
        }
    }

    




    protected int getUrlBarEntryTranslation() {
        
        
        
        return editCancel.getLeft() - urlBarEntry.getRight();
    }

    protected int getUrlBarCurveTranslation() {
        return getWidth() - tabsButton.getLeft();
    }
}
