// TTS_Engine.cpp : Implementation of CTTS_Engine
#include "stdafx.h"
#include "EPOS_SAPI_eng.h"
#include "TTS_Engine.h"
#include "sapi_client.h"
#include "client.cpp"
#include "say_client.h"
#include "common.h"
#include "math.h"
//#include "install.cpp"
//#include "startsrv.cpp"

// say.cpp

#define ID char

typedef struct {
	ID		string1[4];
	int32_t total_length;
	ID		string2[8];
	int32_t fmt_length;
	int16_t datform, numchan;
	int32_t	sf1, avr1; 
	int16_t	alignment, samplesize;
	ID		string3[4];
	int32_t buffer_idx; 
} wave_header;	// .wav file header

typedef struct {
  long	dwIdentifier;
  long	dwPosition;
  ID	fccChunk[4];
  long	dwChunkStart;
  long	dwBlockStart;
  long	dwSampleOffset;
} CuePoint;

typedef struct {
  ID        chunkID[4];
  long      chunkSize;
  long      dwCuePoints;
  CuePoint  points[];
} CueChunk;

typedef struct {
  ID      listID[4];
  long    chunkSize;
  ID      typeID[4];
} ListHeader;

typedef struct {
  ID      chunkID[4];
  long    chunkSize;
  long    dwIdentifier;
  char    dwText[];
} TextChunk;

void clear_unit(char *p){
	while (*p) {
		p += strcspn(p, "?0");
		if (*p)
			memmove(p, p+1, strlen(p+1)+1);
	}
}

bool compare_unit(char *text, char *units){
	int size;
	int out;
	do {
		size = strstr(units, ":") - units;
		out = strncmp(text, units, size);
		units += size+1;
	}
	while (out && *units);
	return (bool)out;
}

HRESULT CTTS_Engine::FinalConstruct(){
	if (WSAStartup(MAKEWORD(2,0), (LPWSADATA)scratch)) 
		return S_FALSE; // shriek(464, "No winsock");
	charset = "cp1250";
	return S_OK;
}

void CTTS_Engine::FinalRelease(){
	send_to_epos("delh ", ctrld);
	send_to_epos(dh, ctrld);
	send_to_epos("\r\ndone\r\n", ctrld);
	get_result(ctrld);
	get_result(ctrld);
	
	close(datad);
	close(ctrld);
}

int CTTS_Engine::initialize(){
	ctrld = connect_socket(0, 0);
	datad = connect_socket(0, 0);
	send_to_epos("data ", datad);
	ch = get_handle(ctrld);
	send_to_epos(ch, datad);
	send_to_epos("\r\n", datad);
	//free(ch);
	send_to_epos("setl charset ",ctrld);
	send_to_epos(charset, ctrld);
	send_to_epos("\r\n", ctrld);
	get_result(ctrld);
	dh = get_handle(datad);
	return get_result(datad)-2;
}

