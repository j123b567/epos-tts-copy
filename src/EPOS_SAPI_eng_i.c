/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Thu Mar 29 14:03:08 2007
 */
/* Compiler settings for D:\work\epos\epos-2.5.37.S\src\EPOS_SAPI_eng.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )
#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

const IID LIBID_EPOS_SAPI_ENGLib = {0xD1FFC3E4,0x1098,0x445F,{0x9D,0x2B,0x6C,0xAE,0x31,0x2D,0x61,0x23}};


const CLSID CLSID_TTS_Engine = {0x1909197A,0x5D63,0x4896,{0xBC,0xC6,0x19,0x8C,0xD6,0x41,0x1F,0x09}};


#ifdef __cplusplus
}
#endif

