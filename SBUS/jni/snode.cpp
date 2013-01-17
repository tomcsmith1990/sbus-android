#include "sbusandroid.h"

extern "C" {

void
Java_uk_ac_cam_tcs40_sbus_SNode_packBoolean( JNIEnv* env,
                                             jobject thiz,
                                             jlong node,
                                             jboolean b,
                                             jstring n )
{	
	snode *sn;
	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, 0);
	
	if (b)
		sn = pack_bool(1, name);
	else
		sn = pack_bool(0, name);
		
	((snode *)node)->append(sn);	

	if (n != NULL) env->ReleaseStringUTFChars(n, name);
}

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

jboolean
Java_uk_ac_cam_tcs40_sbus_SNode_extractBoolean( JNIEnv* env,
                                             jobject thiz,
                                             jlong node,
                                             jstring n )
{	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, 0);
	
	int i = ((snode *)node)->extract_flg(name);	

	if (n != NULL) env->ReleaseStringUTFChars(n, name);
	
	if (i == 1)
		return JNI_TRUE;
	else
		return JNI_FALSE;
}

jint
Java_uk_ac_cam_tcs40_sbus_SNode_extractInt( JNIEnv* env,
                                             jobject thiz,
                                             jlong node,
                                             jstring n )
{	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, 0);
	
	int i = ((snode *)node)->extract_int(name);	

	if (n != NULL) env->ReleaseStringUTFChars(n, name);
	
	return i;
}

jdouble
Java_uk_ac_cam_tcs40_sbus_SNode_extractDouble( JNIEnv* env,
                                             jobject thiz,
                                             jlong node,
                                             jstring n )
{	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, 0);
	
	double d = ((snode *)node)->extract_dbl(name);	

	if (n != NULL) env->ReleaseStringUTFChars(n, name);
	
	return d;
}

jstring
Java_uk_ac_cam_tcs40_sbus_SNode_extractString( JNIEnv* env,
                                             jobject thiz,
                                             jlong node,
                                             jstring n )
{	
	const char *name = (n == NULL) ? NULL : env->GetStringUTFChars(n, 0);
	
	const char *s = ((snode *)node)->extract_txt(name);
			
	if (n != NULL) env->ReleaseStringUTFChars(n, name);
	
	return env->NewStringUTF(s);	
}

}
