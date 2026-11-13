package org.awetzler.cutout.updated;

/** New JNI boundary for the updated recovered C++ implementation. */
final class NativeBridge {
    static { System.loadLibrary("cutout_android"); }
    // Packed result: width*height ARGB cutout values, then width*height binary mask bytes as ints.
    static native int[] segment(int[] argb, int width, int height,
            byte[] foreground, byte[] background, boolean figtree, boolean levelSet, boolean refineAlpha, int maxDimension);
    private NativeBridge() {}
}
