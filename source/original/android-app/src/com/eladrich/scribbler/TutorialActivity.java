package com.eladrich.scribbler;

import android.app.ActionBar;
import android.content.Intent;
import android.graphics.YuvImage;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.app.ListFragment;
import android.support.v4.view.ViewPager;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

public class TutorialActivity extends FragmentActivity {
	static final int NUM_ITEMS = 4;
	static protected int[] TUTORIAL_IMAGES = { R.drawable.tutorial1, R.drawable.tutorial2,
			R.drawable.tutorial3, R.drawable.tutorial4 };
	static protected int[] TUTORIAL_STRINGS = { R.string.tutorial1_explain, R.string.tutorial2_explain,
			R.string.tutorial3_explain, R.string.tutorial4_explain };
	MyAdapter mAdapter;

	ViewPager mPager;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.fragment_pager);
		ActionBar appBar = getActionBar();
		appBar.setHomeButtonEnabled(true);
		appBar.setDisplayHomeAsUpEnabled(true);
		mAdapter = new MyAdapter(getSupportFragmentManager());

		mPager = (ViewPager) findViewById(R.id.pager);
		mPager.setAdapter(mAdapter);

		// Watch for button clicks.
		Button button = (Button) findViewById(R.id.goto_back);
		button.setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				int item = mPager.getCurrentItem() - 1;
				item = item < 0 ? 0 : item;
				mPager.setCurrentItem(item);
			}
		});
		button = (Button) findViewById(R.id.goto_next);
		button.setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				int item = mPager.getCurrentItem() + 1;
				item = item > NUM_ITEMS - 1 ? NUM_ITEMS - 1 : item;
				mPager.setCurrentItem(item);
			}
		});
	}
	
	/**
	 * apply the requested action for menu click
	 */
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle item selection
		switch (item.getItemId()) {
		case android.R.id.home:
			/*Intent home = new Intent(this, MainActivity.class);
			home.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);

			startActivity(home);*/
			onBackPressed();
			return true;
		default:
			return super.onOptionsItemSelected(item);
		}
	}

	public static class MyAdapter extends FragmentPagerAdapter {
		public MyAdapter(FragmentManager fm) {
			super(fm);
		}

		@Override
		public int getCount() {
			return NUM_ITEMS;
		}

		@Override
		public Fragment getItem(int position) {
			return ArrayListFragment.newInstance(position);
		}
	}

	public static class ArrayListFragment extends ListFragment {
		int mNum;

		/**
		 * Create a new instance of CountingFragment, providing "num" as an
		 * argument.
		 */
		static ArrayListFragment newInstance(int num) {
			ArrayListFragment f = new ArrayListFragment();

			// Supply num input as an argument.
			Bundle args = new Bundle();
			args.putInt("num", num);
			f.setArguments(args);

			return f;
		}

		/**
		 * When creating, retrieve this instance's number from its arguments.
		 */
		@Override
		public void onCreate(Bundle savedInstanceState) {
			super.onCreate(savedInstanceState);
			mNum = getArguments() != null ? getArguments().getInt("num") : 1;
		}

		/**
		 * The Fragment's UI is just a simple text view showing its instance
		 * number.
		 */
		@Override
		public View onCreateView(LayoutInflater inflater, ViewGroup container,
				Bundle savedInstanceState) {
			View v = inflater.inflate(R.layout.fragment_pager_list, container,
					false);
			TextView tv = (TextView)v.findViewById(R.id.tutorial_instruct);
			tv.setText(getResources().getString(TUTORIAL_STRINGS[mNum]));
			ImageView iv = (ImageView)v.findViewById(R.id.tutorial_image);
			iv.setImageDrawable(getResources().getDrawable(TUTORIAL_IMAGES[mNum]));
			return v;
		}

		@Override
		public void onActivityCreated(Bundle savedInstanceState) {
			super.onActivityCreated(savedInstanceState);
			/*
			 * setListAdapter(new ArrayAdapter<String>(getActivity(),
			 * android.R.layout.simple_list_item_1, Cheeses.sCheeseStrings));
			 */
		}

		@Override
		public void onListItemClick(ListView l, View v, int position, long id) {
			Log.i("FragmentList", "Item clicked: " + id);
		}
	}
}
