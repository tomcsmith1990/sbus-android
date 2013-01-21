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

void
Java_uk_ac_cam_tcs40_sbus_Multiplex_delete( JNIEnv* env,
                                             jobject thiz,
                                             jlong multi )
{
	// Also deletes endpoints.
	delete ((multiplex *)multi);
}


}
