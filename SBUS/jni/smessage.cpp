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

jstring
Java_uk_ac_cam_tcs40_sbus_SMessage_getSourceComponent( JNIEnv* env,
						                              	jobject thiz,
						                              	jlong message)
{
	const char *source_cpt;
	source_cpt = ((smessage *)message)->source_cpt;
	return env->NewStringUTF(source_cpt);
}

jstring
Java_uk_ac_cam_tcs40_sbus_SMessage_getSourceInstance( JNIEnv* env,
						                              	jobject thiz,
						                              	jlong message)
{
	const char *source_inst;
	source_inst = ((smessage *)message)->source_inst;
	return env->NewStringUTF(source_inst);
}

jstring
Java_uk_ac_cam_tcs40_sbus_SMessage_getSourceEndpoint( JNIEnv* env,
						                              	jobject thiz,
						                              	jlong message)
{
	const char *source_ep;
	source_ep = ((smessage *)message)->source_ep;
	return env->NewStringUTF(source_ep);
}

}
