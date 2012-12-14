/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Thu Mar 29 14:03:08 2007
 */
/* Compiler settings for D:\work\epos\epos-2.5.37.S\src\EPOS_SAPI_eng.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __EPOS_SAPI_eng_h__
#define __EPOS_SAPI_eng_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __TTS_Engine_FWD_DEFINED__
#define __TTS_Engine_FWD_DEFINED__

#ifdef __cplusplus
typedef class TTS_Engine TTS_Engine;
#else
typedef struct TTS_Engine TTS_Engine;
#endif /* __cplusplus */

#endif 	/* __TTS_Engine_FWD_DEFINED__ */


/* header files for imported files */
#include "sapiddk.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __EPOS_SAPI_ENGLib_LIBRARY_DEFINED__
#define __EPOS_SAPI_ENGLib_LIBRARY_DEFINED__

/* library EPOS_SAPI_ENGLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_EPOS_SAPI_ENGLib;

EXTERN_C const CLSID CLSID_TTS_Engine;

#ifdef __cplusplus

class DECLSPEC_UUID("1909197A-5D63-4896-BCC6-198CD6411F09")
TTS_Engine;
#endif
#endif /* __EPOS_SAPI_ENGLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
