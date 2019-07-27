



package org.mozilla.gecko.toolbar;

import org.mozilla.gecko.AppConstants.Versions;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PorterDuff.Mode;
import android.graphics.PorterDuffXfermode;
import android.graphics.Shader;

class CanvasDelegate {
    Paint mPaint;
    PorterDuffXfermode mMode;
    DrawManager mDrawManager;

    
    static interface DrawManager {
        public void defaultDraw(Canvas cavas);
    }

    CanvasDelegate(DrawManager drawManager, Mode mode) {
        mDrawManager = drawManager;

        
        mMode = new PorterDuffXfermode(mode);

        mPaint = new Paint();
        mPaint.setAntiAlias(true);
        mPaint.setColor(0xFFFF0000);
        mPaint.setStrokeWidth(0.0f);
    }

    void draw(Canvas canvas, Path path, int width, int height) {
        
        int count = canvas.saveLayer(0, 0, width, height, null,
                                     Canvas.MATRIX_SAVE_FLAG |
                                     Canvas.CLIP_SAVE_FLAG |
                                     Canvas.HAS_ALPHA_LAYER_SAVE_FLAG |
                                     Canvas.FULL_COLOR_LAYER_SAVE_FLAG |
                                     Canvas.CLIP_TO_LAYER_SAVE_FLAG);

        
        mDrawManager.defaultDraw(canvas);

        if (path != null && !path.isEmpty()) {
            
            
            if (Versions.feature14Plus) {
                mPaint.setXfermode(mMode);
                canvas.drawPath(path, mPaint);
            } else {
                
                Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
                (new Canvas(bitmap)).drawPath(path, mPaint);

                mPaint.setXfermode(mMode);
                canvas.drawBitmap(bitmap, 0, 0, mPaint);
                bitmap.recycle();

                mPaint.setXfermode(null);
            }
        }

        
        canvas.restoreToCount(count);
    }

    void setShader(Shader shader) {
        mPaint.setShader(shader);
    }
}
