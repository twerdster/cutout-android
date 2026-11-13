package com.eladrich.scribbler;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.Locale;

import org.opencv.core.CvType;
import org.opencv.core.Mat;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.hardware.Camera;
import android.media.ExifInterface;
import android.net.Uri;
import android.os.Environment;

public class ScribblerUtils {

	/**
	 * Save the bitmap to the given file
	 * 
	 * @param bitmap
	 *            - the bitmap we want to save
	 * @param file
	 *            - the file we want to write it into
	 * @return the given file for success, or null for failure
	 */
	public static File savebitmap(Bitmap bitmap, File file) {
		OutputStream outStream = null;

		try {
			// Create a new file from bitmap
			outStream = new FileOutputStream(file);
			bitmap.compress(Bitmap.CompressFormat.JPEG, 100, outStream);
			outStream.flush();
			outStream.close();
		} catch (Exception e) {
			e.printStackTrace();
			return null;
		}

		return file;
	}

	public static String fromInt(int val) {
		return String.valueOf(val);
	}

	public static Mat scribbleToMat(ArrayList<float[]> scribble) {
		Mat mScribble = new Mat(2, scribble.size(), CvType.CV_32F);
		for (int i = 0; i < scribble.size(); i++) {
			float[] event = scribble.get(i);
			mScribble.put(0, i, event[0]);
			mScribble.put(1, i, event[1]);
		}
		return mScribble;
	}

	public static Mat scribblesToMat(ArrayList<ArrayList<float[]>> scribbles) {
		int max = maxScribble(scribbles);
		float[] limit = { -1, -1 };
		Mat mScribble = new Mat(scribbles.size(), max, CvType.CV_32FC2);
		for (int i = 0; i < scribbles.size(); i++) {
			ArrayList<float[]> scribble = scribbles.get(i);
			for (int j = 0; j < scribble.size(); j++) {
				float[] event = scribble.get(j);
				mScribble.put(i, j, event);
			}
			if (scribble.size() < max) {
				mScribble.put(i, scribble.size(), limit);
			}
		}
		return mScribble;
	}

	public static int maxScribble(ArrayList<ArrayList<float[]>> scribbles) {
		int maxSize = 0;
		for (int i = 0; i < scribbles.size(); i++) {
			int size = scribbles.get(i).size();
			maxSize = size > maxSize ? size : maxSize;
		}
		return maxSize;
	}

	/**
	 * Decodes a bitmap from an image view and scale it down
	 * 
	 * @param view
	 */
	public static Bitmap setPic(String path, int tWidth, int tHeight) {
		// Get the dimensions of the View

		int targetW = tWidth;
		int targetH = tHeight;

		// Get the dimensions of the bitmap
		BitmapFactory.Options bmOptions = new BitmapFactory.Options();
		bmOptions.inJustDecodeBounds = true;
		BitmapFactory.decodeFile(path, bmOptions);
		int photoW = bmOptions.outWidth;
		int photoH = bmOptions.outHeight;

		// Determine how much to scale down the image
		int scaleFactor = Math.max(photoW / targetW, photoH / targetH);

		// Decode the image file into a Bitmap sized to fill the View
		bmOptions.inJustDecodeBounds = false;
		bmOptions.inSampleSize = scaleFactor;
		bmOptions.inPurgeable = true;

		// BitmapFactory.Options bmOptions = new BitmapFactory.Options();

		Bitmap res = BitmapFactory.decodeFile(path, bmOptions);
		int orientation = getOrientationFromExif(path);
		if (orientation != -1) {
			return Exif.rotate(res, orientation);
		} else {
			return res;
		}

	}
	
	public static Bitmap decodeUriIntoPath(Context c, Uri uri, String path, int tWidth, int tHeight) 
            throws FileNotFoundException {
		// Get the dimensions of the View

		int targetW = tWidth;
		int targetH = tHeight;
		
        BitmapFactory.Options bmOptions = new BitmapFactory.Options();
        bmOptions.inJustDecodeBounds = true;
        BitmapFactory.decodeStream(c.getContentResolver().openInputStream(uri), null, bmOptions);

        int photoW = bmOptions.outWidth;
		int photoH = bmOptions.outHeight;

		// Determine how much to scale down the image
		int scaleFactor = Math.max(photoW / targetW, photoH / targetH);

		// Decode the image file into a Bitmap sized to fill the View
		bmOptions.inJustDecodeBounds = false;
		bmOptions.inSampleSize = scaleFactor;
		bmOptions.inPurgeable = true;


		return BitmapFactory.decodeStream(c.getContentResolver().openInputStream(uri), null, bmOptions);
/*		int orientation = getOrientationFromExif(path);
		if (orientation != -1) {
			return Exif.rotate(res, orientation);
		} else {
			return res;
		}*/
    } 