int CTTS_Engine::interrupt(){
	send_to_epos("delh ", ctrld);
	send_to_epos(dh, ctrld);
	send_to_epos("\r\n", ctrld);
	send_to_epos("intr ", ctrld);
	send_to_epos(ch, ctrld);
	send_to_epos("\r\n", ctrld);
	send_to_epos("\r\ndone\r\n", ctrld);

	close(datad);
	close(ctrld);

	if ( get_result(ctrld) != 2 )
		return S_FALSE;
	else {
		if ( initialize() )
			return S_FALSE;
	
		CSpDynamicString dstrFilePath;
		const WCHAR *pszTokenVal = L"Voice";
		hr = m_cpToken->GetStringValue( pszTokenVal, &dstrFilePath );

		char *data;
		data = (char*)calloc(dstrFilePath.Length()+1, sizeof(*data));
		swide_to_schar((unsigned short*) dstrFilePath.m_psz, data, 0);

		xmit_option("voice", data, ctrld);
		free(data);
		return get_result(ctrld)-2;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////
// CTTS_Engine

// implementace inicializovani daneho hlasu v EPOSu
STDMETHODIMP CTTS_Engine::SetObjectToken( ISpObjectToken * pToken ){
	
	SPDBG_FUNC( "CTTSEngObj::SetObjectToken" );
    hr = SpGenericSetObjectToken(pToken, m_cpToken);

	CSpDynamicString dstrFilePath;
	const WCHAR *pszTokenVal = L"Voice";
    hr = m_cpToken->GetStringValue( pszTokenVal, &dstrFilePath );

	if ( initialize() )
		return S_FALSE;
	char *data;
	data = (char*)calloc(dstrFilePath.Length()+1, sizeof(*data));
	swide_to_schar((unsigned short*) dstrFilePath.m_psz, data, 0);

	xmit_option("voice", data, ctrld);
	hr = get_result(ctrld)-2;
	free(data);

	return hr;
	
}


STDMETHODIMP CTTS_Engine::GetOutputFormat(const GUID * pTargetFormatId, 
									   const WAVEFORMATEX * pTargetWaveFormatEx,
                                       GUID * pDesiredFormatId, WAVEFORMATEX ** ppCoMemDesiredWaveFormatEx ){
	SPDBG_FUNC( "CTTSEngObj::GetOutputFormat" );

	CSpDynamicString dstr;
	const WCHAR *pszTokenVal = L"SampleFormat";
	
	hr = m_cpToken->GetStringValue( pszTokenVal, &dstr );

	char *data;
	data = (char*)calloc(dstr.Length()+1, sizeof(*data));
	swide_to_schar((unsigned short*) dstr.m_psz, data, 0);

	//return SpConvertStreamFormatEnum(SPSF_16kHz16BitMono, pDesiredFormatId, ppCoMemDesiredWaveFormatEx);
	return SpConvertStreamFormatEnum(get_format_idx(data), pDesiredFormatId, ppCoMemDesiredWaveFormatEx);
	free(data);
}

HRESULT XML_speak(SPVTEXTFRAG* pText, ISpTTSEngineSite* pOutputSite);

STDMETHODIMP CTTS_Engine::Speak(DWORD dwSpeakFlags,
							 REFGUID rguidFormatId,
							 const WAVEFORMATEX * pWaveFormatEx,
							 const SPVTEXTFRAG* pTextFragList,
							 ISpTTSEngineSite* pOutputSite ){
	SPDBG_FUNC( "CTTSEngObj::Speak" );

	char *data, *hold, *wave, *txt, *buf, *p, *start, *location, buff[30] = {0};
	int	i, e, play;
	bool wavout = false;
	wave_header *header;

	// PREPROCESS
	CSpDynamicString dstr;
	const WCHAR *pszTokenValOpt = L"Options", *pszTokenValF0 = L"F0";
	
		// GET INFORMATION ABOUT VOICE TYPE
	hr = m_cpToken->GetStringValue( pszTokenValOpt, &dstr );
	char *mbrl;
	mbrl = (char*)calloc(dstr.Length()+1, sizeof(*mbrl));
	swide_to_schar((unsigned short*) dstr.m_psz, mbrl, 0);
	dstr.Clear();
		// GET SPEECH F0
	hr = m_cpToken->GetStringValue( pszTokenValF0, &dstr );
	swide_to_schar((unsigned short*) dstr.m_psz, buff, 0);
	dstr.Clear();
	__int32 F0 = atoi(buff);

	CSpEvent Event;
	//Event.Clear();
	Event.eEventId				= SPEI_WORD_BOUNDARY;	//SPEI_SENTENCE_BOUNDARY; SPEI_WORD_BOUNDARY;SPEI_PHONEME
	Event.elParamType			= SPET_LPARAM_IS_UNDEFINED;	//SPET_LPARAM_IS_UNDEFINED;
	Event.ullAudioStreamOffset	= 0;
	Event.lParam				= 0;
	Event.wParam				= 0;

	// SYNTHESIZE
	for(const SPVTEXTFRAG *pText = pTextFragList; pText != NULL; pText = pText->pNext){
		
		//uprava textu
		data = (char*)calloc(pText->ulTextLen+10, sizeof(*data)); // +10 - place for quotation marks
		swide_to_schar((unsigned short*)pText->pTextStart, data, pText->ulTextLen);
		_strlwr( data );

		for( buf = data; *buf; buf++) {
			if (*buf == '\n' || *buf == '\r') *buf=' ';
		}
		if (data && strspn(data, ",.!?:;+=-*/&^%#$_<>{}()[]|\\~`' \04\t\"") == strlen(data))
			shriek("Input text too funny");
		
		wavout = strstr(data, "--wav") ? true : false;
		if (wavout && (buf = strstr(data, "\"")) ){
			i = strcspn(buf+1, "\"");
			location = (char*)calloc(i+10, 1);

			strncpy(location, buf+1, i);
			data = strstr(buf+1, "\"")+1;
			buf = location;
			while ( (buf = strstr(buf, "\\")) ){
				if (buf[1] != '\\')
					memmove(buf+1, buf, strlen(buf)+1);
				buf +=2;
			}
		}
		else
			location = 0;
		
		// SET SPEECH F0, SAPI - incrementing or decrementing by 1 is multiplying or dividing the rate by the 10th root of 3 (about 1.1). 
		itoa( (int)(F0 * ( (pText->State.PitchAdj.MiddleAdj || pText->State.PitchAdj.RangeAdj) ? 
			pow(1.03, pText->State.PitchAdj.MiddleAdj) * pow(1.1, pText->State.PitchAdj.RangeAdj) : 1) ), buff, 10);
		xmit_option("init_f", buff, ctrld);
		if (get_result(ctrld)-2)
			shriek("Unable to set speech rate");
		memset(buff, 0, 5);

		// SET SPEECH RATE
		__int32 Rate;
		if ( S_OK != pOutputSite->GetRate((long*) &Rate) )
			shriek("Unable to get speech rate");
		itoa(100 - 5*Rate, buff, 10);
		xmit_option("init_t", buff, ctrld);
		if (get_result(ctrld)-2)
			shriek("Unable to set speech rate");

		// print text with applicated rules
		send_to_epos("strm $", ctrld);
		send_to_epos(dh, ctrld);
		send_to_epos(":raw:rules:print:$", ctrld);
		send_to_epos(dh, ctrld);
		send_to_epos("\r\n", ctrld);
		send_to_epos("appl ", ctrld);
		sprintf(scratch, "%d", (int)strlen(data));
		send_to_epos(scratch, ctrld);
		send_to_epos("\r\n", ctrld);
		send_to_epos(data, datad);
		if ( pOutputSite->GetActions() & SPVES_ABORT )
			return (HRESULT)interrupt();
		get_result(ctrld);
		txt = get_data();

		// MBROLA SYNTHESIS
		buf = data;
		if (*mbrl){
			send_to_epos("strm $", ctrld);
			send_to_epos(dh, ctrld);
			send_to_epos(":raw:rules:dump:$", ctrld);
			send_to_epos(dh, ctrld);
			send_to_epos("\r\n", ctrld);
			send_to_epos("appl ", ctrld);
			sprintf(scratch, "%d", (int)strlen(data));
			send_to_epos(scratch, ctrld);
			send_to_epos("\r\n", ctrld);
			send_to_epos(data, datad);
			if ( pOutputSite->GetActions() & SPVES_ABORT )
				return (HRESULT)interrupt();
			if (get_result(ctrld) > 2) shriek("Could not set up a stream");
			char	*b = get_data();
			
			if (!strcmp(mbrl, "units")){		/// Puvodni podminka ?? if (strlen(scratch) > 20){	
				send_to_epos("strm $", ctrld);
				send_to_epos(dh, ctrld);
				send_to_epos(":raw:rules:diphs:$", ctrld);
				send_to_epos(dh, ctrld);
				send_to_epos("\r\n", ctrld);
				send_to_epos("appl ", ctrld);
				sprintf(scratch, "%d", (int)strlen(data));
				send_to_epos(scratch, ctrld);
				send_to_epos("\r\n", ctrld);
				send_to_epos(data, datad);
				if ( pOutputSite->GetActions() & SPVES_ABORT )
					return (HRESULT)interrupt();
				if (get_result(ctrld) > 2) shriek("Could not set up a stream");
				segment *segs = (segment *)get_data();
				
				int i = 0;
				char *mmry = (char*)calloc(strlen(b)+segs->code*5, 1);
				*mmry = ';';

				itoa(segs->code, mmry+1, 10);
				for (i = 1; i < segs->code; i++){
					mmry[strlen(mmry)] = ',';
					itoa(segs[i].code, mmry+strlen(mmry), 10);
					
				}
				mmry[strlen(mmry)] = 0x0A;
				memmove(mmry+strlen(mmry), b, strlen(b)+1);
				free(data);
				data = mmry;
			}
			else {
				free(data);
				data = b;
			}
		}
		send_to_epos("strm $", ctrld);
		send_to_epos(dh, ctrld);
		if (*mbrl)
			send_to_epos(":syn:$", ctrld);
		else
			send_to_epos(":raw:rules:diphs:synth:$", ctrld);
		send_to_epos(dh, ctrld);
		send_to_epos("\r\n", ctrld);
		send_to_epos("appl ", ctrld);
		sprintf(scratch, "%d", (int)strlen(data));
		send_to_epos(scratch, ctrld);
		send_to_epos("\r\n", ctrld);
		send_to_epos(data, datad);

		if (get_result(ctrld) > 2) shriek("Could not set up a stream");
		hold = wave = get_data();

		if ( pOutputSite->GetActions() & SPVES_ABORT )
			return (HRESULT)interrupt();

		header = (wave_header*)wave;
		CueChunk *cue = (CueChunk*)(wave+sizeof(*header)+header->buffer_idx);
		ListHeader *list = (ListHeader*)&cue->points[cue->dwCuePoints];
		TextChunk *txtchnk = (TextChunk*)(list+1);

		if (!strcmp(mbrl, "units")) {
			hr = pOutputSite->Write( wave+sizeof(*header),	header->buffer_idx, NULL);

			FILE *f;
			if (wavout) { 
				if (location && !(*location) ){
					free(location);
					location = _strdup("c:\\output.wav");
				}
				location = !location ? _strdup("c:\\output.wav") : location;

				f = fopen(location, "wb");
				
				if (!size || !wave) shriek("Could not get waveform");
				if (!f || !fwrite(wave, size, 1, f)) 
					shriek("Could not write waveform");
				else
					fclose(f);
			}
			free(location);
			free(data);
			free(wave);
			continue;
			//return hr;
		}

		memset(buff, 0, 3);
		int delka = 0;

		// Text preparation -- spojka "a" - not working properly
		txt = (char*)realloc(txt, strlen(txt) + 100); // uprava pro odmezerovani spojky "a" - nebude fungovat vzdy
		p = txt;
		while (*p++) {
			p += strcspn(p, "I'");
			if (*p)
				if (*p == 'I' && *(p-1) == 'a')
					*p = ' ';
				else if (*(p-1) != ' ' && *(p-1) == 'a') {
					memmove(p+1, p, strlen(p)+1);
					*p = ' ';
				}
		}
		free(data);

		for ( data = txt, start = wave+sizeof(*header), i = cue->dwCuePoints; i--; ){
			//e = strlen(data);

			clear_unit( hold = _strdup(txtchnk->dwText) );

			if ( *hold != '#' ){
				data = strpbrk(data-1, hold);
				e = strstr(hold, ":") - hold;
				data += e;
				if ( (*hold == 'O' || *hold == 'A' || *hold == 'E' ) && *data == 'u' && *(data+1) == 0x20)
					data++;
			}

			if (*data == 0x20 || (*hold == 0x23 && *data == 0x23) || (*data == 0x27 && *(data-1)==0x5E) ) {
				i++;
				if (*data != 0x23 ) {
					if (*data != 0x27)
						memmove(data, data+1, strlen(data));
					if ( *data == 0x27 && strspn(data-1, "@%^OAE") )
						data++;
					while ( compare_unit(data-1, hold) || (e < 2))
					{
						txtchnk = (TextChunk*)((char*)&txtchnk->dwIdentifier + txtchnk->chunkSize);
						txtchnk = (TextChunk*)((char*)&txtchnk->dwIdentifier + txtchnk->chunkSize);
						clear_unit( hold = _strdup(txtchnk->dwText) );
						e = strstr(hold, ":") - hold;
						i--;
					}
					
					Event.eEventId             = SPEI_WORD_BOUNDARY;		//SPEI_SENTENCE_BOUNDARY;
					Event.elParamType          = SPET_LPARAM_IS_UNDEFINED;	//SPET_LPARAM_IS_UNDEFINED;
					Event.wParam               = strlen(strtok(buf, " #."));	// Delka Eventu (pocet hlasek)
					hr = pOutputSite->AddEvents( &Event, 1 );
					
					buf							+= Event.wParam+1;
					Event.lParam				+= Event.wParam+1;	// Offset zacatku Eventu
				}
				else {
					do {
						if (!(--i -1)){
							i--;
							break;

						}
						txtchnk = (TextChunk*)((char*)&txtchnk->dwIdentifier + txtchnk->chunkSize);
						txtchnk = (TextChunk*)((char*)&txtchnk->dwIdentifier + txtchnk->chunkSize);
						clear_unit( hold = _strdup(txtchnk->dwText) );
						data++;
					}
					while (*hold == 0x23);
					Event.wParam = strlen(strtok(buf, ",.?!"));
					hr = pOutputSite->AddEvents( &Event, 1 );
					
					buf	+= Event.wParam+1;
					buf += *buf == 0x20 ? 1 : 0;

					Event.lParam += Event.wParam+1;
					Event.lParam += *(buf-1) == 0x20 ? 1 : 0;

					data = strpbrk(data, ".,");
				}
				
				play = 2*cue->points[cue->dwCuePoints-i].dwSampleOffset - Event.ullAudioStreamOffset;
				if (i)
					hr = pOutputSite->Write( start,	play, NULL);
				else
					hr = pOutputSite->Write( start,	header->buffer_idx - Event.ullAudioStreamOffset-2, NULL);
				
				Event.ullAudioStreamOffset = 2*cue->points[cue->dwCuePoints-i].dwSampleOffset;
				start += play;
			}
			else {
				txtchnk = (TextChunk*)((char*)&txtchnk->dwIdentifier + txtchnk->chunkSize);
				txtchnk = (TextChunk*)((char*)&txtchnk->dwIdentifier + txtchnk->chunkSize);
			}
			free (hold);
		}

		//free(data);
		if (!strcmp(mbrl, "units")) {
			free(txt);
			free(wave);
		}
	}
	if (*mbrl)
		free(mbrl);
	return hr;
}

void swide_to_schar(wchar_t *wchar, char *text, int num){
	//for (int i=0;*wchar;  );
	for (int i=0;i++ < num || (*wchar && !num); )
		unicode_translate((__int16)*wchar++, text++);
}
// Ahoj krásko, žluouèký šalamoun je zde.

SPSTREAMFORMAT get_format_idx(char *format){
	/*
	SPSF_Default	= -1,
	SPSF_NoAssignedFormat	= 0,
	SPSF_Text	= SPSF_NoAssignedFormat + 1,
	SPSF_NonStandardFormat	= SPSF_Text + 1,
	SPSF_ExtendedAudioFormat	= SPSF_NonStandardFormat + 1,
	SPSF_8kHz8BitMono	= SPSF_ExtendedAudioFormat + 1, //4
	*/

	int number;
	switch (atoi(strtok(format, ":"))){
	case 8: number = 4; break;
	case 11: number = 8; break;
	case 12: number = 12; break;
	case 16: number = 16; break;
	case 22: number = 20; break;
	case 24: number = 24; break;
	case 32: number = 28; break;
	case 44: number = 32; break;
	case 48: number = 36; break;
	}

	number += *strtok(format += strlen(format)+1, ":") == '1' ? 2 : 0;
	number += *strtok(format += strlen(format)+1, ":") == 'S' ? 2 : 0;

	return (SPSTREAMFORMAT)number;
}

void unicode_translate(__int16 wchar, char *text){ // UNICODE => WINCP1250
	switch(wchar){
	case 0x0160: *text = (char)0x8A; break;	//#LATIN CAPITAL LETTER S WITH CARON
	case 0x0164: *text = (char)0x8D; break;	//#LATIN CAPITAL LETTER T WITH CARON
	case 0x017D: *text = (char)0x8E; break;	//#LATIN CAPITAL LETTER Z WITH CARON
	case 0x0161: *text = (char)0x9A; break;	//#LATIN SMALL LETTER S WITH CARON
	case 0x0165: *text = (char)0x9D; break;	//#LATIN SMALL LETTER T WITH CARON
	case 0x017E: *text = (char)0x9E; break;	//#LATIN SMALL LETTER Z WITH CARON
	case 0x010C: *text = (char)0xC8; break;	//#LATIN CAPITAL LETTER C WITH CARON
	case 0x011A: *text = (char)0xCC; break;	//#LATIN CAPITAL LETTER E WITH CARON
	case 0x0147: *text = (char)0xD2; break;	//#LATIN CAPITAL LETTER N WITH CARON
	case 0x0158: *text = (char)0xD8; break;	//#LATIN CAPITAL LETTER R WITH CARON
	case 0x016E: *text = (char)0xD9; break;	//#LATIN CAPITAL LETTER U WITH RING ABOVE
	case 0x010D: *text = (char)0xE8; break;	//#LATIN SMALL LETTER C WITH CARON
	case 0x011B: *text = (char)0xEC; break;	//#LATIN SMALL LETTER E WITH CARON
	case 0x010F: *text = (char)0xEF; break;	//#LATIN SMALL LETTER D WITH CARON
	case 0x0148: *text = (char)0xF2; break;	//#LATIN SMALL LETTER N WITH CARON
	case 0x0159: *text = (char)0xF8; break;	//#LATIN SMALL LETTER R WITH CARON
	case 0x016F: *text = (char)0xF9; break;	//#LATIN SMALL LETTER U WITH RING ABOVE
	default: wctomb(text, (wchar_t)wchar );
	}
}
