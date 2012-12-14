// TTS_Engine.h : Declaration of the CTTS_Engine

#ifndef __TTS_ENGINE_H_
#define __TTS_ENGINE_H_


#ifndef __TTS_Engine_FWD_DEFINED__
#include "EPOS_SAPI_Eng.h"
#endif

#ifndef SPDDKHLP_h
#include <spddkhlp.h>
#endif

#ifndef SPCollec_h
#include <spcollec.h>
#endif

#include "resource.h"       // main symbols

//OBECNE
void swide_to_schar(wchar_t *wchar, char *text, int num);
void unicode_translate(__int16 wchar, char *text);

SPSTREAMFORMAT get_format_idx(char *format);

/////////////////////////////////////////////////////////////////////////////
// CTTS_Engine
class ATL_NO_VTABLE CTTS_Engine : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTTS_Engine, &CLSID_TTS_Engine>,
	public ISpTTSEngine,
    public ISpObjectWithToken
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_TTS_ENGINE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTTS_Engine)
	COM_INTERFACE_ENTRY(ISpTTSEngine)
	COM_INTERFACE_ENTRY(ISpObjectWithToken)
END_COM_MAP()

 /*=== Methods ===*/
  public:
	HRESULT FinalConstruct();
	void FinalRelease();

 /*=== Interfaces ====*/
  public:
    //--- ISpObjectWithToken ----------------------------------
    STDMETHODIMP SetObjectToken( ISpObjectToken * pToken );
    STDMETHODIMP GetObjectToken( ISpObjectToken ** ppToken )
        { return SpGenericGetObjectToken( ppToken, m_cpToken ); }


    //--- ISpTTSEngine --------------------------------------------
    STDMETHOD(Speak)( DWORD dwSpeakFlags,
                      REFGUID rguidFormatId, const WAVEFORMATEX * pWaveFormatEx,
                      const SPVTEXTFRAG* pTextFragList, ISpTTSEngineSite* pOutputSite );
	STDMETHOD(GetOutputFormat)( const GUID * pTargetFormatId, const WAVEFORMATEX * pTargetWaveFormatEx,
								GUID * pDesiredFormatId, WAVEFORMATEX ** ppCoMemDesiredWaveFormatEx );


// ITTS_Engine
public:
private:
	int interrupt();
	int initialize();

	HRESULT hr;
    CComPtr<ISpObjectToken> m_cpToken;
};

#endif //__TTS_ENGINE_H_
