package com.eladrich.scribbler;

import java.util.ArrayList;

import android.R.bool;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapShader;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Shader;
import android.graphics.Shader.TileMode;
import android.util.AttributeSet;
import android.widget.ImageView;

public class DrawableImageView extends ImageView {

	/** constants */
	private static final float TOUCH_TOLERANCE = 4; //Defined by Google :P

	
	/** enum for scribble type*/
	private enum Type {
		BACK, FRONT
	};

	/** local Fields*/
	private float mX, mY;
	private Paint backPaint, frontPaint, tracePaint;
	private ArrayList<Tuple<Path, Type>> paths;
	private Shader magShader;
	private Paint magPaint;
	private Matrix magMatrix;
	private float magX, magY;
	private boolean zooming = false;

	public DrawableImageView(Context context) {
		super(context);
		initializePaint();
	}

	public DrawableImageView(Context context, AttributeSet attrs) {
		super(context, attrs);
		initializePaint();
	}

	/**
	 * Initialize the backPaint and frontPaint
	 */
	private void initializePaint() {
		this.setWillNotDraw(false);
		backPaint = new Paint();
		backPaint.setAntiAlias(true);
		backPaint.setDither(true);
		backPaint.setColor(0xFF940B10);
		backPaint.setStyle(Paint.Style.STROKE);
		backPaint.setStrokeJoin(Paint.Join.ROUND);
		backPaint.setStrokeCap(Paint.Cap.ROUND);
		backPaint.setStrokeWidth(6);

		frontPaint = new Paint();
		frontPaint.setAntiAlias(true);
		frontPaint.setDither(true);
		frontPaint.setColor(0xFF259B24);
		frontPaint.setStyle(Paint.Style.STROKE);
		frontPaint.setStrokeJoin(Paint.Join.ROUND);
		frontPaint.setStrokeCap(Paint.Cap.ROUND);
		frontPaint.setStrokeWidth(6);
		
		tracePaint = new Paint();
		tracePaint.setAntiAlias(true);
		tracePaint.setDither(true);
		tracePaint.setColor(0xFF8278FF);
		tracePaint.setStyle(Paint.Style.FILL);
		tracePaint.setStrokeJoin(Paint.Join.ROUND);
		tracePaint.setStrokeCap(Paint.Cap.ROUND);
		tracePaint.setStrokeWidth(5);
		paths = new ArrayList<Tuple<Path, Type>>();
	}

	/**
	 * Respond to a new touch on the screen
	 * @param x - the x coordinate
	 * @param y - the y coordinate
	 * @param isFrontScribble
	 */
	public void touch_start(float x, float y, boolean isFrontScribble) {
		zooming = true;
		if (isFrontScribble) {
			paths.add(new Tuple<Path, DrawableImageView.Type>(new Path(),
					Type.FRONT));
		} else {
			paths.add(new Tuple<Path, DrawableImageView.Type>(new Path(),
					Type.BACK));
		}
		paths.get(paths.size() - 1).first.reset();
		paths.get(paths.size() - 1).first.moveTo(x, y);
		mX = x;
		mY = y;
	}
	
	/**
	 * flush the scribbles for next draw
	 */
	public void flushScribbles() {
		paths = new ArrayList<Tuple<Path, Type>>();
	}
	
	public void setMagnifyBitmap(Bitmap map) {
		magShader = new BitmapShader(map, TileMode.CLAMP, TileMode.CLAMP);
		magPaint = new Paint();
		magPaint.setShader(magShader);
		magPaint.setShadowLayer(10, 1, 1, Color.GRAY);
		 setLayerType(LAYER_TYPE_SOFTWARE, magPaint);
		magMatrix = new Matrix();
	}


	public void touch_move(float x, float y, float imgX, float imgY) {
		magX = imgX;
		magY = imgY;
		float dx = Math.abs(x - mX);
		float dy = Math.abs(y - mY);
		if (dx >= TOUCH_TOLERANCE || dy >= TOUCH_TOLERANCE) {
			paths.get(paths.size() - 1).first.quadTo(mX, mY, (x + mX) / 2,
					(y + mY) / 2);
			mX = x;
			mY = y;
		}
	}

	public void touch_up() {
		zooming = false;
		paths.get(paths.size() - 1).first.lineTo(mX, mY);
	}

	/**
	 * Draw the image and the picture on it
	 */
	@Override
	protected void onDraw(Canvas canvas) {
		//super.onDraw(canvas);
		for (Tuple<Path, DrawableImageView.Type> mPath : paths) {
			switch (mPath.second) {
			case BACK:
				canvas.drawPath(mPath.first, backPaint);
				break;
			case FRONT:
				canvas.drawPath(mPath.first, frontPaint);
			}

		}
		if (zooming && magShader != null) {
			float cx, cy;
			cx = mX > 100 ? mX -100 : mX + 100;
			cy = mY > 100 ? mY -100 : mY + 100;
			magMatrix.reset();
			magMatrix.postScale(1f, 1f);
			magMatrix.postTranslate(cx - magX, cy - magY); //Thinking usually sovles the problem :)
			magShader.setLocalMatrix(magMatrix);
			canvas.drawCircle(cx, cy, 50, magPaint);
			canvas.drawCircle(cx, cy, 5, tracePaint);
		}
	}

}
