package peak.chao.picturecompression;

import android.graphics.Bitmap;

public class CompressUtil {

    static {
        System.loadLibrary("native-lib");
    }

    public native static int compressBitmap(Bitmap bitmap, int quality, String destFile);
}
