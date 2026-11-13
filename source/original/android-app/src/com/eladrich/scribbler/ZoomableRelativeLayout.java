package com.eladrich.scribbler;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.RelativeLayout;

public class ZoomableRelativeLayout extends RelativeLayout {
float mScaleFactor = 1;
float mPivotX;
float mPivotY;
Matrix mat;

public ZoomableRelativeLayout(Context context) {
    super(context);
    mat = new Matrix();
    // TODO Auto-generated constructor stub
}

public ZoomableRelativeLayout(Context context, AttributeSet attrs) {
    super(context, attrs);
    mat = new Matrix();
    // TODO Auto-generated constructor stub
}

public ZoomableRelativeLayout(Context context, AttributeSet attrs,
        int defStyle) {
    super(context, attrs, defStyle);
    mat = new Matrix();
    // TODO Auto-generated constructor stub
}

protected void dispatchDraw(Canvas canvas) {
    canvas.save();
	//canvas.setMatrix(mat);
	 

    canvas.scale(mScaleFactor, mScaleFactor, mPivotX, mPivotY);
    super.dispatchDraw(canvas);
    canvas.restore();
}

public void scale(float scaleFactor, float pivotX, float pivotY) {
    mScaleFactor = scaleFactor;
    mPivotX = pivotX;
    mPivotY = pivotY;
/*    Log.d("Scaling", "From " + mPivotX + "," + mPivotY);
    float[] xy = {mPivotX, mPivotY};
    Matrix inverse = new Matrix();
    (this).getMatrix().invert(inverse);
    inverse.mapPoints(xy);
    mPivotX = xy[0];
    mPivotY = xy[1];
    mat.preScale(mScaleFactor, mScaleFactor, mPivotX, mPivotY);*/
//    Log.d("Scaling", "To " + mPivotX + "," + mPivotY);
    this.invalidate();
}

public void updateMat(float scaleBy) {
	 mat.preScale(scaleBy, scaleBy, mPivotX, mPivotY);

}

public Matrix getMat() {
	return mat;
}

public void restore() {
    mScaleFactor = 1;
    this.invalidate();
}

}