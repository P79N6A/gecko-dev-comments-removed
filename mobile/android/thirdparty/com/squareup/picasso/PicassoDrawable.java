














package com.squareup.picasso;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.SystemClock;
import android.widget.ImageView;

import static android.graphics.Color.WHITE;
import static com.squareup.picasso.Picasso.LoadedFrom.MEMORY;

final class PicassoDrawable extends Drawable {
  
  private static final Paint DEBUG_PAINT = new Paint();

  private static final float FADE_DURATION = 200f; 

  



  static void setBitmap(ImageView target, Context context, Bitmap bitmap,
      Picasso.LoadedFrom loadedFrom, boolean noFade, boolean debugging) {
    Drawable placeholder = target.getDrawable();
    if (placeholder instanceof AnimationDrawable) {
      ((AnimationDrawable) placeholder).stop();
    }
    PicassoDrawable drawable =
        new PicassoDrawable(context, placeholder, bitmap, loadedFrom, noFade, debugging);
    target.setImageDrawable(drawable);
  }

  



  static void setPlaceholder(ImageView target, int placeholderResId, Drawable placeholderDrawable) {
    if (placeholderResId != 0) {
      target.setImageResource(placeholderResId);
    } else {
      target.setImageDrawable(placeholderDrawable);
    }
    if (target.getDrawable() instanceof AnimationDrawable) {
      ((AnimationDrawable) target.getDrawable()).start();
    }
  }

  private final boolean debugging;
  private final float density;
  private final Picasso.LoadedFrom loadedFrom;
  final BitmapDrawable image;

  Drawable placeholder;

  long startTimeMillis;
  boolean animating;
  int alpha = 0xFF;

  PicassoDrawable(Context context, Drawable placeholder, Bitmap bitmap,
      Picasso.LoadedFrom loadedFrom, boolean noFade, boolean debugging) {
    Resources res = context.getResources();

    this.debugging = debugging;
    this.density = res.getDisplayMetrics().density;

    this.loadedFrom = loadedFrom;

    this.image = new BitmapDrawable(res, bitmap);

    boolean fade = loadedFrom != MEMORY && !noFade;
    if (fade) {
      this.placeholder = placeholder;
      animating = true;
      startTimeMillis = SystemClock.uptimeMillis();
    }
  }

  @Override public void draw(Canvas canvas) {
    if (!animating) {
      image.draw(canvas);
    } else {
      float normalized = (SystemClock.uptimeMillis() - startTimeMillis) / FADE_DURATION;
      if (normalized >= 1f) {
        animating = false;
        placeholder = null;
        image.draw(canvas);
      } else {
        if (placeholder != null) {
          placeholder.draw(canvas);
        }

        int partialAlpha = (int) (alpha * normalized);
        image.setAlpha(partialAlpha);
        image.draw(canvas);
        image.setAlpha(alpha);
        invalidateSelf();
      }
    }

    if (debugging) {
      drawDebugIndicator(canvas);
    }
  }

  @Override public int getIntrinsicWidth() {
    return image.getIntrinsicWidth();
  }

  @Override public int getIntrinsicHeight() {
    return image.getIntrinsicHeight();
  }

  @Override public void setAlpha(int alpha) {
    this.alpha = alpha;
    if (placeholder != null) {
      placeholder.setAlpha(alpha);
    }
    image.setAlpha(alpha);
  }

  @Override public void setColorFilter(ColorFilter cf) {
    if (placeholder != null) {
      placeholder.setColorFilter(cf);
    }
    image.setColorFilter(cf);
  }

  @Override public int getOpacity() {
    return image.getOpacity();
  }

  @Override protected void onBoundsChange(Rect bounds) {
    super.onBoundsChange(bounds);

    image.setBounds(bounds);
    if (placeholder != null) {
      placeholder.setBounds(bounds);
    }
  }

  private void drawDebugIndicator(Canvas canvas) {
    DEBUG_PAINT.setColor(WHITE);
    Path path = getTrianglePath(new Point(0, 0), (int) (16 * density));
    canvas.drawPath(path, DEBUG_PAINT);

    DEBUG_PAINT.setColor(loadedFrom.debugColor);
    path = getTrianglePath(new Point(0, 0), (int) (15 * density));
    canvas.drawPath(path, DEBUG_PAINT);
  }

  private static Path getTrianglePath(Point p1, int width) {
    Point p2 = new Point(p1.x + width, p1.y);
    Point p3 = new Point(p1.x, p1.y + width);

    Path path = new Path();
    path.moveTo(p1.x, p1.y);
    path.lineTo(p2.x, p2.y);
    path.lineTo(p3.x, p3.y);

    return path;
  }
}
