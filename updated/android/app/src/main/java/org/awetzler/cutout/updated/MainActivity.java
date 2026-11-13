package org.awetzler.cutout.updated;

import android.app.Activity;
import android.content.Intent;
import android.graphics.*;
import android.net.Uri;
import android.media.ExifInterface;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.*;
import org.json.JSONObject;
import java.io.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** Updated interface; original CutOut UI/source remains under source/original. */
public final class MainActivity extends Activity {
    private static final String TAG = "CutOutUpdated";
    private static final int PICK_IMAGE = 41;
    private final ExecutorService worker = Executors.newSingleThreadExecutor();
    private ImageCanvas imageView;
    private TextView status, sourceCredit;
    private static final String BASE_CREDIT = "Recovered Elad Richardson segmentation code. New UI/JNI adapter (2026). Draw both seed classes, then segment.";
    private CheckBox figtreeBox, levelSetBox, alphaBox;
    private Spinner workingSize;
    private int appliedWorkingPosition;
    private Button runButton;
    private boolean busy;

    @Override public void onCreate(Bundle saved) {
        super.onCreate(saved);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(12, 10, 12, 8);
        root.setBackgroundColor(Color.rgb(246, 248, 250));
        TextView title = new TextView(this);
        title.setText("CutOut · updated Android interface");
        title.setTextSize(21);
        title.setTextColor(Color.rgb(30, 42, 52));
        root.addView(title);
        sourceCredit = new TextView(this);
        sourceCredit.setText(BASE_CREDIT);
        sourceCredit.setTextSize(13);
        root.addView(sourceCredit);
        imageView = new ImageCanvas();
        root.addView(imageView, new LinearLayout.LayoutParams(-1, 0, 1));
        LinearLayout first = row(root);
        button(first, "Open image", v -> chooseImage());
        button(first, "Examples", v -> { if (!busy) new android.app.AlertDialog.Builder(this).setTitle("Choose an example").setItems(new String[]{"Ellipse run check", "Coffee photograph (CC0)", "Credits and terms"}, (dialog, which) -> { if (which == 0) loadFixture(); else if (which == 1) loadCoffee(); else showCredits(); }).show(); });
        button(first, "Clear", v -> { if (!busy) { imageView.clearSeeds(); status.setText("Scribbles cleared."); } });
        LinearLayout second = row(root);
        button(second, "Foreground", v -> { imageView.foregroundMode = true; status.setText("Draw green foreground scribbles."); });
        button(second, "Background", v -> { imageView.foregroundMode = false; status.setText("Draw red background scribbles."); });
        LinearLayout options = row(root);
        figtreeBox = new CheckBox(this); figtreeBox.setText("FIGTree"); figtreeBox.setChecked(true); options.addView(figtreeBox);
        levelSetBox = new CheckBox(this); levelSetBox.setText("Level-set tree"); options.addView(levelSetBox);
        LinearLayout processing = row(root);
        alphaBox = new CheckBox(this); alphaBox.setText("Refine alpha"); processing.addView(alphaBox, new LinearLayout.LayoutParams(0, -2, 1));
        workingSize = new Spinner(this);
        workingSize.setAdapter(new ArrayAdapter<String>(this, android.R.layout.simple_spinner_dropdown_item, new String[]{"Working: 400 px", "Working: 800 px", "Working: full input"}));
        workingSize.setContentDescription("Maximum working image dimension");
        processing.addView(workingSize, new LinearLayout.LayoutParams(0, -2, 1));
        LinearLayout third = row(root);
        runButton = button(third, "Segment", v -> segment());
        button(third, "Image / result", v -> {
            if (imageView.result == null) {
                imageView.showResult = false;
                status.setText("No current result. Segment the current image, scribbles and options first.");
            } else imageView.showResult = !imageView.showResult;
            imageView.invalidate();
        });
        status = new TextView(this); status.setTextSize(13); status.setMinLines(2); root.addView(status);
        setContentView(root);
        android.widget.CompoundButton.OnCheckedChangeListener changed = (control, checked) -> inputsChanged("Options changed. Segment again to update the result.");
        figtreeBox.setOnCheckedChangeListener(changed);
        levelSetBox.setOnCheckedChangeListener(changed);
        alphaBox.setOnCheckedChangeListener(changed);
        workingSize.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (position == appliedWorkingPosition) return;
                appliedWorkingPosition = position;
                inputsChanged("Working size changed. Segment again to update the result.");
            }
            @Override public void onNothingSelected(AdapterView<?> parent) {}
        });
        loadFixture();
        if (getIntent().getBooleanExtra("run_fixture", false)) {
            if ("coffee".equals(getIntent().getStringExtra("example"))) loadCoffee();
            figtreeBox.setChecked(getIntent().getBooleanExtra("figtree", true));
            levelSetBox.setChecked(getIntent().getBooleanExtra("level_set", false));
            alphaBox.setChecked(getIntent().getBooleanExtra("refine_alpha", false));
            int limit = getIntent().getIntExtra("max_dimension", 400);
            workingSize.setSelection(limit == 0 ? 2 : limit == 800 ? 1 : 0);
            imageView.postDelayed(this::segment, 400);
        }
    }

    private void inputsChanged(String message) {
        imageView.invalidateResult();
        status.setText(message);
    }
    private void setBusy(boolean value) {
        busy = value;
        runButton.setEnabled(!value);
        figtreeBox.setEnabled(!value);
        levelSetBox.setEnabled(!value);
        alphaBox.setEnabled(!value);
        workingSize.setEnabled(!value);
    }
    private String readAssetText(String path) throws IOException {
        try (InputStream in = getAssets().open(path); ByteArrayOutputStream out = new ByteArrayOutputStream()) {
            byte[] buffer = new byte[4096];
            int count;
            while ((count = in.read(buffer)) != -1) out.write(buffer, 0, count);
            return out.toString("UTF-8");
        }
    }
    private void showText(String title, String text) {
        TextView body = new TextView(this);
        body.setText(text); body.setTextSize(14); body.setPadding(24, 12, 24, 12); body.setTextIsSelectable(true);
        ScrollView scroll = new ScrollView(this); scroll.addView(body);
        new android.app.AlertDialog.Builder(this).setTitle(title).setView(scroll).setPositiveButton("Close", null).show();
    }
    private void showCredits() {
        try {
            TextView body = new TextView(this); body.setText(readAssetText("credits.txt")); body.setTextSize(14); body.setPadding(24, 12, 24, 12); body.setTextIsSelectable(true);
            ScrollView scroll = new ScrollView(this); scroll.addView(body);
            new android.app.AlertDialog.Builder(this).setTitle("Credits and terms").setView(scroll).setPositiveButton("Close", null).setNeutralButton("License texts", (dialog, which) -> showLicenseList()).show();
        } catch (IOException error) { status.setText("Cannot read credits: " + error.getMessage()); }
    }
    private void collectLicensePaths(String directory, java.util.List<String> paths) throws IOException {
        String[] entries = getAssets().list(directory);
        if (entries == null) return;
        for (String name : entries) {
            String path = directory + "/" + name;
            String[] children = getAssets().list(path);
            if (children != null && children.length > 0) collectLicensePaths(path, paths);
            else paths.add(path);
        }
    }
    private void showLicenseList() {
        try {
            java.util.List<String> paths = new java.util.ArrayList<>(); collectLicensePaths("licenses", paths); java.util.Collections.sort(paths);
            String[] names = paths.stream().map(path -> path.substring("licenses/".length())).toArray(String[]::new);
            new android.app.AlertDialog.Builder(this).setTitle("Packaged license texts").setItems(names, (dialog, which) -> {
                try { showText(names[which], readAssetText(paths.get(which))); }
                catch (IOException error) { status.setText("Cannot read license: " + error.getMessage()); }
            }).setNegativeButton("Close", null).show();
        } catch (IOException error) { status.setText("Cannot list licenses: " + error.getMessage()); }
    }
    private LinearLayout row(LinearLayout parent) {
        LinearLayout layout = new LinearLayout(this); layout.setOrientation(LinearLayout.HORIZONTAL);
        parent.addView(layout); return layout;
    }
    private Button button(LinearLayout row, String text, View.OnClickListener click) {
        Button b = new Button(this); b.setText(text); b.setTextSize(12); b.setAllCaps(false);
        row.addView(b, new LinearLayout.LayoutParams(0, -2, 1)); b.setOnClickListener(click); return b;
    }
    private void chooseImage() {
        if (busy) return;
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT); intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("image/*"); startActivityForResult(intent, PICK_IMAGE);
    }
    @Override protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (busy || request != PICK_IMAGE || result != RESULT_OK || data == null) return;
        Uri uri = data.getData();
        try {
            BitmapFactory.Options size = new BitmapFactory.Options(); size.inJustDecodeBounds = true;
            try (InputStream in = getContentResolver().openInputStream(uri)) { BitmapFactory.decodeStream(in, null, size); }
            if (size.outWidth < 1 || size.outHeight < 1) throw new IOException("Unsupported image");
            BitmapFactory.Options decode = new BitmapFactory.Options(); decode.inSampleSize = 1;
            while (Math.max(size.outWidth, size.outHeight) / decode.inSampleSize > 1600) decode.inSampleSize *= 2;
            Bitmap bitmap;
            try (InputStream in = getContentResolver().openInputStream(uri)) { bitmap = BitmapFactory.decodeStream(in, null, decode); }
            if (bitmap == null) throw new IOException("Could not decode image");
            try (InputStream in = getContentResolver().openInputStream(uri)) {
                int orientation = new ExifInterface(in).getAttributeInt(ExifInterface.TAG_ORIENTATION, ExifInterface.ORIENTATION_NORMAL);
                Matrix transform = new Matrix();
                switch (orientation) {
                    case ExifInterface.ORIENTATION_FLIP_HORIZONTAL: transform.setScale(-1, 1); break;
                    case ExifInterface.ORIENTATION_ROTATE_180: transform.setRotate(180); break;
                    case ExifInterface.ORIENTATION_FLIP_VERTICAL: transform.setScale(1, -1); break;
                    case ExifInterface.ORIENTATION_TRANSPOSE: transform.setRotate(90); transform.postScale(-1, 1); break;
                    case ExifInterface.ORIENTATION_ROTATE_90: transform.setRotate(90); break;
                    case ExifInterface.ORIENTATION_TRANSVERSE: transform.setRotate(270); transform.postScale(-1, 1); break;
                    case ExifInterface.ORIENTATION_ROTATE_270: transform.setRotate(270); break;
                }
                if (!transform.isIdentity()) bitmap = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), transform, true);
            } catch (IOException ignored) { /* PNG/other images may have no Exif metadata. */ }
            double scale = Math.min(1.0, 800.0 / Math.max(bitmap.getWidth(), bitmap.getHeight()));
            if (scale < 1) bitmap = Bitmap.createScaledBitmap(bitmap, Math.max(1, (int)Math.round(bitmap.getWidth()*scale)), Math.max(1, (int)Math.round(bitmap.getHeight()*scale)), true);
            imageView.setImage(bitmap); sourceCredit.setText(BASE_CREDIT); status.setText("Loaded " + bitmap.getWidth() + " × " + bitmap.getHeight() + ". Draw foreground and background scribbles.");
        } catch (Exception error) { status.setText("Open failed: " + error.getMessage()); Log.e(TAG, "Open image", error); }
    }
    private void loadCoffee() {
        try {
            Bitmap photograph;
            try (InputStream in = getAssets().open("examples/coffee.png")) { photograph = BitmapFactory.decodeStream(in); }
            if (photograph == null) throw new IOException("Cannot decode coffee example");
            imageView.setImage(photograph);
            byte[] jsonBytes;
            try (InputStream in = getAssets().open("examples/coffee-scribbles.json")) { java.io.ByteArrayOutputStream bytes = new java.io.ByteArrayOutputStream(); byte[] buffer = new byte[4096]; int read; while ((read = in.read(buffer)) != -1) bytes.write(buffer, 0, read); jsonBytes = bytes.toByteArray(); }
            JSONObject scribbles = new JSONObject(new String(jsonBytes, java.nio.charset.StandardCharsets.UTF_8));
            drawStrokes(imageView.foreground, scribbles.getJSONArray("foreground"), scribbles.getInt("brush_radius"));
            drawStrokes(imageView.background, scribbles.getJSONArray("background"), scribbles.getInt("brush_radius"));
            sourceCredit.setText(BASE_CREDIT + " Coffee photo: Rachel Michetti / Pikolo Espresso Bar, CC0 (scikit-image).");
            imageView.invalidate(); status.setText("Coffee photograph, 600 × 400, with supplied example scribbles. No independent reference mask.");
        } catch (Exception error) { status.setText("Example failed: " + error.getMessage()); Log.e(TAG, "Coffee example", error); }
    }
    private static void drawStrokes(Bitmap mask, org.json.JSONArray strokes, int radius) throws org.json.JSONException {
        Paint stroke = new Paint(); stroke.setColor(Color.WHITE); stroke.setStyle(Paint.Style.STROKE); stroke.setStrokeWidth(2*radius+1); stroke.setStrokeCap(Paint.Cap.ROUND);
        Canvas canvas = new Canvas(mask);
        for (int i=0;i<strokes.length();i++) {
            org.json.JSONArray points=strokes.getJSONArray(i);
            for (int j=0;j<points.length();j++) {
                org.json.JSONArray point=points.getJSONArray(j), previous=points.getJSONArray(Math.max(0,j-1));
                canvas.drawLine((float)previous.getDouble(0),(float)previous.getDouble(1),(float)point.getDouble(0),(float)point.getDouble(1),stroke);
            }
        }
    }
    private void loadFixture() {
        sourceCredit.setText(BASE_CREDIT);
        Bitmap bitmap = Bitmap.createBitmap(128, 96, Bitmap.Config.ARGB_8888);
        Canvas c = new Canvas(bitmap); c.drawColor(Color.rgb(42, 72, 130));
        Paint paint = new Paint(); paint.setColor(Color.rgb(220, 112, 42)); c.drawOval(30, 18, 100, 82, paint);
        imageView.setImage(bitmap);
        paint.setColor(Color.WHITE); paint.setStrokeWidth(5); paint.setStyle(Paint.Style.STROKE);
        new Canvas(imageView.foreground).drawLine(57, 40, 68, 60, paint);
        new Canvas(imageView.background).drawLine(10, 10, 115, 10, paint);
        imageView.invalidate(); status.setText("Procedural fixture with foreground/background seeds. This is a run check, not a photograph benchmark.");
    }
    private byte[] maskBytes(Bitmap bitmap) {
        int n = bitmap.getWidth() * bitmap.getHeight(); int[] pixels = new int[n]; byte[] mask = new byte[n];
        bitmap.getPixels(pixels, 0, bitmap.getWidth(), 0, 0, bitmap.getWidth(), bitmap.getHeight());
        for (int i=0; i<n; i++) mask[i] = (byte)((pixels[i] >>> 24) != 0 ? 255 : 0);
        return mask;
    }
    private void segment() {
        if (busy) return;
        setBusy(true); imageView.invalidateResult();
        final int width = imageView.image.getWidth(), height = imageView.image.getHeight();
        final int[] pixels = new int[width*height]; imageView.image.getPixels(pixels,0,width,0,0,width,height);
        final byte[] foreground = maskBytes(imageView.foreground), background = maskBytes(imageView.background);
        final boolean figtree = figtreeBox.isChecked(), levelSet = levelSetBox.isChecked(), refineAlpha = alphaBox.isChecked();
        final int maxDimension = workingSize.getSelectedItemPosition() == 2 ? 0 : workingSize.getSelectedItemPosition() == 1 ? 800 : 400;
        final Bitmap input = imageView.image.copy(Bitmap.Config.ARGB_8888, false);
        final Bitmap fgCopy = imageView.foreground.copy(Bitmap.Config.ARGB_8888, false), bgCopy = imageView.background.copy(Bitmap.Config.ARGB_8888, false);
        status.setText("Running recovered segmentation…");
        worker.execute(() -> {
            long start = System.nanoTime();
            try {
                int[] packed = NativeBridge.segment(pixels, width, height, foreground, background, figtree, levelSet, refineAlpha, maxDimension);
                int[] output = java.util.Arrays.copyOf(packed, width*height);
                double elapsedMs = (System.nanoTime()-start)/1e6;
                Bitmap cutout = Bitmap.createBitmap(output,width,height,Bitmap.Config.ARGB_8888);
                int selected = 0; int[] maskPixels = new int[output.length];
                for (int i=0; i<output.length; i++) { boolean object=packed[output.length+i]!=0; if(object) selected++; maskPixels[i]=object?Color.WHITE:Color.BLACK; }
                JSONObject record = new JSONObject(); record.put("width",width);record.put("height",height);record.put("figtree",figtree);record.put("level_set",levelSet);record.put("refine_alpha",refineAlpha);record.put("max_dimension",maxDimension);record.put("selected_pixels",selected);record.put("elapsed_ms",elapsedMs);record.put("output",refineAlpha ? "recovered boundary-band alpha refinement" : "binary-mask alpha");
                String persistenceStatus;
                try {
                    File external = getExternalFilesDir(null);
                    File directory = new File(external != null ? external : getFilesDir(), "last-run");
                    if(!directory.exists() && !directory.mkdirs()) throw new IOException("Cannot create result directory");
                    // result.json is the completion marker: invalidate it before changing any PNG.
                    File marker = new File(directory, "result.json");
                    File pendingMarker = new File(directory, "result.json.pending");
                    if (marker.exists() && !marker.delete()) throw new IOException("Cannot invalidate previous completion marker");
                    if (pendingMarker.exists() && !pendingMarker.delete()) throw new IOException("Cannot clear incomplete completion marker");
                    savePng(input,new File(directory,"input.png"));savePng(fgCopy,new File(directory,"foreground.png"));savePng(bgCopy,new File(directory,"background.png"));
                    savePng(cutout,new File(directory,"cutout.png"));savePng(Bitmap.createBitmap(maskPixels,width,height,Bitmap.Config.ARGB_8888),new File(directory,"mask.png"));
                    try(Writer writer=new OutputStreamWriter(new FileOutputStream(pendingMarker),"UTF-8")){writer.write(record.toString(2));}
                    if (!pendingMarker.renameTo(marker)) throw new IOException("Cannot publish completion marker");
                    persistenceStatus = external != null ? "Saved to app files/last-run." : "Saved to internal app files/last-run.";
                } catch (Exception saveError) {
                    // A completed computation remains visible even when output storage is unavailable.
                    persistenceStatus = "Result displayed; saving failed: " + saveError.getMessage();
                    record.put("persistence_warning", saveError.getMessage());
                    Log.w(TAG,"CUTOUT_SAVE_WARNING",saveError);
                }
                Log.i(TAG,"CUTOUT_RESULT " + record);
                final int selectedPixels=selected;
                final String saveMessage=persistenceStatus;
                runOnUiThread(() -> { imageView.result=cutout;imageView.showResult=true;imageView.invalidate();status.setText(String.format(java.util.Locale.ROOT,"Segmented %d / %d pixels in %.1f ms. %s",selectedPixels,output.length,elapsedMs,saveMessage)); });
            } catch(Throwable error) { Log.e(TAG,"CUTOUT_ERROR",error);runOnUiThread(()->status.setText("Segmentation failed: "+error.getMessage())); }
            finally { runOnUiThread(()->setBusy(false)); }
        });
    }
    private static void savePng(Bitmap bitmap,File path)throws IOException { try(OutputStream out=new FileOutputStream(path)){if(!bitmap.compress(Bitmap.CompressFormat.PNG,100,out))throw new IOException("PNG write failed");} }
    @Override protected void onDestroy(){worker.shutdown();super.onDestroy();}

    private final class ImageCanvas extends View {
        Bitmap image,foreground,background,result;
        boolean foregroundMode=true,showResult;
        private final RectF destination=new RectF();
        private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
        private float previousX,previousY;
        private boolean continuingStroke;
        ImageCanvas(){super(MainActivity.this);setMinimumHeight(160);setContentDescription("Image with foreground and background scribbles; use the mode buttons, then draw on the image");}
        void setImage(Bitmap bitmap){image=bitmap;foreground=Bitmap.createBitmap(bitmap.getWidth(),bitmap.getHeight(),Bitmap.Config.ARGB_8888);background=Bitmap.createBitmap(bitmap.getWidth(),bitmap.getHeight(),Bitmap.Config.ARGB_8888);continuingStroke=false;invalidateResult();}
        void clearSeeds(){foreground.eraseColor(Color.TRANSPARENT);background.eraseColor(Color.TRANSPARENT);continuingStroke=false;invalidateResult();}
        void invalidateResult(){result=null;showResult=false;invalidate();}
        @Override protected void onDraw(Canvas canvas){
            super.onDraw(canvas);if(image==null)return;
            float scale=Math.min((getWidth()-8f)/image.getWidth(),(getHeight()-8f)/image.getHeight());
            float w=image.getWidth()*scale,h=image.getHeight()*scale;destination.set((getWidth()-w)/2,(getHeight()-h)/2,(getWidth()+w)/2,(getHeight()+h)/2);
            canvas.save();canvas.clipRect(destination);
            for(int y=(int)destination.top;y<destination.bottom;y+=20)for(int x=(int)destination.left;x<destination.right;x+=20){paint.setColor((((x-(int)destination.left)/20+(y-(int)destination.top)/20)&1)==0?0xffdedede:0xffffffff);canvas.drawRect(x,y,x+20,y+20,paint);}
            paint.setColorFilter(null);paint.setAlpha(255);canvas.drawBitmap(showResult&&result!=null?result:image,null,destination,paint);
            if(!showResult){paint.setColorFilter(new PorterDuffColorFilter(0xff29c55a,PorterDuff.Mode.SRC_IN));canvas.drawBitmap(foreground,null,destination,paint);paint.setColorFilter(new PorterDuffColorFilter(0xffef4444,PorterDuff.Mode.SRC_IN));canvas.drawBitmap(background,null,destination,paint);paint.setColorFilter(null);}
            canvas.restore();
        }
        @Override public boolean performClick(){super.performClick();return true;}
        @Override public boolean onTouchEvent(MotionEvent event){
            if (busy || image == null) return false;
            int action = event.getActionMasked();
            if (action == MotionEvent.ACTION_CANCEL) { continuingStroke = false; return true; }
            if (!destination.contains(event.getX(), event.getY())) {
                continuingStroke = false;
                return action != MotionEvent.ACTION_DOWN;
            }
            if (action != MotionEvent.ACTION_DOWN && action != MotionEvent.ACTION_MOVE && action != MotionEvent.ACTION_UP) return true;
            float x=(event.getX()-destination.left)*image.getWidth()/destination.width(), y=(event.getY()-destination.top)*image.getHeight()/destination.height();
            if (action == MotionEvent.ACTION_DOWN || !continuingStroke) {
                previousX=x; previousY=y;
                inputsChanged("Scribbles changed. Segment again to update the result.");
            } else invalidateResult();
            Paint stroke=new Paint(); stroke.setColor(Color.WHITE); stroke.setStrokeWidth(5); stroke.setStrokeCap(Paint.Cap.ROUND); stroke.setStyle(Paint.Style.STROKE);
            Paint erase=new Paint(stroke); erase.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.CLEAR));
            Canvas active=new Canvas(foregroundMode?foreground:background), opposite=new Canvas(foregroundMode?background:foreground);
            active.drawLine(previousX,previousY,x,y,stroke); opposite.drawLine(previousX,previousY,x,y,erase);
            previousX=x; previousY=y; continuingStroke=action != MotionEvent.ACTION_UP;
            invalidate();
            if (action == MotionEvent.ACTION_UP) performClick();
            return true;
        }
    }
}
