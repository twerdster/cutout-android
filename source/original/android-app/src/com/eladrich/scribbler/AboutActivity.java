package com.eladrich.scribbler;

import android.app.ActionBar;
import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.MenuItem;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.Switch;

public class AboutActivity extends Activity {
	public final String PREF_NAME = "eladrich_cutout_pref";
	public final String PREF_USE_FIGTREE = "cutout_use_figtree";

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_about);
		ActionBar appBar = getActionBar();
		appBar.setHomeButtonEnabled(true);
		appBar.setDisplayHomeAsUpEnabled(true);
		/*Switch figtreeSwitch = (Switch) findViewById(R.id.figtree_switch);
		SharedPreferences settings = getSharedPreferences(PREF_NAME, 0);
		figtreeSwitch.setChecked(settings.getBoolean(PREF_USE_FIGTREE, true));
		figtreeSwitch.setOnCheckedChangeListener(new OnCheckedChangeListener() {

			@Override
			public void onCheckedChanged(CompoundButton buttonView,
					boolean isChecked) {
				// TODO Auto-generated method stub
				SharedPreferences settings = getSharedPreferences(PREF_NAME, 0);
				SharedPreferences.Editor editor = settings.edit();
				editor.putBoolean(PREF_USE_FIGTREE, isChecked);
				editor.commit();
			}
		});
*/	}

	/**
	 * apply the requested action for menu click
	 */
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle item selection
		switch (item.getItemId()) {
		case android.R.id.home:
			Intent home = new Intent(this, MainActivity.class);
			home.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);

			startActivity(home);
			return true;
		default:
			return super.onOptionsItemSelected(item);
		}
	}
}
