#include "sbusandroid.h"

extern "C" {

void
Java_uk_ac_cam_tcs40_sbus_SNode_packInt( JNIEnv* env,
                                             jobject thiz,
                                             jlong node,
                                             jint i,
                                             jstring n )
{	
	snode *sn;
	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, 0);
	
	sn = pack(i, name);
	((snode *)node)->append(sn);	

	if (n != NULL) env->ReleaseStringUTFChars(n, name);
}

void
Java_uk_ac_cam_tcs40_sbus_SNode_packDouble( JNIEnv* env,
                                             jobject thiz,
                                             jlong node,
                                             jdouble d,
                                             jstring n )
{	
	snode *sn;
	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, 0);
	
	sn = pack(d, name);
	((snode *)node)->append(sn);	

	if (n != NULL) env->ReleaseStringUTFChars(n, name);
}

void
Java_uk_ac_cam_tcs40_sbus_SNode_packString( JNIEnv* env,
                                                jobject thiz,
                                                jlong node,
                                                jstring s,
                                                jstring n )
{
	snode *sn;
	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, 0);
	const char *string = env->GetStringUTFChars(s, 0);

	sn = pack(string, name);
	((snode *)node)->append(sn);
	
	env->ReleaseStringUTFChars(s, string);
	if (n != NULL) env->ReleaseStringUTFChars(n, name);
}

void
Java_uk_ac_cam_tcs40_sbus_SNode_packClock( JNIEnv* env,
                                               jobject thiz,
                                               jlong node,
                                               jstring c,
                                               jstring n )
{
	snode *sn;
	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, 0);
	const char *clk = env->GetStringUTFChars(c, 0);

	sdatetime datetime(clk);
	
	sn = pack(&datetime, name);
			
	((snode *)node)->append(sn);	
	
	env->ReleaseStringUTFChars(c, clk);
	if (n != NULL) env->ReleaseStringUTFChars(n, name);
}

void
Java_uk_ac_cam_tcs40_sbus_SNode_delete( JNIEnv* env,
                                             jobject thiz,
                                             jlong node )
{
	delete ((snode *)node);
}

}
