#include "sbusandroid.h"

extern "C" {

void
Java_uk_ac_cam_tcs40_sbus_SMessage_delete( JNIEnv* env,
                                          	jobject thiz,
                                          	jlong message)
{
	delete ((smessage *)message);
}

jlong
Java_uk_ac_cam_tcs40_sbus_SMessage_getTree( JNIEnv* env,
                                          	jobject thiz,
                                          	jlong message)
{
	snode *sn;
	sn = ((smessage *)message)->tree;
	return (long)sn;
}

}