	private static int getOrientationFromExif(String imagePath) {
		int orientation = -1;
		try {
			ExifInterface exif = new ExifInterface(imagePath);
			int exifOrientation = exif.getAttributeInt(
					ExifInterface.TAG_ORIENTATION,
					ExifInterface.ORIENTATION_NORMAL);

			switch (exifOrientation) {
			case ExifInterface.ORIENTATION_ROTATE_270:
				orientation = 270;

				break;
			case ExifInterface.ORIENTATION_ROTATE_180:
				orientation = 180;

				break;
			case ExifInterface.ORIENTATION_ROTATE_90:
				orientation = 90;

				break;

			case ExifInterface.ORIENTATION_NORMAL:
				orientation = 0;

				break;
			default:
				break;
			}
		} catch (IOException e) {
			// Log.e(TAG, "Unable to get image exif orientation", e);
		}

		return orientation;
	}

	/**
	 * creates the image file
	 * 
	 * @param storageDir
	 *            - getExternalFilesDir(null);
	 * @return the File
	 * @throws IOException
	 */
	public static File createImageFile(File storageDir) throws IOException {
		// Create an image file name
		String timeStamp = new SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US)
				.format(new Date());
		String imageFileName = "JPEG_" + timeStamp + "_";
		File image = File.createTempFile(imageFileName, /* prefix */
				".jpg", /* suffix */
				storageDir /* directory */
		);

		return image;
	}

	public static String savePhoto(Bitmap bmp, String folderName) {
		/*
		 * File imageFileFolder = new File(
		 * Environment.getExternalStorageDirectory(), "Scribbler"); // when
		 */// you
			// need
			// to
			// save
			// the
			// image
			// inside
			// your
			// own
			// folder
			// in
			// the
			// SD
			// Card

		/*
		 * File path = Environment
		 * .getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES);
		 */
		File path = new File(Environment.getExternalStoragePublicDirectory(
				Environment.DIRECTORY_PICTURES).getPath()
				+ "/" + folderName + "/");
		if (!path.exists()) {
			path.mkdirs();
		}
		/*
		 * Uri uri =
		 * Uri.parse(Environment.getExternalStorageDirectory().getPath() +
		 * "/myFolder/");
		 */
		// this is the default location inside SD Card - Pictures folder
		// imageFileFolder.mkdir(); //when you create your own folder, you use
		// this line.
		FileOutputStream out = null;
		Calendar c = Calendar.getInstance();
		String date = fromInt(c.get(Calendar.MONTH))
				+ fromInt(c.get(Calendar.DAY_OF_MONTH))
				+ fromInt(c.get(Calendar.YEAR))
				+ fromInt(c.get(Calendar.HOUR_OF_DAY))
				+ fromInt(c.get(Calendar.MINUTE))
				+ fromInt(c.get(Calendar.SECOND));
		File imageFileName = new File(path, date.toString() + ".png"); // imageFileFolder

		try {
			out = new FileOutputStream(imageFileName);
			bmp.compress(Bitmap.CompressFormat.PNG, 100, out);
			out.flush();
			out.close();
			// scanPhoto(imageFileName.toString());
			out = null;
		} catch (Exception e) {
			e.printStackTrace();
		}
		return imageFileName.toString();
	}

	/** A safe way to get an instance of the Camera object. */
	public static Camera getCameraInstance() {
		Camera c = null;
		try {
			c = Camera.open(); // attempt to get a Camera instance
		} catch (Exception e) {
			// Camera is not available (in use or does not exist)
		}
		return c; // returns null if camera is unavailable
	}

	/*
	 * invoke the system's media scanner to add your photo to the Media
	 * Provider's database, making it available in the Android Gallery
	 * application and to other apps.
	 */
	public static void scanPhoto(String imageFileName, Context context) {
		Intent mediaScanIntent = new Intent(
				Intent.ACTION_MEDIA_SCANNER_SCAN_FILE);
		File f = new File(imageFileName);
		Uri contentUri = Uri.fromFile(f);
		mediaScanIntent.setData(contentUri);
		// this.cordova.getContext().sendBroadcast(mediaScanIntent); //this is
		// deprecated
		context.sendBroadcast(mediaScanIntent);
	}

}
