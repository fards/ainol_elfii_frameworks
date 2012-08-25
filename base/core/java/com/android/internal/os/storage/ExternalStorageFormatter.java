package com.android.internal.os.storage;

import android.app.ProgressDialog;
import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Environment;
import android.os.IBinder;
import android.os.PowerManager;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.storage.IMountService;
import android.os.storage.StorageEventListener;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.util.Log;
import android.view.WindowManager;
import android.widget.Toast;

import com.android.internal.R;

/**
 * Takes care of unmounting and formatting external storage.
 */
public class ExternalStorageFormatter extends Service
        implements DialogInterface.OnCancelListener {
    static final String TAG = "ExternalStorageFormatter";

    public static final String FORMAT_ONLY = "com.android.internal.os.storage.FORMAT_ONLY";
    public static final String FORMAT_AND_FACTORY_RESET = "com.android.internal.os.storage.FORMAT_AND_FACTORY_RESET";

    public static final String EXTRA_ALWAYS_RESET = "always_reset";

    // If non-null, the volume to format. Otherwise, will use the default external storage directory
    private StorageVolume mStorageVolume;

    public static final ComponentName COMPONENT_NAME
            = new ComponentName("android", ExternalStorageFormatter.class.getName());

    // Access using getMountService()
    private IMountService mMountService = null;

    private StorageManager mStorageManager = null;

    private PowerManager.WakeLock mWakeLock;

    private ProgressDialog mProgressDialog = null;

    private boolean mFactoryReset = false;
    private boolean mAlwaysReset = false;
	
    // If !true, v-sdcard in enabled
    private boolean mTryMountExtStorage2 = false;
    private static boolean mExternalStoragebeSD = true;

    StorageEventListener mStorageListener = new StorageEventListener() {
        @Override
        public void onStorageStateChanged(String path, String oldState, String newState) {
            Log.i(TAG, "Received storage state changed notification that " +
                    path + " changed state from " + oldState +
                    " to " + newState);
            updateProgressState(path,newState);
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();

        mExternalStoragebeSD = Environment.isExternalStorageBeSdcard();

        if (mStorageManager == null) {
            mStorageManager = (StorageManager) getSystemService(Context.STORAGE_SERVICE);
            mStorageManager.registerListener(mStorageListener);
        }

        mWakeLock = ((PowerManager)getSystemService(Context.POWER_SERVICE))
                .newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "ExternalStorageFormatter");
        mWakeLock.acquire();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (FORMAT_AND_FACTORY_RESET.equals(intent.getAction())) {
            mFactoryReset = true;
        }
        if (intent.getBooleanExtra(EXTRA_ALWAYS_RESET, false)) {
            mAlwaysReset = true;
        }

        mStorageVolume = intent.getParcelableExtra(StorageVolume.EXTRA_STORAGE_VOLUME);

        if (mProgressDialog == null) {
            mProgressDialog = new ProgressDialog(this);
            mProgressDialog.setIndeterminate(true);
            mProgressDialog.setCancelable(true);
            mProgressDialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
            if (!mAlwaysReset) {
                mProgressDialog.setOnCancelListener(this);
            }
            tryUnmountExternalStorage2();
	     updateProgressState(null,null);
            mProgressDialog.show();
        }

        return Service.START_REDELIVER_INTENT;
    }

    @Override
    public void onDestroy() {
        if (mStorageManager != null) {
            mStorageManager.unregisterListener(mStorageListener);
        }
        if (mProgressDialog != null) {
            mProgressDialog.dismiss();
        }
        mWakeLock.release();
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCancel(DialogInterface dialog) {
        IMountService mountService = getMountService();
        String extStoragePath = mStorageVolume == null ?
                Environment.getExternalStorageDirectory().toString() :
                mStorageVolume.getPath();
        try {
            mountService.mountVolume(extStoragePath);
        } catch (RemoteException e) {
            Log.w(TAG, "Failed talking with mount service", e);
        }
	if((!mExternalStoragebeSD)
	    &&(extStoragePath.equals(Environment.getExternalStorageDirectory().toString()))){
	     Log.i(TAG, "cancel format on a internal sdcard volume,try mount external sdcard");
	     try {
	         mountService.mountVolume( Environment.getExternalStorage2Directory().toString());
	     } catch (RemoteException e) {
	         Log.w(TAG, "Failed talking with mount service when mount extsd2", e);
	     }
	 }	
        stopSelf();
    }

    void fail(int msg) {
        Toast.makeText(this, msg, Toast.LENGTH_LONG).show();
        if (mAlwaysReset) {
            sendBroadcast(new Intent("android.intent.action.MASTER_CLEAR"));
        }
        stopSelf();
    }

    void tryUnmountExternalStorage2()
    {
        final String extStoragePath = mStorageVolume == null ?
                Environment.getExternalStorageDirectory().toString() :
                mStorageVolume.getPath();
        mTryMountExtStorage2=false;
        if((!mExternalStoragebeSD)&&(extStoragePath.equals(Environment.getExternalStorageDirectory().toString())))
        {
    	    Log.i(TAG, "unmount external storage2 first!");
            String status = Environment.getExternalStorage2State();
            if (Environment.MEDIA_MOUNTED.equals(status)
                || Environment.MEDIA_MOUNTED_READ_ONLY.equals(status)) {
                
      		updateProgressDialog(R.string.progress_unmounting);
                IMountService mountService = getMountService();
        	String ext2StoragePath = Environment.getExternalStorage2Directory().toString();
                try {
        		mountService.unmountVolume(ext2StoragePath, true,false); 
                } catch (RemoteException e) {
                      Log.w(TAG, "Failed talking with mount service", e);
                }
                mTryMountExtStorage2=true;
             }
         }
     }

     void updateProgressState(String path,String newState) {
        if(mTryMountExtStorage2)
        {
            if(path==null)//wait for Storage2 status notify
               return;
            if(path.equals(Environment.getExternalStorage2Directory().toString()))
            {
               if((!Environment.MEDIA_UNMOUNTED.equals(newState) ) && 
                       (!Environment.MEDIA_REMOVED.equals(newState) ))
               {
                       return;//bypass all the state change of Storage2 until it unmonted 
               }
           }
       }		
       boolean isNand=false;
        String status = mStorageVolume == null ?
                Environment.getExternalStorageState() :
                mStorageManager.getVolumeState(mStorageVolume.getPath());
		if(mStorageVolume!=null&&mStorageVolume.getPath().equals("/mnt/flash")){
			isNand=true;
		}
        if (Environment.MEDIA_MOUNTED.equals(status)
                || Environment.MEDIA_MOUNTED_READ_ONLY.equals(status)) {
            if(isNand)
									updateProgressDialog(R.string.nand_unmounting);
						else
            updateProgressDialog(R.string.progress_unmounting);
            IMountService mountService = getMountService();
            final String extStoragePath = mStorageVolume == null ?
                    Environment.getExternalStorageDirectory().toString() :
                    mStorageVolume.getPath();
            try {
                // Remove encryption mapping if this is an unmount for a factory reset.
                mountService.unmountVolume(extStoragePath, true, mFactoryReset);
            } catch (RemoteException e) {
                Log.w(TAG, "Failed talking with mount service", e);
            }
        } else if (Environment.MEDIA_NOFS.equals(status)
                || Environment.MEDIA_UNMOUNTED.equals(status)
                || Environment.MEDIA_UNMOUNTABLE.equals(status)) {
            if(isNand)	updateProgressDialog(R.string.nand_erasing);
            else				updateProgressDialog(R.string.progress_erasing);
            
            final IMountService mountService = getMountService();
            final String extStoragePath = mStorageVolume == null ?
                    Environment.getExternalStorageDirectory().toString() :
                    mStorageVolume.getPath();
						final boolean nandRemove=isNand;
            if (mountService != null) {
                new Thread() {
                    @Override
                    public void run() {
                        boolean success = false;
                        try {
                            mountService.formatVolume(extStoragePath);

							String ext2StoragePath = Environment.getExternalStorage2Directory().toString();
							mountService.formatVolume(ext2StoragePath);
							
                            success = true;
                        } catch (Exception e) {
                        	if(nandRemove){
								Toast.makeText(ExternalStorageFormatter.this,
                                    R.string.nand_format_error, Toast.LENGTH_LONG).show();
							}else{
                            Toast.makeText(ExternalStorageFormatter.this,
                                    R.string.format_error, Toast.LENGTH_LONG).show();
                        }
                        }
                        if (success) {
                            if (mFactoryReset) {
                                sendBroadcast(new Intent("android.intent.action.MASTER_CLEAR"));
                                // Intent handling is asynchronous -- assume it will happen soon.
                                stopSelf();
                                return;
                            }
                        }
                        // If we didn't succeed, or aren't doing a full factory
                        // reset, then it is time to remount the storage.
                        if (!success && mAlwaysReset) {
                            sendBroadcast(new Intent("android.intent.action.MASTER_CLEAR"));
                        } else {
                            try {
                                mountService.mountVolume(extStoragePath);
                                if(mTryMountExtStorage2==true)
                                {
                                		final String extStoragePath2= Environment.getExternalStorage2Directory().toString();
                                		mountService.mountVolume(extStoragePath2);
                                }
                            } catch (RemoteException e) {
                                Log.w(TAG, "Failed talking with mount service", e);
                            }
                        }
                        stopSelf();
                        return;
                    }
                }.start();
            } else {
                Log.w(TAG, "Unable to locate IMountService");
            }
        } else if (Environment.MEDIA_BAD_REMOVAL.equals(status)) {
      				 if(isNand)	 fail(R.string.nand_bad_removal);
							 else			 fail(R.string.media_bad_removal);	
				           
        } else if (Environment.MEDIA_CHECKING.equals(status)) {
        			if(isNand)  fail(R.string.nand_checking);
							else  fail(R.string.media_checking);
        } else if (Environment.MEDIA_REMOVED.equals(status)) {
            if(isNand) fail(R.string.nand_removed);
						else fail(R.string.media_removed);
        } else if (Environment.MEDIA_SHARED.equals(status)) {
        		if(isNand) fail(R.string.nand_shared);
						else fail(R.string.media_shared);
        } else {
            fail(R.string.media_unknown_state);
            Log.w(TAG, "Unknown storage state: " + status);
            stopSelf();
        }
    }

    public void updateProgressDialog(int msg) {
        if (mProgressDialog == null) {
            mProgressDialog = new ProgressDialog(this);
            mProgressDialog.setIndeterminate(true);
            mProgressDialog.setCancelable(false);
            mProgressDialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
            mProgressDialog.show();
        }

        mProgressDialog.setMessage(getText(msg));
    }

    IMountService getMountService() {
        if (mMountService == null) {
            IBinder service = ServiceManager.getService("mount");
            if (service != null) {
                mMountService = IMountService.Stub.asInterface(service);
            } else {
                Log.e(TAG, "Can't get mount service");
            }
        }
        return mMountService;
    }
}