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

void
Java_uk_ac_cam_tcs40_sbus_SEndpoint_setAutomapPolicy( JNIEnv* env,
									                   jobject thiz,
								                       jlong endpoint,
								                       jstring addr,
								                       jstring ept )
{
	const char *address = env->GetStringUTFChars(addr, NULL);
	const char *endpt = (ept == NULL) ? NULL : env->GetStringUTFChars(ept, NULL);
	
	((sendpoint *)endpoint)->set_automap_policy(address, endpt);
	
	env->ReleaseStringUTFChars(addr, address);
	if (ept != NULL) env->ReleaseStringUTFChars(ept, endpt);
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

jlong
Java_uk_ac_cam_tcs40_sbus_SEndpoint_reply( JNIEnv* env,
                                         	 jobject thiz,
                                         	 jlong endpoint,
                                         	 jlong query,
                                         	 jlong reply)
{
	((sendpoint *)endpoint)->reply((smessage *)query, (snode *)reply);
}

jlong
Java_uk_ac_cam_tcs40_sbus_SEndpoint_rpc( JNIEnv* env,
                                         	 jobject thiz,
                                         	 jlong endpoint,
                                         	 jlong query)
{
	snode *sn;
	if (query == 0)
		sn = NULL;
	else
		sn = ((snode *)query);
		
	smessage *message;
	message = ((sendpoint *)endpoint)->rpc(sn);
	
	if (message == NULL)
		return 0;
	else
		return (long)message;
}

}
