How to make an SBUS app:

- Add sbusandroid.jar (which contains uk.ac.cam.tcs40.sbus.SComponent/FileBootloader).
- In Order & Export, move it to the top and tick to export.
- Copy libsbusandroid.so to libs/armeabi
- Right click sbusandroid.jar and go to Properties - set Native Library to the libs folder.