package com.eladrich.scribbler;

import android.app.Dialog;
import android.app.DialogFragment;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.view.Window;
import android.view.WindowManager;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.ImageView;
import android.widget.TextView;

public class CustomLoading extends DialogFragment {
	int elementID;
	
	   /**
     * Create a new instance of MyDialogFragment, providing "num"
     * as an argument.
     */
    static CustomLoading newInstance(int num) {
    	CustomLoading customD = new CustomLoading();

        // Supply num input as an argument.
        Bundle args = new Bundle();
        args.putInt("element", num);
        customD.setArguments(args);

        return customD;
    }
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		//setStyle(DialogFragment.STYLE_NO_TITLE, R.style.MyDialog);
		// getDialog().getWindow().setBackgroundDrawable(getResources().getDrawable(R.color.transparent));
		elementID = getArguments().getInt("element");
	}

	@Override
	public Dialog onCreateDialog(Bundle savedInstanceState) {
		/*
		 * // Use the Builder class for convenient dialog construction
		 * AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
		 * 
		 * // Get the layout inflater LayoutInflater inflater =
		 * getActivity().getLayoutInflater();
		 * builder.setView(inflater.inflate(R.layout.dialog_loading, null));
		 * 
		 * // Create the AlertDialog object and return it return
		 * builder.create();
		 */
		final Dialog dialog = new Dialog(getActivity(), R.style.MyDialog);
		dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
		dialog.setContentView(R.layout.dialog_loading);
/*		WindowManager.LayoutParams lp = dialog.getWindow().getAttributes();  
		lp.dimAmount=0.5f; // Dim level. 0.0 - no dim, 1.0 - completely opaque
		dialog.getWindow().setAttributes(lp);*/

		 // Load the ImageView that will host the animation and
		 // set its background to our AnimationDrawable XML resource.
		 ImageView circle = (ImageView)dialog.findViewById(R.id.loading_circle_part);
		 Animation circleAnimation = AnimationUtils.loadAnimation(getActivity(), R.anim.rotate_counterclockwise);
		 circle.startAnimation(circleAnimation);
		 
		 ImageView element = (ImageView)dialog.findViewById(R.id.loading_element_part);
		 if (elementID != 0) {
			 element.setBackgroundResource(elementID);
		 }
		 Animation elementAnimation = AnimationUtils.loadAnimation(getActivity(), R.anim.rotate_clockwise);
		 element.startAnimation(elementAnimation);

		 TextView loadingtext = (TextView)dialog.findViewById(R.id.loading_text);
		 loadingtext.setText(getTag());
/*		 // Get the background, which has been compiled to an AnimationDrawable object.
		 AnimationDrawable frameAnimation = (AnimationDrawable) img.getBackground();

		 // Start the animation (looped playback by default).
		 frameAnimation.start();*/
		return dialog;

	}
	

}