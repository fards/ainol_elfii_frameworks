package com.htc.view;

import android.util.Log;
import android.view.Surface;
import android.view.Display;
import android.os.RemoteException;

public class DisplaySetting
{
	public static final int STEREOSCOPIC_3D_FORMAT_OFF=0;
	public static final int STEREOSCOPIC_3D_FORMAT_SIDE_BY_SIDE=1;
	public static final int STEREOSCOPIC_3D_FORMAT_TOP_BOTTOM=2;
	public static final int STEREOSCOPIC_3D_FORMAT_INTERLEAVED=4;
	public DisplaySetting()
	{
	
	}
	
	public static boolean setStereoscopic3DFormat(android.view.Surface surface,int format)
	{
		try {
			Log.v("yjf DisplaySetting1","");
			Display.getWindowManager().setSformat(surface.getIdentity(),format);
			Log.v("yjf DisplaySetting2","id "+surface.getIdentity());
		}catch (RemoteException e) {
			Log.v("yjf DisplaySetting err","id "+surface.getIdentity());
        }
		return true;
	}
}
