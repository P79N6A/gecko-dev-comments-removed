














package com.squareup.picasso;

import android.net.Uri;
import java.util.ArrayList;
import java.util.List;

import static java.util.Collections.unmodifiableList;


public final class Request {
  




  public final Uri uri;
  




  public final int resourceId;
  
  public final List<Transformation> transformations;
  
  public final int targetWidth;
  
  public final int targetHeight;
  




  public final boolean centerCrop;
  




  public final boolean centerInside;
  
  public final float rotationDegrees;
  
  public final float rotationPivotX;
  
  public final float rotationPivotY;
  
  public final boolean hasRotationPivot;

  private Request(Uri uri, int resourceId, List<Transformation> transformations, int targetWidth,
      int targetHeight, boolean centerCrop, boolean centerInside, float rotationDegrees,
      float rotationPivotX, float rotationPivotY, boolean hasRotationPivot) {
    this.uri = uri;
    this.resourceId = resourceId;
    if (transformations == null) {
      this.transformations = null;
    } else {
      this.transformations = unmodifiableList(transformations);
    }
    this.targetWidth = targetWidth;
    this.targetHeight = targetHeight;
    this.centerCrop = centerCrop;
    this.centerInside = centerInside;
    this.rotationDegrees = rotationDegrees;
    this.rotationPivotX = rotationPivotX;
    this.rotationPivotY = rotationPivotY;
    this.hasRotationPivot = hasRotationPivot;
  }

  String getName() {
    if (uri != null) {
      return uri.getPath();
    }
    return Integer.toHexString(resourceId);
  }

  public boolean hasSize() {
    return targetWidth != 0;
  }

  boolean needsTransformation() {
    return needsMatrixTransform() || hasCustomTransformations();
  }

  boolean needsMatrixTransform() {
    return targetWidth != 0 || rotationDegrees != 0;
  }

  boolean hasCustomTransformations() {
    return transformations != null;
  }

  public Builder buildUpon() {
    return new Builder(this);
  }

  
  public static final class Builder {
    private Uri uri;
    private int resourceId;
    private int targetWidth;
    private int targetHeight;
    private boolean centerCrop;
    private boolean centerInside;
    private float rotationDegrees;
    private float rotationPivotX;
    private float rotationPivotY;
    private boolean hasRotationPivot;
    private List<Transformation> transformations;

    
    public Builder(Uri uri) {
      setUri(uri);
    }

    
    public Builder(int resourceId) {
      setResourceId(resourceId);
    }

    Builder(Uri uri, int resourceId) {
      this.uri = uri;
      this.resourceId = resourceId;
    }

    private Builder(Request request) {
      uri = request.uri;
      resourceId = request.resourceId;
      targetWidth = request.targetWidth;
      targetHeight = request.targetHeight;
      centerCrop = request.centerCrop;
      centerInside = request.centerInside;
      rotationDegrees = request.rotationDegrees;
      rotationPivotX = request.rotationPivotX;
      rotationPivotY = request.rotationPivotY;
      hasRotationPivot = request.hasRotationPivot;
      if (request.transformations != null) {
        transformations = new ArrayList<Transformation>(request.transformations);
      }
    }

    boolean hasImage() {
      return uri != null || resourceId != 0;
    }

    boolean hasSize() {
      return targetWidth != 0;
    }

    




    public Builder setUri(Uri uri) {
      if (uri == null) {
        throw new IllegalArgumentException("Image URI may not be null.");
      }
      this.uri = uri;
      this.resourceId = 0;
      return this;
    }

    




    public Builder setResourceId(int resourceId) {
      if (resourceId == 0) {
        throw new IllegalArgumentException("Image resource ID may not be 0.");
      }
      this.resourceId = resourceId;
      this.uri = null;
      return this;
    }

    
    public Builder resize(int targetWidth, int targetHeight) {
      if (targetWidth <= 0) {
        throw new IllegalArgumentException("Width must be positive number.");
      }
      if (targetHeight <= 0) {
        throw new IllegalArgumentException("Height must be positive number.");
      }
      this.targetWidth = targetWidth;
      this.targetHeight = targetHeight;
      return this;
    }

    
    public Builder clearResize() {
      targetWidth = 0;
      targetHeight = 0;
      centerCrop = false;
      centerInside = false;
      return this;
    }

    




    public Builder centerCrop() {
      if (centerInside) {
        throw new IllegalStateException("Center crop can not be used after calling centerInside");
      }
      centerCrop = true;
      return this;
    }

    
    public Builder clearCenterCrop() {
      centerCrop = false;
      return this;
    }

    



    public Builder centerInside() {
      if (centerCrop) {
        throw new IllegalStateException("Center inside can not be used after calling centerCrop");
      }
      centerInside = true;
      return this;
    }

    
    public Builder clearCenterInside() {
      centerInside = false;
      return this;
    }

    
    public Builder rotate(float degrees) {
      rotationDegrees = degrees;
      return this;
    }

    
    public Builder rotate(float degrees, float pivotX, float pivotY) {
      rotationDegrees = degrees;
      rotationPivotX = pivotX;
      rotationPivotY = pivotY;
      hasRotationPivot = true;
      return this;
    }

    
    public Builder clearRotation() {
      rotationDegrees = 0;
      rotationPivotX = 0;
      rotationPivotY = 0;
      hasRotationPivot = false;
      return this;
    }

    




    public Builder transform(Transformation transformation) {
      if (transformation == null) {
        throw new IllegalArgumentException("Transformation must not be null.");
      }
      if (transformations == null) {
        transformations = new ArrayList<Transformation>(2);
      }
      transformations.add(transformation);
      return this;
    }

    
    public Request build() {
      if (centerInside && centerCrop) {
        throw new IllegalStateException("Center crop and center inside can not be used together.");
      }
      if (centerCrop && targetWidth == 0) {
        throw new IllegalStateException("Center crop requires calling resize.");
      }
      if (centerInside && targetWidth == 0) {
        throw new IllegalStateException("Center inside requires calling resize.");
      }
      return new Request(uri, resourceId, transformations, targetWidth, targetHeight, centerCrop,
          centerInside, rotationDegrees, rotationPivotX, rotationPivotY, hasRotationPivot);
    }
  }
}
