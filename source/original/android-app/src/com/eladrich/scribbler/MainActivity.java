package com.eladrich.scribbler;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.android.Utils;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.Scalar;

import android.animation.ObjectAnimator;
import android.annotation.SuppressLint;
import android.app.ActionBar;
import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.hardware.Camera;
import android.hardware.Camera.PictureCallback;
import android.media.MediaScannerConnection;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.provider.MediaStore;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;
import android.view.animation.AnimationUtils;
import android.view.animation.DecelerateInterpolator;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ViewAnimator;

/**
 * The main Activity handling all the actions connected to the user interaction
 * The user loads a new picture either from file or from snapshot and adds the
 * requested scribbles Implements ColorDialogListener to get updates from the
 * scribble chooser dialog TODO: separate the object from the background and
 * allow putting it on a new background
 * 
 * @author elad
 * 
 */
public class MainActivity extends Activity implements
		ColorDialog.ColorDialogListener {
	//public final String PREF_NAME = "eladrich_cutout_pref";
	public final String PREF_USE_FIGTREE = "cutout_use_figtree";
	public final String PREF_USE_LEVELSET = "cutout_use_levelset";
	private static final String TAG = "SCRIBBLER_TAG";
	public static final int MEDIA_TYPE_IMAGE = 1;

	/**
	 * DoKernelEstimation - Find the object using Kernel Estimation
	 * 
	 * @param addrImg
	 *            - 8UC4 Matrix - The address of the original Image.
	 * @param addrFrontSources
	 *            /addrBackSources - 32FC2 Matrix - The address of 2Channel the
	 *            Front/Back Scribbles Matrix * Translated to a vector of
	 *            vectors using matToVectors. * * Each row contains one vector,
	 *            where the coordinates are saved in the 1th, 2th channels * *
	 *            The end of scribbles that are shorter than the number of cols
	 *            is denoted by (-1,-1) coordinate *
	 * @param addrRes
	 *            - 8UC3 Matrix - The address of the result matrix, where the
	 *            object is outlined.
	 * @param mode
	 *            - The scribbling mode, as defined by KernelEstimation::Mode
	 * @param addrFrontPDF
	 *            /addrBackPDF - 32FC1 Matrix - The address of the PDF matching
	 *            the front/back sources, saved between calls
	 * @param addrObjectMask
	 *            - 8UC1 Matrix - The resulting mask. * mask == 255 where object
	 *            is located *
	 */
	public native void DoKernelEstimation(long addrMat, long addrFrontSources,
			long addrBackSources, long addrRes, int mode, long addrFrontPDF,
			long addrBackPDF, long addrObjectMask, boolean useFIGTree, boolean useLevelset);

	/**
	 * DoAlphaFromMask - Generate image with alpha channel, according to the
	 * mask.
	 * 
	 * @param addrImg
	 *            - 8UC4 Matrix - The address of the original Image.
	 * @param addrMask
	 *            - 8UC1 Matrix - The address of the object's mask.
	 * @param addrRes
	 *            - 8UC4 Matrix - The address where the result will be saved,
	 *            with the new alpha channel.
	 */
	public native void DoAlphaFromMask(long addrImg, long addrMask,
			long addrRes, long addrBound, boolean useFIGTree);

	/**
	 * DoBlurFromMask - Blur the background/foreground of the image.
	 * 
	 * @param addrImg
	 *            - 8UC4 Matrix - The address of the original Image.
	 * @param addrMask
	 *            - 8UC1 Matrix - The address of the object's mask.
	 * @param addrRes
	 *            - 8UC4 Matrix - The address where the result will be saved,
	 *            after being blurred
	 * @param radius
	 *            - The blurring raduis * positive value means we are are
	 *            blurring the object * * negative value means we are blurring
	 *            the background *
	 */
	public native void DoBlurFromMask(long addrImg, long addrMask,
			long addrRes, int radius);

	/**
	 * DoGrayFromMask - Gray out the background/foreground of the image.
	 * 
	 * @param addrImg
	 *            - 8UC4 Matrix - The address of the original Image.
	 * @param addrMask
	 *            - 8UC1 Matrix - The address of the object's mask.
	 * @param addrRes
	 *            - 8UC4 Matrix - The address where the result will be saved,
	 *            after being blurred
	 * @param intensity
	 *            - The graying intensity * positive value means we are are
	 *            blurring the object * * negative value means we are blurring
	 *            the background *
	 */
	public native void DoGrayFromMask(long addrImg, long addrMask,
			long addrRes, float intensity);

	/** Some constants */
	static int REQUEST_IMAGE_CAPTURE = 1; // unique id for image capture result
	static int REQUEST_IMAGE_GALLERY = 2; // unique id for image import result
	static int REQUEST_IMAGE_GALLERY_CUTOUT = 3; // unique id for cutout import
													// result

	static int ANIMATOR_CHILD_SCRIBBLE = 0;
	static int ANIMATOR_CHILD_EFFECTS = 1;
	static int ANIMATOR_CHILD_CAMERA = 2;
	static int ANIMATOR_CHILD_BLUR = 3;
	static int ANIMATOR_CHILD_GRAY = 4;

	/** Saving */
	static final String STATE_PHOTO_FILE = "photoFile";
	static final String STATE_APP_MODE = "appMode";

	/** UI related fields */

	private DrawableImageView divDraw; // An image view that can be scribbled
										// upon
	private ImageView ivBack; // An imageView containing the image itself
	private ImageView ivWelcome;
	private ImageView ivMoveViewBack;
	// Note: the DrawableImageView can contain the picture itself
	// In the current implementation it covers the Back ImageView
	private ImageButton ibScribbleButton, ibZoomButton;
	// ibOKButton,
	private ImageButton ibCameraControlCapture, ibCameraControlUndo;

	private TextView tvDescription;
	private MenuItem miDiscard, miGallery, miSave, miCutout, miOK, miAbout, miSettings, miTutorial;
	private SeekBar sbblurBar, sbgrayBar;
	private Bitmap bImg, bContourImg, bBackground, effectImg, bCamera;
	private LinearLayout lLayoutInstructions; // The instructions Layout
	private LinearLayout lLayoutcameraControl, lLayoutBlurControl; // The camera
																	// controls
																	// layout
	private LinearLayout lLayoutEffects;
	private LinearLayout lLayoutWelcome, lLayoutWork;
	private LinearLayout floatingLayout;
	private RelativeLayout rLayoutDetails;
	private RelativeLayout rLayoutMoveView;
	private FrameLayout fLayoutscribble;
	private FrameLayout fLayoutCamera;

	private ZoomView zoomableLayout;
	private PhotoDragView moveView;

	private ViewAnimator bottomBar;
	// TODO: revert from this stupid naming method!
	private ActionBar appBar;

	/** Files related fields */
	private File photoFile;

	private Animation hoverAnim, inAnim, outAnim, rightAnim, leftAnim;
	private ObjectAnimator hideAnim;

	/** Scribbles data structure */
	private ArrayList<ArrayList<float[]>> frontScribbles; // the scribbles for
															// foreground
	private ArrayList<ArrayList<float[]>> backScribbles; // the scribbles for
															// background
	private ArrayList<ArrayList<float[]>> currentScribbles; // Just a reference

	private Mat objectMask, frontPDF, backPDF;

	/** enum that describes the current mode of scribbling */
	private enum Mode {
		FRONT, BACK, NONE
	}

	private enum AppMode {
		WELCOME, SCRIBBLE, EFFECT_BLUR, EFFECT_GRAY, EFFECT_STICKER, CHOOSE_EFFECT
	}

	private AppMode appMode = AppMode.WELCOME;
	private Mode currentMode = Mode.NONE; // We start without scribbles

	private Camera mCamera;
	private CameraPreview mPreview;
	private File pictureFile;
	private boolean isReverseAnim = true;

	private static boolean openCVLoaded = false;
	// private boolean moveViewMode = false;
	private boolean zoomEnabled = false;

	private boolean fingerDown = false;
	private boolean outOfImageFingerDown = false;
	private int blurRadius = 60;
	private int grayRadius = 20;

	private BaseLoaderCallback mLoaderCallback = new BaseLoaderCallback(this) {
		@Override
		public void onManagerConnected(int status) {
			switch (status) {
			case LoaderCallbackInterface.SUCCESS: {
				System.loadLibrary("figtree");
				System.loadLibrary("scribble_module");
				Log.i(TAG, "OpenCV loaded successfully");
				/*
				 * Toast.makeText(getApplicationContext(), "Loaded openCV",
				 * Toast.LENGTH_SHORT).show();
				 */
			}
				break;
			default: {
				Toast.makeText(getApplicationContext(), "Cannot Load openCV",
						Toast.LENGTH_SHORT).show();
				super.onManagerConnected(status);
			}
				break;
			}
		}
	};

	static {
		// A block that is executed when the class is created, even if there
		// aren't instances yet.
		if (!OpenCVLoader.initDebug()) {
			// Handle initialization error
			Log.i(TAG, "Error :(");
			openCVLoaded = false;

		} else {
			System.loadLibrary("figtree");
			System.loadLibrary("scribble_module");
			Log.i(TAG, "OpenCV loaded successfully");
			openCVLoaded = true;
		}
	}

	/* On Event Methods */
	/**
	 * creates the new activity get references for UI fields and set some
	 * listeners
	 * 
	 * @param savedInstanceState
	 */
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		Log.i(TAG, "Scribbler Creaing");
		setContentView(R.layout.activity_main);
		// Reference some of our views
		appBar = getActionBar();
		lLayoutWelcome = (LinearLayout) findViewById(R.id.welcome_layout);
		lLayoutWork = (LinearLayout) findViewById(R.id.work_layout);
		divDraw = (DrawableImageView) findViewById(R.id.image_view_draw);
		divDraw.setOnTouchListener(drawableImageOnTouchListener);
		ivBack = (ImageView) findViewById(R.id.image_view_back);
		ibScribbleButton = (ImageButton) findViewById(R.id.scribble_button);
		ibScribbleButton.setOnClickListener(scribbleButtonOnClickListener);
		floatingLayout = (LinearLayout) findViewById(R.id.floating_layout);
		/*
		 * ibOKButton = (ImageButton) findViewById(R.id.ok_button);
		 * ibOKButton.setOnClickListener(okButtonOnClickListener);
		 */
		lLayoutInstructions = (LinearLayout) findViewById(R.id.instrcutions_layout);
		fLayoutscribble = (FrameLayout) findViewById(R.id.scribble_layout);
		moveView = (PhotoDragView) findViewById(R.id.image_view_move);
		rLayoutMoveView = (RelativeLayout) findViewById(R.id.move_view_layout);
		fLayoutCamera = (FrameLayout) findViewById(R.id.camera_preview);
		tvDescription = (TextView) findViewById(R.id.scibble_message);
		ibZoomButton = (ImageButton) findViewById(R.id.scribble_zoom);
		ibZoomButton.setOnClickListener(zoomButtonOnClickListener);
		zoomableLayout = (ZoomView) findViewById(R.id.scribble_zoom_view);
		zoomableLayout.setZooming(zoomEnabled);
		ivWelcome = (ImageView) findViewById(R.id.welcome_android);
		lLayoutEffects = (LinearLayout) findViewById(R.id.effects_layout);
		rLayoutDetails = (RelativeLayout) findViewById(R.id.details_layout);
		lLayoutcameraControl = (LinearLayout) findViewById(R.id.camera_layout);
		ibCameraControlCapture = (ImageButton) findViewById(R.id.camera_capture);
		ibCameraControlCapture.setOnClickListener(cameraCaptureOnClickListener);
		ibCameraControlUndo = (ImageButton) findViewById(R.id.camera_undo);
		ibCameraControlUndo.setOnClickListener(cameraUndoOnClickListener);
		lLayoutBlurControl = (LinearLayout) findViewById(R.id.blur_layout);
		sbblurBar = (SeekBar) findViewById(R.id.blur_bar);
		sbblurBar.setOnSeekBarChangeListener(blurBarChangeListener);
		sbblurBar.setProgress(blurRadius);
		sbblurBar.setMax(2 * blurRadius);
		sbgrayBar = (SeekBar) findViewById(R.id.gray_bar);
		sbgrayBar.setOnSeekBarChangeListener(grayBarChangeListener);
		sbgrayBar.setProgress(grayRadius);
		sbgrayBar.setMax(2 * grayRadius);
		ivMoveViewBack = (ImageView) findViewById(R.id.move_view_back);
		bottomBar = (ViewAnimator) findViewById(R.id.bottom_bar);
		inAnim = AnimationUtils.loadAnimation(this, R.anim.slide_in);
		outAnim = AnimationUtils.loadAnimation(this, R.anim.slide_out);
		rightAnim = AnimationUtils.loadAnimation(this, R.anim.slide_right);
		leftAnim = AnimationUtils.loadAnimation(this, R.anim.slide_left);
		// inAnim.setAnimationListener(inAnimListener);
		// outAnim.setAnimationListener(outAnimListener);
		bottomBar.setInAnimation(inAnim);
		bottomBar.setOutAnimation(outAnim);
		// rightAnim.setAnimationListener(rightAnimListener);
		updateVisibilty();
	}

	@Override
	public void onSaveInstanceState(Bundle savedInstanceState) {
		Log.i(TAG, "Saving Data");
		savedInstanceState.putInt(STATE_APP_MODE, appMode.ordinal());
		savedInstanceState.putSerializable(STATE_PHOTO_FILE, photoFile);

		// Always call the superclass so it can save the view hierarchy state
		super.onSaveInstanceState(savedInstanceState);
	}

	public void onRestoreInstanceState(Bundle savedInstanceState) {
		Log.i(TAG, "Restoring");
		// Always call the superclass so it can restore the view hierarchy
		super.onRestoreInstanceState(savedInstanceState);

		// Restore state members from saved instance
		appMode = AppMode.values()[savedInstanceState.getInt(STATE_APP_MODE)];
		photoFile = (File) savedInstanceState.getSerializable(STATE_PHOTO_FILE);
	}

	@Override
	protected void onStart() {
		super.onStart();
		Log.i(TAG, "Scribbler Starting");
		if (!openCVLoaded) {
			/*
			 * Log.i(TAG, "Trying to load OpenCV library"); if
			 * (!OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_2_4_9, this,
			 * mLoaderCallback)) { Log.e(TAG,
			 * "Cannot connect to OpenCV Manager"); } else { Log.i(TAG,
			 * "opencv successfull"); openCVLoaded = true; }
			 */
			Toast.makeText(getApplicationContext(),
					"Cannot Load openCV, please report as bug",
					Toast.LENGTH_SHORT).show();
			this.finish();
		}
	}

	@Override
	protected void onResume() {
		super.onResume();
		Log.i(TAG, "Scribbler Resuming");
	}

	@Override
	protected void onPause() {
		super.onPause();
		Log.i(TAG, "Scribbler Pausing");
	}

	@Override
	protected void onStop() {
		super.onStop();
		Log.i(TAG, "Scribbler Stopping");
	}

	@Override
	protected void onDestroy() {
		Log.i(TAG, "Scribbler Destroying");
		if (mCamera != null) {
			mCamera.release();
		}
		if (bImg != null) {
			bImg.recycle();
		}
		super.onDestroy();
	}

	// 2.0 and above
	@Override
	public void onBackPressed() {
		// We want to go back to
		switch (appMode) {
		case EFFECT_BLUR:
		case EFFECT_GRAY:
			ivBack.setImageBitmap(bContourImg);
			if (effectImg != null) {
				effectImg.recycle();
				effectImg = null;
			}
			appMode = AppMode.CHOOSE_EFFECT;
			updateVisibilty();
			break;
		case EFFECT_STICKER:
			moveView.clearDragView();
			appMode = AppMode.CHOOSE_EFFECT;
			updateVisibilty();
			break;
		case CHOOSE_EFFECT:
			appMode = AppMode.SCRIBBLE;
			updateVisibilty();
			break;
		case SCRIBBLE:
			appMode = AppMode.WELCOME;
			photoFile = null;
			this.recreate();
			break;
		default:
			super.onBackPressed();
			break;
		}
	}

	/**
	 * Inflate the menu; this adds items to the action bar if it is present. Get
	 * a reference to the discardItem menu item.
	 * 
	 * @param menu
	 * @return
	 */
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.main, menu);
		miOK = menu.getItem(0);
		miDiscard = menu.getItem(1);
		miCutout = menu.getItem(2);
		miGallery = menu.getItem(3);
		miSave = menu.getItem(4);
		miAbout = menu.getItem(6);
		miSettings = menu.getItem(7);
		miTutorial = menu.getItem(8);
		updateVisibilty();
		return true;
	}

	/**
	 * apply the requested action for menu click
	 */
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle item selection
		switch (item.getItemId()) {
		case R.id.action_ok:
			// ibZoomButton.setVisibility(ImageButton.GONE);
			// miOK.setVisible(false);
			// divDraw.setVisibility(View.GONE);
			zoomableLayout.resetZoom();
			// rLayoutDetails.startAnimation(outAnim);
			floatingLayout.startAnimation(rightAnim);
			appMode = AppMode.CHOOSE_EFFECT;
			updateVisibilty();
			return true;
		case R.id.action_from_camera:
			dispatchTakePictureIntent();
			return true;
		case R.id.action_from_gallery:
			dispatchGetFromGalleryIntent(false);
			return true;
		case R.id.action_discard:
			initScribbles(); // remove all scribbles
			ivBack.setImageBitmap(bImg);
			divDraw.setMagnifyBitmap(bImg);
			if (appMode != AppMode.SCRIBBLE) {
				appMode = AppMode.SCRIBBLE;
				updateVisibilty();
			}
			return true;
		case R.id.action_save:
			if (appMode == AppMode.EFFECT_STICKER) {
				rLayoutMoveView.setDrawingCacheEnabled(false);
				rLayoutMoveView.setDrawingCacheEnabled(true);
				Bitmap viewMap = rLayoutMoveView.getDrawingCache();
				new SaveResultTask().execute(viewMap, "CutOutResults");
			} else if (appMode == AppMode.EFFECT_BLUR
					|| appMode == AppMode.EFFECT_GRAY) {
				new SaveResultTask().execute(effectImg, "CutOutResults");
			}
			return true;
		case R.id.action_add_cutout:
			dispatchGetFromGalleryIntent(true);
			return true;
		case R.id.action_about:
			Intent about = new Intent(this, AboutActivity.class);
			startActivity(about);
			return true;
		case R.id.action_settings:
			Intent settings = new Intent(this, SettingsActivity.class);
			startActivity(settings);
			return true;
		case R.id.action_help:
			Intent tutorial = new Intent(this, TutorialActivity.class);
			startActivity(tutorial);
			return true;
		case android.R.id.home:
			onBackPressed();
			return true;
		default:
			return super.onOptionsItemSelected(item);
		}
	}

	/**
	 * Create a scribble according to the chosen option
	 * 
	 * @param frontMode
	 */
	@Override
	public void onDialogClick(boolean frontMode) {
		if (zoomEnabled) {
			zoomButtonOnClickListener.onClick(ibZoomButton);
		}
		createScribble(frontMode);
	}

	/**
	 * reacts to either the image capture result, or image gallery result scales
	 * the image down using setPic to enable fast redrawing For camera the
	 * output is already saved in our local directory For gallery we will first
	 * copy a compressed copy
	 * 
	 * @param requestCode
	 * @param resultCode
	 * @param data
	 */

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		Log.i(TAG, "Got Result!");
		if (requestCode == REQUEST_IMAGE_CAPTURE && resultCode == RESULT_OK) {
			updateVisibilty();
			bImg = ScribblerUtils.setPic(photoFile.getAbsolutePath(),
					ivBack.getWidth(), ivBack.getHeight());
			ivBack.setImageBitmap(bImg);
			divDraw.setMagnifyBitmap(bImg);
			ibZoomButton.setVisibility(ImageButton.VISIBLE);
			initScribbles();
			return;
		}
		if ((requestCode == REQUEST_IMAGE_GALLERY || requestCode == REQUEST_IMAGE_GALLERY_CUTOUT)
				&& resultCode == RESULT_OK && data != null) {
			Uri imageUri = data.getData();
			// appMode = AppMode.SCRIBBLE;
			updateVisibilty();

			if (imageUri != null) {

				/** Initialize the new file */
				File localPhotoFile = null;
				try {
					localPhotoFile = ScribblerUtils
							.createImageFile(getExternalFilesDir(null));
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}

				try {
					/*
					 * bitmap = MediaStore.Images.Media.getBitmap(
					 * this.getContentResolver(), imageUri);
					 */
					/*
					 * Log.i(TAG, "Launcing Save Image!"); new
					 * SaveImageTask().execute(bitmap, photoFile, requestCode,
					 * appMode); // save // locally
					 */if (requestCode == REQUEST_IMAGE_GALLERY_CUTOUT) {
						Bitmap bitmap = ScribblerUtils.decodeUriIntoPath(this,
								imageUri, localPhotoFile.getAbsolutePath(),
								moveView.getWidth(), moveView.getHeight());
						BitmapDrawable bd = new BitmapDrawable(getResources(),
								bitmap);
						moveView.loadAnImage(getApplicationContext(), bd);
					} else if (appMode == AppMode.EFFECT_STICKER) {
						Bitmap bitmap = ScribblerUtils.decodeUriIntoPath(this,
								imageUri, localPhotoFile.getAbsolutePath(),
								ivMoveViewBack.getWidth(),
								ivMoveViewBack.getHeight());
						ivMoveViewBack.setImageBitmap(bitmap);

						ibCameraControlCapture.setVisibility(ImageButton.GONE);
						ibCameraControlUndo.setVisibility(ImageButton.VISIBLE);
						miSave.setEnabled(true);
						fLayoutCamera.setVisibility(FrameLayout.GONE);
						if (mCamera != null) {
							mCamera.release();
							mCamera = null;
							mPreview = null;
							fLayoutCamera.removeAllViews();
						}

					} else {
						bImg = ScribblerUtils.decodeUriIntoPath(this, imageUri,
								localPhotoFile.getAbsolutePath(),
								ivBack.getWidth(), ivBack.getHeight());
						ivBack.setImageBitmap(bImg);
						divDraw.setMagnifyBitmap(bImg);
						ibZoomButton.setVisibility(ImageButton.VISIBLE);
						initScribbles();
					}
				} catch (FileNotFoundException e) {
					// TODO Auto-generated catch block
					Log.d(TAG, e.getMessage());
				} catch (IOException e) {
					// TODO Auto-generated catch block
					Log.d(TAG, e.getMessage());
				}
			}
		}

	}

	private void updateVisibilty() {
		switch (appMode) {
		case WELCOME:
			appBar.setHomeButtonEnabled(false);
			appBar.setDisplayHomeAsUpEnabled(false);
			if (miDiscard != null && miGallery != null && miSave != null) {
				miDiscard.setVisible(false);
				miGallery.setVisible(false);
				miSave.setVisible(false);
				miOK.setVisible(false);
				miCutout.setVisible(false);
				miAbout.setVisible(true);
				miSettings.setVisible(true);
				miTutorial.setVisible(true);
				miTutorial.setShowAsAction(MenuItem.SHOW_AS_ACTION_NEVER);
			}
			lLayoutWelcome.setVisibility(View.VISIBLE);
			lLayoutWork.setVisibility(View.INVISIBLE);
			divDraw.setVisibility(View.VISIBLE);
			ivBack.setVisibility(View.VISIBLE);
			ibScribbleButton.setVisibility(View.INVISIBLE);
			// ibOKButton.setVisibility(View.INVISIBLE);
			// instLayout.setVisibility(View.VISIBLE);
			fLayoutscribble.setVisibility(View.VISIBLE);
			// moveView.setVisibility(View.GONE);
			rLayoutMoveView.setVisibility(View.GONE);
			// cameraLayout.setVisibility(View.GONE);
			ibZoomButton.setVisibility(View.GONE);
			zoomableLayout.setVisibility(View.VISIBLE);
			floatingLayout.setVisibility(View.VISIBLE);
			bottomBar.setVisibility(View.GONE);
			// lLayoutBlurControl.setVisibility(View.GONE);
			// lLayoutcameraControl.setVisibility(LinearLayout.GONE);
			// lLayoutEffects.setVisibility(View.GONE);
			// rLayoutDetails.setVisibility(View.VISIBLE);
			break;
		case SCRIBBLE:
			appBar.setHomeButtonEnabled(true);
			appBar.setDisplayHomeAsUpEnabled(true);
			if (miDiscard != null && miGallery != null && miSave != null) {
				miDiscard.setVisible(true);
				miGallery.setVisible(false);
				miSave.setVisible(false);
				miOK.setVisible(true);
				miCutout.setVisible(false);
				miAbout.setVisible(false);
				miSettings.setVisible(false);
				miTutorial.setVisible(true);
				miTutorial.setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS);
			}
			lLayoutWelcome.setVisibility(View.GONE);
			lLayoutWork.setVisibility(View.VISIBLE);
			fLayoutscribble.setVisibility(View.VISIBLE);
			divDraw.setVisibility(View.VISIBLE);
			ivBack.setVisibility(View.VISIBLE);
			// ibOKButton.setVisibility(View.INVISIBLE);
			ibZoomButton.setVisibility(View.VISIBLE);
			ibScribbleButton.setVisibility(ImageButton.VISIBLE);

			rLayoutMoveView.setVisibility(View.GONE);
			fLayoutCamera.setVisibility(View.GONE);
			floatingLayout.setVisibility(View.VISIBLE);
			floatingLayout.startAnimation(leftAnim);
			bottomBar.setVisibility(View.VISIBLE);
			bottomBar.setDisplayedChild(ANIMATOR_CHILD_SCRIBBLE);
			// lLayoutBlurControl.setVisibility(View.GONE);
			// lLayoutcameraControl.setVisibility(LinearLayout.GONE);
			// lLayoutEffects.setVisibility(View.GONE);
			// rLayoutDetails.setVisibility(View.VISIBLE);
			hideAnim = ObjectAnimator.ofFloat(floatingLayout, "translationY",
					floatingLayout.getHeight() / 2);
			hideAnim.setDuration(500);
			hideAnim.setInterpolator(new DecelerateInterpolator(2f));
			break;
		case CHOOSE_EFFECT:
			appBar.setHomeButtonEnabled(true);
			appBar.setDisplayHomeAsUpEnabled(true);
			if (miDiscard != null && miGallery != null && miSave != null) {
				miDiscard.setVisible(true);
				miGallery.setVisible(false);
				miSave.setVisible(false);
				miOK.setVisible(false);
				miCutout.setVisible(false);
				miAbout.setVisible(false);
				miSettings.setVisible(false);
				miTutorial.setVisible(false);
			}
			lLayoutWelcome.setVisibility(View.GONE);
			lLayoutWork.setVisibility(View.VISIBLE);
			fLayoutscribble.setVisibility(View.VISIBLE);
			divDraw.setVisibility(View.GONE);
			ivBack.setVisibility(View.VISIBLE);
			// ibOKButton.setVisibility(View.INVISIBLE);
			ibZoomButton.setVisibility(View.VISIBLE);
			ibScribbleButton.setVisibility(ImageButton.VISIBLE);

			rLayoutMoveView.setVisibility(View.GONE);
			fLayoutCamera.setVisibility(View.GONE);
			floatingLayout.setVisibility(View.GONE);
			bottomBar.setVisibility(View.VISIBLE);
			bottomBar.setDisplayedChild(ANIMATOR_CHILD_EFFECTS);
			// lLayoutBlurControl.setVisibility(View.GONE);
			// lLayoutcameraControl.setVisibility(LinearLayout.GONE);
			// lLayoutEffects.setVisibility(View.VISIBLE);
			// rLayoutDetails.setVisibility(View.GONE);
			break;
		case EFFECT_STICKER:
			miDiscard.setVisible(false);
			miGallery.setVisible(true);
			miSave.setVisible(true);
			miOK.setVisible(false);
			miCutout.setVisible(true);
			// miSave.setEnabled(false);
			miAbout.setVisible(false);
			miSettings.setVisible(false);
			miTutorial.setVisible(false);
			fLayoutscribble.setVisibility(View.GONE);
			rLayoutMoveView.setVisibility(View.VISIBLE);
			fLayoutCamera.setVisibility(View.VISIBLE);
			ibZoomButton.setVisibility(View.GONE);
			floatingLayout.setVisibility(View.GONE);
			ibScribbleButton.setVisibility(ImageButton.GONE);
			//tvDescription.setText(getString(R.string.action_add_background));

			bottomBar.setVisibility(View.VISIBLE);
			bottomBar.setDisplayedChild(ANIMATOR_CHILD_CAMERA);
			// lLayoutBlurControl.setVisibility(View.GONE);
			// lLayoutcameraControl.setVisibility(LinearLayout.VISIBLE);
			// lLayoutEffects.setVisibility(View.GONE);
			// rLayoutDetails.setVisibility(View.GONE);

			break;
		case EFFECT_BLUR:
			miDiscard.setVisible(false);
			miGallery.setVisible(false);
			miSave.setVisible(true);
			miOK.setVisible(false);
			miCutout.setVisible(false);
			miSave.setEnabled(true);
			miAbout.setVisible(false);
			miSettings.setVisible(false);
			miTutorial.setVisible(false);
			fLayoutscribble.setVisibility(View.VISIBLE);
			divDraw.setVisibility(View.GONE);
			ivBack.setVisibility(View.VISIBLE);
			rLayoutMoveView.setVisibility(View.GONE);
			fLayoutCamera.setVisibility(View.GONE);
			ibZoomButton.setVisibility(View.GONE);
			floatingLayout.setVisibility(View.GONE);
			sbblurBar.setProgress(blurRadius);

			bottomBar.setVisibility(View.VISIBLE);
			bottomBar.setDisplayedChild(ANIMATOR_CHILD_BLUR);
			// lLayoutBlurControl.setVisibility(View.VISIBLE);
			// lLayoutcameraControl.setVisibility(LinearLayout.GONE);
			// lLayoutEffects.setVisibility(View.GONE);
			// rLayoutDetails.setVisibility(View.GONE);
			break;
		case EFFECT_GRAY:
			miDiscard.setVisible(false);
			miGallery.setVisible(false);
			miSave.setVisible(true);
			miOK.setVisible(false);
			miCutout.setVisible(false);
			miSave.setEnabled(true);
			miAbout.setVisible(false);
			miSettings.setVisible(false);
			miTutorial.setVisible(false);
			fLayoutscribble.setVisibility(View.VISIBLE);
			divDraw.setVisibility(View.GONE);
			ivBack.setVisibility(View.VISIBLE);
			rLayoutMoveView.setVisibility(View.GONE);
			fLayoutCamera.setVisibility(View.GONE);
			ibZoomButton.setVisibility(View.GONE);
			floatingLayout.setVisibility(View.GONE);
			sbgrayBar.setProgress(grayRadius);

			bottomBar.setVisibility(View.VISIBLE);
			bottomBar.setDisplayedChild(ANIMATOR_CHILD_GRAY);
			// lLayoutBlurControl.setVisibility(View.VISIBLE);
			// lLayoutcameraControl.setVisibility(LinearLayout.GONE);
			// lLayoutEffects.setVisibility(View.GONE);
			// rLayoutDetails.setVisibility(View.GONE);
			break;
		default:
			break;
		}
	}

	/* Images related */

	/**
	 * Get image from gallery and initialize a file for later save
	 */
	private void dispatchGetFromGalleryIntent(boolean cutout) {
		Intent getFromGalleryIntent = new Intent(Intent.ACTION_GET_CONTENT);
		getFromGalleryIntent.setType("image/*");
		/*
		 * if (cutout) {
		 * 
		 * Uri uri = Uri.parse(Environment.getExternalStoragePublicDirectory(
		 * Environment.DIRECTORY_PICTURES).getPath() + "/CutOuts/");
		 * getFromGalleryIntent = new Intent(Intent.ACTION_PICK, uri);
		 * getFromGalleryIntent.setType("image/*"); //
		 * getFromGalleryIntent.setDataAndType(uri, "image/*");
		 * getFromGalleryIntent.setType("image/png"); }
		 */
		if (getFromGalleryIntent.resolveActivity(getPackageManager()) != null) {

			if (cutout) {
				startActivityForResult(getFromGalleryIntent,
						REQUEST_IMAGE_GALLERY_CUTOUT);
			} else {
				startActivityForResult(getFromGalleryIntent,
						REQUEST_IMAGE_GALLERY);
			}

		}
	}

	/**
	 * Get image from gallery and initialize a file to save it into
	 */
	private void dispatchTakePictureIntent() {
		Intent takePictureIntent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
		if (takePictureIntent.resolveActivity(getPackageManager()) != null) {

			/** Initialize the new file */
			photoFile = null;
			try {
				photoFile = ScribblerUtils
						.createImageFile(getExternalFilesDir(null));
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

			// Start intent if the file was successfully created
			if (photoFile != null) {
				takePictureIntent.putExtra(MediaStore.EXTRA_OUTPUT,
						Uri.fromFile(photoFile)); // Write output to photoFile
				startActivityForResult(takePictureIntent, REQUEST_IMAGE_CAPTURE);
			}
		}
	}

	/* Utility Functions */

	/**
	 * initializes a new scribble from the requested kind
	 * 
	 * @param isFrontScribble
	 */
	private void createScribble(boolean isFrontScribble) {
		ibScribbleButton.setClickable(false);
		if (isFrontScribble) {
			setBackDrawable(
					ibScribbleButton,
					getResources().getDrawable(
							R.drawable.ic_foreground_scribble));
			tvDescription.setText(R.string.action_scribble_front);
			currentMode = Mode.FRONT;
		} else {
			setBackDrawable(
					ibScribbleButton,
					getResources().getDrawable(
							R.drawable.ic_background_scribble));
			tvDescription.setText(R.string.action_scribble_back);
			currentMode = Mode.BACK;
		}
	}

	/**
	 * Initialize the scribbles handling
	 */
	private void initScribbles() {
		frontScribbles = new ArrayList<ArrayList<float[]>>();
		backScribbles = new ArrayList<ArrayList<float[]>>();
		currentScribbles = null;
		currentMode = Mode.FRONT;

		ibScribbleButton.setVisibility(ImageButton.VISIBLE);
		ibScribbleButton.setClickable(false);
		setBackDrawable(ibScribbleButton,
				getResources().getDrawable(R.drawable.ic_foreground_scribble));

		tvDescription.setText(R.string.action_scribble_front);

		divDraw.flushScribbles();
		divDraw.invalidate();
	}

	/* Listeners */

	/**
	 * Click Listener for the scribble button
	 */
	@Deprecated
	OnClickListener okButtonOnClickListener = new OnClickListener() {

		@Override
		public void onClick(View v) {

			// new RunAlphaTask().execute();

			// iWelcome.startAnimation(hoverAnim);
			ibZoomButton.setVisibility(ImageButton.GONE);
			zoomableLayout.resetZoom();
			rLayoutDetails.startAnimation(outAnim);
			floatingLayout.startAnimation(outAnim);

		}
	};

	AnimationListener outAnimListener = new AnimationListener() {

		@Override
		public void onAnimationStart(Animation animation) {
			// TODO Auto-generated method stub

		}

		@Override
		public void onAnimationRepeat(Animation animation) {
			// TODO Auto-generated method stub

		}

		@Override
		public void onAnimationEnd(Animation animation) {
			// TODO Auto-generated method stub
			rLayoutDetails.setVisibility(LinearLayout.GONE);
			floatingLayout.setVisibility(LinearLayout.GONE);
			// ibZoomButton.setVisibility(ImageButton.GONE);
			// ibZoomButton.setVisibility(ImageButton.GONE);
			lLayoutEffects.startAnimation(inAnim);
			lLayoutEffects.setVisibility(LinearLayout.VISIBLE);
		}
	};

	AnimationListener inAnimListener = new AnimationListener() {

		@Override
		public void onAnimationStart(Animation animation) {
			// TODO Auto-generated method stub

		}

		@Override
		public void onAnimationRepeat(Animation animation) {
			// TODO Auto-generated method stub

		}

		@Override
		public void onAnimationEnd(Animation animation) {
			// We've finished moving to the choose effect view
			appMode = AppMode.CHOOSE_EFFECT;
			updateVisibilty();
		}
	};

	AnimationListener rightAnimListener = new AnimationListener() {

		@Override
		public void onAnimationStart(Animation animation) {
			// TODO Auto-generated method stub

		}

		@Override
		public void onAnimationRepeat(Animation animation) {
			// TODO Auto-generated method stub

		}

		@Override
		public void onAnimationEnd(Animation animation) {
			// TODO Auto-generated method stub
			floatingLayout.setVisibility(LinearLayout.GONE);
		}
	};

	public void onEffectClick(View v) {
		int id = v.getId();
		switch (id) {
		case R.id.effect_stick_camera:
			appMode = AppMode.EFFECT_STICKER;
			new RunAlphaTask().execute();
			break;
		case R.id.effect_blur:
			appMode = AppMode.EFFECT_BLUR;
			ivBack.setImageBitmap(bImg);
			updateVisibilty();
			// new RunBlurTask().execute(11);
			break;
		case R.id.effect_gray:
			appMode = AppMode.EFFECT_GRAY;
			ivBack.setImageBitmap(bImg);
			updateVisibilty();
			// new RunBlurTask().execute(11);
			break;
		default:
			break;
		}
	}

	public void onActionClick(View v) {
		int id = v.getId();
		switch (id) {
		case R.id.action_camera:
			appMode = AppMode.SCRIBBLE;
			dispatchTakePictureIntent();
			break;
		case R.id.action_gallery:
			appMode = AppMode.SCRIBBLE;
			dispatchGetFromGalleryIntent(false);
			break;
		default:
			break;
		}
	}

	/**
	 * Definition of the drawable Image touch listener
	 */
	OnTouchListener drawableImageOnTouchListener = new OnTouchListener() {

		// TODO: what the hell are you thinking?! split it into multiple
		// functions!
		@Override
		public boolean onTouch(View view, MotionEvent event) {

			if (currentMode == Mode.NONE) {
				return true;
			}
			/** Initialize UI references */
			// ImageView dView = (ImageView) view;
			/*
			 * TextView touchedXY = (TextView) findViewById(R.id.touched_xy);
			 * TextView invertedXY = (TextView) findViewById(R.id.inverted_xy);
			 * TextView imgSize = (TextView) findViewById(R.id.touched_size);
			 * TextView colorRGB = (TextView) findViewById(R.id.touched_color);
			 */

			/** Inside ZoomableRelative */

			float[] origXY = { event.getX(), event.getY() };
			Matrix invertZoom = new Matrix();
			zoomableLayout.getDrawMat().invert(invertZoom);
			invertZoom.mapPoints(origXY);
			float eventX = origXY[0];
			float eventY = origXY[1];

			/** Calculate event location */
			// float eventX = event.getX();
			// float eventY = event.getY();
			float[] eventXY = new float[] { eventX, eventY };
			float[] imageXY = new float[] { eventX, eventY };

			Matrix invertMatrix = new Matrix();
			ivBack.getImageMatrix().invert(invertMatrix);

			invertMatrix.mapPoints(imageXY); // Updates imageXY
			int x = Integer.valueOf((int) imageXY[0]);
			int y = Integer.valueOf((int) imageXY[1]);

			/** update scribble */

			Drawable imgDrawable = ivBack.getDrawable();
			Bitmap bitmap = ((BitmapDrawable) imgDrawable).getBitmap();

			if (x >= 0 && x <= bitmap.getWidth() - 1 && y >= 0
					&& y <= bitmap.getHeight() - 1) {

				/** Update UI debug data */
				/*
				 * touchedXY.setText("touched position: " +
				 * String.valueOf(eventXY[0]) + " / " +
				 * String.valueOf(eventXY[1]));
				 * invertedXY.setText("touched position: " +
				 * String.valueOf(imageXY[0]) + " / " +
				 * String.valueOf(imageXY[1]));
				 * 
				 * imgSize.setText("drawable size: " +
				 * String.valueOf(bitmap.getWidth()) + " / " +
				 * String.valueOf(bitmap.getHeight()));
				 * 
				 * int touchedRGB = bitmap.getPixel(x, y); Log.d("INFO",
				 * "Adding" + x + "," + y + "to: " + currentMode.name());
				 * colorRGB.setText("touched color: " + "#" +
				 * Integer.toHexString(touchedRGB));
				 * colorRGB.setTextColor(touchedRGB);
				 */

				if (outOfImageFingerDown) {
					updateScribble(MotionEvent.ACTION_DOWN, eventXY, imageXY);
					outOfImageFingerDown = false;
				} else {
					updateScribble(event.getAction(), eventXY, imageXY);
				}

			} else {
				if (fingerDown) {
					// The user got out of the picture
					updateScribble(MotionEvent.ACTION_UP, eventXY, imageXY);
					outOfImageFingerDown = true;
				}
				switch (event.getAction()) {
				case MotionEvent.ACTION_DOWN:
					outOfImageFingerDown = true;
					break;
				case MotionEvent.ACTION_UP:
					outOfImageFingerDown = false;
					break;
				}

			}

			return true;
		}

		/**
		 * update the drawable and data structure with the new point
		 * 
		 * @param action
		 * @param eventXY
		 * @param imageXY
		 */
		private void updateScribble(int action, float[] eventXY, float[] imageXY) {
			DrawableImageView sView = (DrawableImageView) findViewById(R.id.image_view_draw);
			if (eventXY[1] + 100 > floatingLayout.getY()) {
				if (isReverseAnim) {
					isReverseAnim = false;
					hideAnim.start();
				}
			}
			if (action == MotionEvent.ACTION_UP
					|| eventXY[1] + 150 < floatingLayout.getY()) {
				if (!isReverseAnim) {
					isReverseAnim = true;
					hideAnim.reverse();
				}
			}
			switch (action) {
			case MotionEvent.ACTION_DOWN:
				if (fingerDown) {
					break;
				}
				fingerDown = true;
				// Log.i(TAG, "Touch down");
				Boolean frontMode = false;
				switch (currentMode) {
				case BACK:
					currentScribbles = backScribbles;
					frontMode = true;
					break;
				case FRONT:
					currentScribbles = frontScribbles;
					frontMode = false;
					break;
				default: // we are not supposed to be in a different mode
					Log.d("LOGIC_ERROR", "what are we doing here?!");
					break;
				}

				currentScribbles.add(new ArrayList<float[]>());
				currentScribbles.get(currentScribbles.size() - 1).add(imageXY);

				sView.touch_start(eventXY[0], eventXY[1], frontMode);
				break;
			case MotionEvent.ACTION_MOVE:
				// Log.i(TAG, "Touch move");
				currentScribbles.get(currentScribbles.size() - 1).add(imageXY);
				sView.touch_move(eventXY[0], eventXY[1], imageXY[0], imageXY[1]);
				break;
			case MotionEvent.ACTION_UP:
				if (!fingerDown) {
					break;
				}
				fingerDown = false;
				// Log.i(TAG, "Touch up");
				if (backScribbles.size() == 0) { // No back scribbles yet
					currentMode = Mode.BACK;
					setBackDrawable(ibScribbleButton, getResources()
							.getDrawable(R.drawable.ic_background_scribble));
					tvDescription.setText(R.string.action_scribble_back);
					// iButton.setClickable(false);
					// new RunDijkstraTask().execute(iMap,
					// currentScribbles.get(0));
				} else { // Finished two scribbles
					int mode;
					if (backScribbles.size() == 1 && frontScribbles.size() == 1) {
						mode = 0;
						miOK.setVisible(true);
						/*
						 * ibOKButton.setVisibility(ImageButton.VISIBLE);
						 * ibOKButton.setClickable(true);
						 */
					} else {
						if (currentMode == Mode.FRONT) {
							mode = 1;
						} else {
							mode = 2;
						}
					}
					currentMode = Mode.NONE;
					setBackDrawable(ibScribbleButton, getResources()
							.getDrawable(R.drawable.add_scribble));
					ibScribbleButton.setClickable(true);
					tvDescription.setText(R.string.action_scribble_add);
					new RunKernelEstimationTask().execute(mode);
				}
				sView.touch_up();
				break;
			}
			sView.invalidate();
		}

	};

	/**
	 * Click Listener for the scribble button
	 */
	OnClickListener scribbleButtonOnClickListener = new OnClickListener() {

		@Override
		public void onClick(View v) {
			ColorDialog dialog = new ColorDialog();
			dialog.show(getFragmentManager(), "Scribble Chooser");
		}
	};

	OnClickListener cameraCaptureOnClickListener = new OnClickListener() {

		@Override
		public void onClick(View v) {
			// TODO Auto-generated method stub
			mCamera.takePicture(null, null, mPicture);
			ibCameraControlCapture.setVisibility(ImageButton.GONE);
			ibCameraControlUndo.setVisibility(ImageButton.VISIBLE);
			miSave.setEnabled(true);

		}
	};

	OnClickListener cameraUndoOnClickListener = new OnClickListener() {

		@Override
		public void onClick(View v) {
			// TODO Auto-generated method stub
			ibCameraControlCapture.setVisibility(ImageButton.VISIBLE);
			ibCameraControlUndo.setVisibility(ImageButton.GONE);
			miSave.setEnabled(false);
			fLayoutCamera.setVisibility(FrameLayout.VISIBLE);
			mCamera = ScribblerUtils.getCameraInstance();
			// mCamera.setDisplayOrientation(90);
			mPreview = new CameraPreview(getApplicationContext(), mCamera);
			fLayoutCamera.addView(mPreview);
			/*
			 * moveView.setBackgroundColor(getResources().getColor(
			 * R.color.transparent));
			 */
			ivMoveViewBack.setImageResource(getResources().getColor(
					R.color.transparent));
		}
	};

	OnClickListener zoomButtonOnClickListener = new OnClickListener() {
		private String origText;

		@Override
		public void onClick(View v) {
			zoomEnabled = !zoomEnabled;
			zoomableLayout.setZooming(zoomEnabled);
			if (zoomEnabled) {
				origText = (String) tvDescription.getText();
				tvDescription.setText(getResources().getString(
						R.string.action_zoom));
				setBackDrawable(ibZoomButton,
						getResources().getDrawable(R.drawable.zoom_on_button));
			} else {
				tvDescription.setText(origText);
				setBackDrawable(ibZoomButton,
						getResources().getDrawable(R.drawable.zoom_off_button));
			}
		}
	};

	OnSeekBarChangeListener blurBarChangeListener = new OnSeekBarChangeListener() {

		@Override
		public void onStopTrackingTouch(SeekBar seekBar) {
			// TODO Auto-generated method stub
			new RunBlurTask().execute(seekBar.getProgress() - blurRadius);
		}

		@Override
		public void onStartTrackingTouch(SeekBar seekBar) {
			// TODO Auto-generated method stub

		}

		@Override
		public void onProgressChanged(SeekBar seekBar, int progress,
				boolean fromUser) {
			// TODO Auto-generated method stub

		}
	};

	OnSeekBarChangeListener grayBarChangeListener = new OnSeekBarChangeListener() {

		@Override
		public void onStopTrackingTouch(SeekBar seekBar) {
			// TODO Auto-generated method stub
			float intensity = (seekBar.getProgress() - grayRadius)
					/ (float) (grayRadius);
			new RunGrayTask().execute(intensity);
		}

		@Override
		public void onStartTrackingTouch(SeekBar seekBar) {
			// TODO Auto-generated method stub

		}

		@Override
		public void onProgressChanged(SeekBar seekBar, int progress,
				boolean fromUser) {
			// TODO Auto-generated method stub

		}
	};

	@SuppressWarnings("deprecation")
	@SuppressLint("NewApi")
	private void setBackDrawable(View v, Drawable bd) {
		int sdk = android.os.Build.VERSION.SDK_INT;
		if (sdk < android.os.Build.VERSION_CODES.JELLY_BEAN) {
			v.setBackgroundDrawable(bd);
		} else {
			v.setBackground(bd);
		}
	}

	private PictureCallback mPicture = new PictureCallback() {

		@Override
		public void onPictureTaken(byte[] data, Camera camera) {
			// int orientation = Exif.getOrientation(data);
			boolean fast = false;
			if (fast) {
				Bitmap bmpSrc = BitmapFactory.decodeByteArray(data, 0,
						data.length);
				bCamera = Exif.rotate(bmpSrc, 90);
				// bCamera = bmpSrc;
				if (bCamera != bmpSrc) {
					bmpSrc.recycle();
					Log.d(TAG, "Succesfully rotated");
				}

				/*
				 * BitmapDrawable bd = new BitmapDrawable(getResources(),
				 * bmpRotated); setBackDrawable(moveView, bd);
				 */
			} else {

				try {
					pictureFile = ScribblerUtils
							.createImageFile(Environment
									.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES));
				} catch (IOException e1) {
					Log.d(TAG,
							"Error creating media file, check storage permission"
									+ e1.getMessage());
					return;
				}

				try {
					Log.d(TAG, "Saving image");
					FileOutputStream fos = new FileOutputStream(pictureFile);
					fos.write(data);
					fos.close();
					Log.d(TAG, "Image saved");
					MediaScannerConnection.scanFile(getApplicationContext(),
							new String[] { pictureFile.toString() },
							new String[] { "image/jpeg" }, null);
					Bitmap map = ScribblerUtils
							.setPic(pictureFile.getAbsolutePath(),
									fLayoutCamera.getWidth(),
									fLayoutCamera.getHeight());
					bCamera = Exif.rotate(map, 90);
					Log.d(TAG, "Reloading as bitmap");
					// BitmapDrawable bd = new BitmapDrawable(getResources(),
					// map);
					// moveView.setBackgroundDrawable(bd); // mCamera.release();
					// cameraLayout.setVisibility(FrameLayout.GONE);
				} catch (FileNotFoundException e) {
					Log.d(TAG, "File not found: " + e.getMessage());
				} catch (IOException e) {
					Log.d(TAG, "Error accessing file: " + e.getMessage());
				}

			}
			ivMoveViewBack.setImageBitmap(bCamera);
			/*
			 * Matrix mat = new Matrix(); float sx =
			 * (float)ivMoveViewBack.getWidth()/bCamera.getWidth(); float sy =
			 * (float)ivMoveViewBack.getHeight()/bCamera.getHeight(); sx = sx <
			 * 1 ? 1 : sx; sy = sy < 1 ? 1 : sy; mat.postScale(sx, sy);
			 * ivMoveViewBack.setImageMatrix(mat);
			 */
			fLayoutCamera.setVisibility(FrameLayout.GONE);
			mCamera.release();
			mCamera = null;
			mPreview = null;
			fLayoutCamera.removeAllViews();
		}
	};

	/* Async Tasks */

	/**
	 * AsyncTask for image saving
	 * 
	 * @author elad
	 * 
	 */
	@Deprecated
	private class SaveImageTask extends AsyncTask<Object, Integer, Boolean> {
		ProgressDialog Asycdialog = new ProgressDialog(MainActivity.this);

		// CustomLoading cDialog =
		// CustomLoading.newInstance(R.drawable.loading_element_scissors);
		private int requestCode;
		private AppMode aMode;

		@Override
		protected void onPreExecute() {
			// set message of the dialog
			Asycdialog.setMessage(getString(R.string.loadingtype));
			Asycdialog.setCancelable(false);
			// show dialog
			Asycdialog.show();
			// cDialog.setCancelable(false);

			// cDialog.show(getFragmentManager(),
			// getString(R.string.loadingtype));

			super.onPreExecute();
		}

		protected Boolean doInBackground(Object... objects) {
			Bitmap bitmap = (Bitmap) objects[0];
			File file = (File) objects[1];
			requestCode = (Integer) objects[2];
			aMode = (AppMode) objects[3];
			return (ScribblerUtils.savebitmap(bitmap, file) != null);

		}

		protected void onProgressUpdate(Integer... progress) {
		}

		protected void onPostExecute(Boolean result) {
			Asycdialog.dismiss();
			// cDialog.dismiss();
			if (result) {
				if (requestCode == REQUEST_IMAGE_GALLERY_CUTOUT) {
					Bitmap bitmap = ScribblerUtils.setPic(
							photoFile.getAbsolutePath(),
							lLayoutWork.getWidth(), lLayoutWork.getHeight());
					BitmapDrawable bd = new BitmapDrawable(getResources(),
							bitmap);
					moveView.loadAnImage(getApplicationContext(), bd);
				} else if (aMode == AppMode.EFFECT_STICKER) {
					Bitmap bitmap = ScribblerUtils.setPic(
							photoFile.getAbsolutePath(),
							ivMoveViewBack.getWidth(),
							ivMoveViewBack.getHeight());
					ivMoveViewBack.setImageBitmap(bitmap);
					/*
					 * BitmapDrawable bd = new BitmapDrawable(getResources(),
					 * bitmap); // TODO: should I recycle previous matrices?
					 * setBackDrawable(moveView, bd);
					 */
					ibCameraControlCapture.setVisibility(ImageButton.GONE);
					ibCameraControlUndo.setVisibility(ImageButton.VISIBLE);
					miSave.setEnabled(true);
					fLayoutCamera.setVisibility(FrameLayout.GONE);
					if (mCamera != null) {
						mCamera.release();
						mCamera = null;
						mPreview = null;
						fLayoutCamera.removeAllViews();
					}

				} else {
					bImg = ScribblerUtils.setPic(photoFile.getAbsolutePath(),
							ivBack.getWidth(), ivBack.getHeight());
					ivBack.setImageBitmap(bImg);
					divDraw.setMagnifyBitmap(bImg);
					ibZoomButton.setVisibility(ImageView.VISIBLE);
					initScribbles();
				}
			}
		}
	}

	private class RunKernelEstimationTask extends
			AsyncTask<Object, Integer, Bitmap> {
		ProgressDialog Asycdialog = new ProgressDialog(MainActivity.this);
		CustomLoading cDialog = CustomLoading
				.newInstance(R.drawable.loading_element_scissors);
		boolean useFIGTree;
		boolean useLevelset;

		@Override
		protected void onPreExecute() {
			// set message of the dialog
			Asycdialog.setMessage(getString(R.string.wait));
			Asycdialog.setCancelable(false);
			// show dialog
			// Asycdialog.show();
			cDialog.setCancelable(false);

			cDialog.show(getFragmentManager(), getString(R.string.wait));
			SharedPreferences settings =  PreferenceManager.getDefaultSharedPreferences(getApplicationContext());///getSharedPreferences(PREF_NAME, 0);
			useFIGTree = settings.getBoolean(PREF_USE_FIGTREE, true);
			useLevelset = settings.getBoolean(PREF_USE_LEVELSET, false);
			super.onPreExecute();
		}

		protected Bitmap doInBackground(Object... objects) {
			int mode = (Integer) objects[0];
			Mat frontScribbleMat = ScribblerUtils
					.scribblesToMat(frontScribbles);
			Mat backScribbleMat = ScribblerUtils.scribblesToMat(backScribbles);
			Mat picMat = new Mat();
			Mat resMat = new Mat();
			Utils.bitmapToMat(bImg, picMat);
			if (mode == 0) {
				Scalar s = new Scalar(255);
				objectMask = new Mat(picMat.rows(), picMat.cols(),
						CvType.CV_8UC1, s);
				frontPDF = new Mat();
				backPDF = new Mat();
			}
			DoKernelEstimation(picMat.getNativeObjAddr(),
					frontScribbleMat.getNativeObjAddr(),
					backScribbleMat.getNativeObjAddr(),
					resMat.getNativeObjAddr(), mode,
					frontPDF.getNativeObjAddr(), backPDF.getNativeObjAddr(),
					objectMask.getNativeObjAddr(), useFIGTree, useLevelset);
			Bitmap bm = Bitmap.createBitmap(resMat.cols(), resMat.rows(),
					Bitmap.Config.ARGB_8888);
			Utils.matToBitmap(resMat, bm);
			picMat.release();
			resMat.release();
			return bm;
		}

		protected void onProgressUpdate(Integer... progress) {
		}

		protected void onPostExecute(Bitmap result) {
			// Asycdialog.dismiss();
			cDialog.dismiss();
			if (result != null) {
				bContourImg = result;
				ivBack.setImageBitmap(bContourImg);
			}
		}
	}

	private class RunAlphaTask extends AsyncTask<Object, Integer, Bitmap> {
		ProgressDialog Asycdialog = new ProgressDialog(MainActivity.this);
		CustomLoading cDialog = CustomLoading
				.newInstance(R.drawable.loading_element_scissors);
		private float objectX, objectY;
		private boolean useFIGTree;

		@Override
		protected void onPreExecute() {
			// set message of the dialog
			Asycdialog.setMessage(getString(R.string.waitalpha));
			Asycdialog.setCancelable(false);
			// show dialog
			// Asycdialog.show();
			cDialog.setCancelable(false);

			cDialog.show(getFragmentManager(), getString(R.string.waitalpha));
			SharedPreferences settings =  PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
			useFIGTree = settings.getBoolean(PREF_USE_FIGTREE, true);
			super.onPreExecute();
		}

		protected Bitmap doInBackground(Object... objects) {
			Mat picMat = new Mat();
			Mat resMat = new Mat();
			Mat boundMat = new Mat();
			Utils.bitmapToMat(bImg, picMat);
			DoAlphaFromMask(picMat.getNativeObjAddr(),
					objectMask.getNativeObjAddr(), resMat.getNativeObjAddr(),
					boundMat.getNativeObjAddr(), useFIGTree);
			Bitmap bm = Bitmap.createBitmap(resMat.cols(), resMat.rows(),
					Bitmap.Config.ARGB_8888);
			Utils.matToBitmap(resMat, bm, true);

			objectX = (float) boundMat.get(0, 0)[0];
			objectY = (float) boundMat.get(0, 1)[0];
			Log.d(TAG, objectX + "," + objectY);
			picMat.release();
			resMat.release();
			return bm;
		}

		protected void onProgressUpdate(Integer... progress) {
		}

		protected void onPostExecute(Bitmap result) {
			// Asycdialog.dismiss();
			cDialog.dismiss();
			if (result != null) {
				new SaveResultTask().execute(result, "CutOuts");

				BitmapDrawable bd = new BitmapDrawable(getResources(), result);

				float cx, cy, sc;
				Matrix mat = new Matrix();
				mat = ivBack.getImageMatrix();
				float[] pts = { objectX, objectY };
				mat.mapPoints(pts);

				cx = Integer.valueOf((int) pts[0]);
				cy = Integer.valueOf((int) pts[1]);

				float[] imageMatrix = new float[9];
				mat.getValues(imageMatrix);
				sc = imageMatrix[Matrix.MSCALE_X];

				moveView.loadAnImageInPlace(getApplicationContext(), bd, cx,
						cy, sc);

				Log.d(TAG, "Matrix Transform:" + cx + "," + cy + "," + sc);

				updateVisibilty();
				miSave.setEnabled(false);
				// divDraw.setVisibility(View.GONE);
				mCamera = ScribblerUtils.getCameraInstance();
				// mCamera.setDisplayOrientation(90);
				// Create our Preview view and set it as the content of our
				// activity.
				mPreview = new CameraPreview(getApplicationContext(), mCamera);
				fLayoutCamera.addView(mPreview);

				ivMoveViewBack.setImageResource(getResources().getColor(
						R.color.transparent));

			}

		}

	}

	public static int[] getBitmapOffset(ImageView img, Boolean includeLayout) {
		int[] offset = new int[2];
		float[] values = new float[9];

		Matrix m = img.getImageMatrix();
		m.getValues(values);

		offset[0] = (int) values[5];
		offset[1] = (int) values[2];

		if (includeLayout) {
			ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams) img
					.getLayoutParams();
			int paddingTop = (int) (img.getPaddingTop());
			int paddingLeft = (int) (img.getPaddingLeft());

			offset[0] += paddingTop + lp.topMargin;
			offset[1] += paddingLeft + lp.leftMargin;
		}
		return offset;
	}

	private class RunBlurTask extends AsyncTask<Object, Integer, Bitmap> {
		ProgressDialog Asycdialog = new ProgressDialog(MainActivity.this);
		CustomLoading cDialog = CustomLoading
				.newInstance(R.drawable.loading_element_scissors);

		@Override
		protected void onPreExecute() {
			// set message of the dialog
			Asycdialog.setMessage(getString(R.string.waitblur));
			Asycdialog.setCancelable(false);
			// show dialog
			// Asycdialog.show();
			cDialog.setCancelable(false);

			cDialog.show(getFragmentManager(), getString(R.string.waitblur));
			super.onPreExecute();
		}

		protected Bitmap doInBackground(Object... objects) {
			int r = (Integer) objects[0];
			Mat picMat = new Mat();
			Mat resMat = new Mat();
			Log.d(TAG, "Blurring with" + r);
			Utils.bitmapToMat(bImg, picMat);
			DoBlurFromMask(picMat.getNativeObjAddr(),
					objectMask.getNativeObjAddr(), resMat.getNativeObjAddr(), r);
			Bitmap bm = Bitmap.createBitmap(resMat.cols(), resMat.rows(),
					Bitmap.Config.ARGB_8888);
			Utils.matToBitmap(resMat, bm, true);
			picMat.release();
			resMat.release();
			return bm;
		}

		protected void onProgressUpdate(Integer... progress) {
		}

		protected void onPostExecute(Bitmap result) {
			// Asycdialog.dismiss();
			cDialog.dismiss();
			if (result != null) {
				// new SaveResultTask().execute(result);
				// updateVisibilty();
				effectImg = result;
				ivBack.setImageBitmap(result);
				/*
				 * moveView.loadAnImage(getApplicationContext(),
				 * iViewBack.getDrawable()); moveViewMode = true;
				 */
			}

		}

	}

	private class RunGrayTask extends AsyncTask<Object, Integer, Bitmap> {
		ProgressDialog Asycdialog = new ProgressDialog(MainActivity.this);
		CustomLoading cDialog = CustomLoading
				.newInstance(R.drawable.loading_element_scissors);

		@Override
		protected void onPreExecute() {
			// set message of the dialog
			Asycdialog.setMessage(getString(R.string.waitblur));
			Asycdialog.setCancelable(false);
			// show dialog
			// Asycdialog.show();
			cDialog.setCancelable(false);

			cDialog.show(getFragmentManager(), getString(R.string.waitgray));
			super.onPreExecute();
		}

		protected Bitmap doInBackground(Object... objects) {
			float intensity = (Float) objects[0];
			Mat picMat = new Mat();
			Mat resMat = new Mat();
			Log.d(TAG, "Graying with" + intensity);
			Utils.bitmapToMat(bImg, picMat);
			DoGrayFromMask(picMat.getNativeObjAddr(),
					objectMask.getNativeObjAddr(), resMat.getNativeObjAddr(),
					intensity);
			Bitmap bm = Bitmap.createBitmap(resMat.cols(), resMat.rows(),
					Bitmap.Config.ARGB_8888);
			Utils.matToBitmap(resMat, bm, true);
			picMat.release();
			resMat.release();
			return bm;
		}

		protected void onProgressUpdate(Integer... progress) {
		}

		protected void onPostExecute(Bitmap result) {
			// Asycdialog.dismiss();
			cDialog.dismiss();
			if (result != null) {
				// new SaveResultTask().execute(result);
				// updateVisibilty();
				effectImg = result;
				ivBack.setImageBitmap(result);
				/*
				 * moveView.loadAnImage(getApplicationContext(),
				 * iViewBack.getDrawable()); moveViewMode = true;
				 */
			}

		}

	}

	private class SaveResultTask extends AsyncTask<Object, Integer, String> {
		ProgressDialog Asycdialog = new ProgressDialog(MainActivity.this);

		@Override
		protected void onPreExecute() {
			// set message of the dialog
			Asycdialog.setMessage(getString(R.string.savingtype));
			Asycdialog.setCancelable(false);
			// show dialog
			if (appMode == AppMode.EFFECT_BLUR || appMode == AppMode.EFFECT_GRAY) {
				Asycdialog.show();
			}
			super.onPreExecute();
		}

		protected String doInBackground(Object... objects) {
			Bitmap bitmap = (Bitmap) objects[0];
			String folderName = (String) objects[1];
			return ScribblerUtils.savePhoto(bitmap, folderName);
		}

		protected void onProgressUpdate(Integer... progress) {
		}

		protected void onPostExecute(String result) {
			if (appMode == AppMode.EFFECT_BLUR || appMode == AppMode.EFFECT_GRAY) {
				Asycdialog.dismiss();
			}
			ScribblerUtils.scanPhoto(result, getApplicationContext());
		}
	}

}
