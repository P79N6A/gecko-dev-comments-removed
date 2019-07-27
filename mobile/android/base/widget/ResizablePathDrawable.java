




package org.mozilla.gecko.widget;

import android.content.res.ColorStateList;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.drawable.ShapeDrawable;
import android.graphics.drawable.shapes.Shape;

public class ResizablePathDrawable extends ShapeDrawable {
    private final ColorStateList colorStateList;
    private int currentColor;

    public ResizablePathDrawable(NonScaledPathShape shape, int color) {
        this(shape, ColorStateList.valueOf(color));
    }

    public ResizablePathDrawable(NonScaledPathShape shape, ColorStateList colorStateList) {
        super(shape);
        this.colorStateList = colorStateList;
        updateColor(getState());
    }

    private boolean updateColor(int[] stateSet) {
        int newColor = colorStateList.getColorForState(stateSet, Color.WHITE);
        if (newColor != currentColor) {
            currentColor = newColor;
            invalidateSelf();
            return true;
        }

        return false;
    }

    public Path getPath() {
        final NonScaledPathShape shape = (NonScaledPathShape) getShape();
        return shape.path;
    }

    @Override
    public boolean isStateful() {
        return true;
    }

    @Override
    protected void onDraw(Shape shape, Canvas canvas, Paint paint) {
        paint.setColor(currentColor);
        super.onDraw(shape, canvas, paint);
    }

    @Override
    protected boolean onStateChange(int[] stateSet) {
        return updateColor(stateSet);
    }

    




    public static class NonScaledPathShape extends Shape {
        private Path path;

        public NonScaledPathShape() {
            path = new Path();
        }

        @Override
        public void draw(Canvas canvas, Paint paint) {
            
            
            if (paint.getColor() == Color.TRANSPARENT) {
                return;
            }

            canvas.drawPath(path, paint);
        }

        protected Path getPath() {
            return path;
        }

        @Override
        public NonScaledPathShape clone() throws CloneNotSupportedException {
            final NonScaledPathShape clonedShape = (NonScaledPathShape) super.clone();
            clonedShape.path = new Path(path);
            return clonedShape;
        }
    }
}
