package com.eladrich.scribbler;

import android.app.Activity;
import android.app.Dialog;
import android.app.DialogFragment;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.widget.LinearLayout;

public class ColorDialog extends DialogFragment {

	/*
	 * The activity that creates an instance of this dialog fragment must
	 * implement this interface in order to receive event callbacks. Each method
	 * passes the DialogFragment in case the host needs to query it.
	 */
	public interface ColorDialogListener {
		public void onDialogClick(boolean frontMode);
	}

	// Use this instance of the interface to deliver action events
	ColorDialogListener mListener; //OOP Style :)

	// Override the Fragment.onAttach() method to instantiate the
	// NoticeDialogListener
	@Override
	public void onAttach(Activity activity) {
		super.onAttach(activity);
		// Verify that the host activity implements the callback interface
		try {
			// Instantiate the NoticeDialogListener so we can send events to the
			// host
			mListener = (ColorDialogListener) activity;
		} catch (ClassCastException e) {
			// The activity doesn't implement the interface, throw exception
			throw new ClassCastException(activity.toString()
					+ " must implement ColorDialogListener");
		}
	}

	@Override
	public Dialog onCreateDialog(Bundle savedInstanceState) {
/*		AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
		builder.setTitle(R.string.action_pick_scribble).setItems(
				R.array.scribble_array, new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {
						mListener.onDialogClick(which == 0);
						// The 'which' argument contains the index position
						// of the selected item
					}
				});
		return builder.create();*/
		final Dialog dialog = new Dialog(getActivity(), R.style.MyDialog);
		dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
		dialog.setContentView(R.layout.dialog_color);
		LinearLayout foregroundLayout = (LinearLayout)dialog.findViewById(R.id.layout_foreground);
		foregroundLayout.setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				mListener.onDialogClick(true);
				dialog.dismiss();
			}
		});
		LinearLayout backgroundLayout = (LinearLayout)dialog.findViewById(R.id.layout_background);
		backgroundLayout.setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				mListener.onDialogClick(false);
				dialog.dismiss();
			}
		});
		return dialog;
	}
}
