#include "sbusandroid.h"

extern "C" {

jstring
Java_uk_ac_cam_tcs40_sbus_SEndpoint_map( JNIEnv* env,
				                                 jobject thiz, 
			                                     jlong endpoint,
			                                     jstring addr,
			                                     jstring ept )
{
	const char *address = env->GetStringUTFChars(addr, NULL);
	const char *endpt = env->GetStringUTFChars(ept, NULL);
	
	const char *s = ((sendpoint *)endpoint)->map(address, endpt);
	
	env->ReleaseStringUTFChars(addr, address);
	env->ReleaseStringUTFChars(ept, endpt);
	
	if (s == NULL) return NULL;
	else return env->NewStringUTF(s);
}

void
Java_uk_ac_cam_tcs40_sbus_SEndpoint_unmap( JNIEnv* env,
				                                   jobject thiz,
			                                       jlong endpoint )
{
	((sendpoint *)endpoint)->unmap();
}

jlong
Java_uk_ac_cam_tcs40_sbus_SEndpoint_createMessage( JNIEnv* env,
	                                               jobject thiz,
	                                               jlong endpoint,
	                                               jstring name )
{
	const char *msg_type = env->GetStringUTFChars(name, NULL);
	
	snode *message;
	message = mklist(msg_type);
	
	env->ReleaseStringUTFChars(name, msg_type);
	
	return (long)message;
}


jstring
Java_uk_ac_cam_tcs40_sbus_SEndpoint_emit( JNIEnv* env,
                                          jobject thiz,
                                          jlong endpoint,
                                          jlong message )
{
	((sendpoint *)endpoint)->emit(((snode *)message));
	
	const char *xml = ((snode *)message)->toxml(1);
			
	return env->NewStringUTF(xml);
}

jlong
Java_uk_ac_cam_tcs40_sbus_SEndpoint_receive( JNIEnv* env,
                                         	 jobject thiz,
                                         	 jlong endpoint)
{
	smessage *message;
	// blocks until it receives a message.
	message = ((sendpoint *)endpoint)->rcv();
	return (long)message;
}

}
