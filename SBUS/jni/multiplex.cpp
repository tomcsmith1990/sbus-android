#include "sbusandroid.h"

extern "C" {

jlong
Java_uk_ac_cam_tcs40_sbus_Multiplex_multiplex( JNIEnv* env,
                                               jobject thiz )
{
	return (long) (new multiplex());
}

void
Java_uk_ac_cam_tcs40_sbus_Multiplex_add( JNIEnv* env,
                                         jobject thiz,
                                         jlong multi,
                                         jlong endpoint )
{
	((multiplex *)multi)->add((sendpoint *)endpoint);
}

jlong
Java_uk_ac_cam_tcs40_sbus_Multiplex_waitForMessage(	 JNIEnv* env,
						                             jobject thiz,
						                             jlong multi,
						                             jlong component )
{
	int fd = ((multiplex *)multi)->wait();
	if (fd < 0) return -1;
	
	sendpoint *ept;
	ept = ((scomponent *)component)->fd_to_endpoint(fd);
	
	return (long)ept;
}

void
Java_uk_ac_cam_tcs40_sbus_Multiplex_delete( JNIEnv* env,
                                             jobject thiz,
                                             jlong multi )
{
	// Also deletes endpoints.
	delete ((multiplex *)multi);
}


}
