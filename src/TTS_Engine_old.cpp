// TTS_Engine.cpp : Implementation of CTTS_Engine
#include "stdafx.h"
#include "EPOS_SAPI_eng.h"
#include "TTS_Engine.h"
#include "sapi_client.h"
#include "client.cpp"
#include "say_client.h"
#include "common.h"
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
	int size = strstr(units, ":") - units;
	bool out;
	do {
		size = strstr(units, ":") - units;
		out = strncmp(text, units, size);
		units += size+1;
	}
	while (out && *units);
	return out;
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

	if (WSAStartup(MAKEWORD(2,0), (LPWSADATA)scratch)) shriek(464, "No winsock");
	charset = "cp1250";

	/*if (!start_service()) {
		report("Error", "EPOS system has not been started (maybe not installed?)\n");
		return S_FALSE;
	}*/

	ctrld = connect_socket(0, 0);
	datad = connect_socket(0, 0);
	send_to_epos("data ", datad);
	ch = get_handle(ctrld);
	send_to_epos(ch, datad);
	send_to_epos("\r\n", datad);
	free(ch);
	send_to_epos("setl charset ",ctrld);
	send_to_epos(charset, ctrld);
	send_to_epos("\r\n", ctrld);
	get_result(ctrld);
	//send_cmd_line(argc, argv);
	dh = get_handle(datad);
	get_result(datad);

	char *data;
	data = (char*)calloc(dstrFilePath.Length()+1, sizeof(*data));
	swide_to_schar((unsigned short*) dstrFilePath.m_psz, data);

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
	swide_to_schar((unsigned short*) dstr.m_psz, data);

	//return SpConvertStreamFormatEnum(SPSF_16kHz16BitMono, pDesiredFormatId, ppCoMemDesiredWaveFormatEx);
	return SpConvertStreamFormatEnum(get_format_idx(data), pDesiredFormatId, ppCoMemDesiredWaveFormatEx);	
}

STDMETHODIMP CTTS_Engine::Speak(DWORD dwSpeakFlags,
							 REFGUID rguidFormatId,
							 const WAVEFORMATEX * pWaveFormatEx,
							 const SPVTEXTFRAG* pTextFragList,
							 ISpTTSEngineSite* pOutputSite ){
	SPDBG_FUNC( "CTTSEngObj::Speak" );

	char *data, *hold, *wave, *txt, *buf, *p, *start, *end;
	int	i, n, e, play;
	wave_header *header;

	//uprava textu
	data = (char*)calloc(pTextFragList->ulTextLen+1, sizeof(*data));
	swide_to_schar((unsigned short*)pTextFragList->pTextStart, data);
	//_strlwr( data );
	for( buf = data; *buf; buf++) 
		if (*buf == '\n' || *buf == '\r') *buf=' ';
	if (data && strspn(data, ",.!?:;+=-*/&^%#$_<>{}()[]|\\~`' \04\t\"") == strlen(data))
		shriek("Input text too funny");

	// SET SPEECH RATE
	__int32 Rate;
	char	buff[30] = {0};
	if (S_OK != pOutputSite->GetRate((long*) &Rate))
		shriek("Unable to get speech rate");
	itoa(100 - 7*Rate, buff, 10);
	xmit_option("init_t", buff, ctrld);
	if (get_result(ctrld)-2)
		shriek("Unable to set speech rate");
	
	// SET F0
	//pTextFragList->State.PitchAdj.MiddleAdj

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
	get_result(ctrld);
	p = txt = get_data();
	
	while (*p) {
		p += strcspn(p, "I");
		*p = ' ';
	}

	send_to_epos("strm $", ctrld);
	send_to_epos(dh, ctrld);
	send_to_epos(":raw:rules:diphs:synth:", ctrld);
	send_to_epos("$", ctrld), send_to_epos(dh, ctrld);
	send_to_epos("\r\n", ctrld);
	send_to_epos("appl ", ctrld);
	sprintf(scratch, "%d", (int)strlen(data));
	send_to_epos(scratch, ctrld);
	send_to_epos("\r\n", ctrld);
	if (!(pOutputSite->GetActions() & SPVES_ABORT) )
		send_to_epos(data, datad);
	if (get_result(ctrld) > 2) shriek("Could not set up a stream");
	if (!(pOutputSite->GetActions() & SPVES_ABORT) ) 
		hold = wave = get_data();
	header = (wave_header*)wave;
	CueChunk *cue = (CueChunk*)(wave+sizeof(*header)+header->buffer_idx);
	ListHeader *list = (ListHeader*)&cue->points[cue->dwCuePoints];
	TextChunk *txtchnk = (TextChunk*)(list+1);

	CSpEvent Event;
	Event.eEventId				= SPEI_WORD_BOUNDARY;	//SPEI_SENTENCE_BOUNDARY; SPEI_WORD_BOUNDARY;SPEI_PHONEME
	Event.elParamType			= SPET_LPARAM_IS_UNDEFINED;	//SPET_LPARAM_IS_UNDEFINED;
	Event.ullAudioStreamOffset	= 0;
	Event.lParam				= 0;

	memset(buff, 0, 3);
	//hr = pOutputSite->Write( wave+sizeof(*header),	header->buffer_idx, NULL);
	int delka = 0;

	for ( buf = data, data = txt, start = wave+sizeof(*header), i = cue->dwCuePoints; i--; ){
		//e = strlen(data);

		clear_unit( hold = _strdup(txtchnk->dwText) );

		if ( *hold != '#' ){
			data = strpbrk(data-1, hold);
			e = strstr(hold, ":") - hold;
			data += e;
			if ( (*hold == 'O' || *hold == 'A' || *hold == 'E' ) && *data == 'u' && *(data+1) == 0x20)
				data++;
		}

		if (*data == 0x20 || 
			(*hold == 0x23 && *data == 0x23) || 
			(*data == 0x27 && *(data-1)==0x5E) ) { //ZDE JE CHYBA
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

	/*for (buf = data, start = wave+sizeof(*header), i = cue->dwCuePoints; i--; ){
		//e = strlen(data);

		p = hold = _strdup(txtchnk->dwText);
		while (*p) {
			p += strcspn(p, "?'@%^0");
			if (*p)
				memmove(p, p+1, strlen(p+1)+1);
		}

		if ( *hold == '#' ){
			// -2 je v play kvuli lupanci na konci wavka od machace
			play = *data == '.' && *(data+1) == 0 ? header->buffer_idx - (start-(wave+sizeof(*header))) -2 
				: 2*(cue->points[cue->dwCuePoints-i].dwSampleOffset - cue->points[cue->dwCuePoints-i-1].dwSampleOffset);
			hr = pOutputSite->Write( start,	play, NULL);
			txtchnk = (TextChunk*)((char*)&txtchnk->dwIdentifier + txtchnk->chunkSize);
			txtchnk = (TextChunk*)((char*)&txtchnk->dwIdentifier + txtchnk->chunkSize);
			if (txtchnk->dwText[1] != '#') 
				data++;
			else
				if (!*(data+1))
					break;
			continue;
		}
		data = strpbrk(data-1, hold);
		e = strstr(hold, ":") - hold;
		data += e;

		Event.ullAudioStreamOffset = 2*cue->points[cue->dwCuePoints-i-1].dwSampleOffset;
		Event.lParam               = data-buf;	// Offset zacatku Eventu
		Event.wParam               = e; // Delka Eventu (pocet hlasek)
		hr = pOutputSite->AddEvents( &Event, 1 );
		play = 2*cue->points[cue->dwCuePoints-i].dwSampleOffset - Event.ullAudioStreamOffset;
		hr = pOutputSite->Write( start,	play, NULL);
		start += play;
		//Event.wParam += 1;

		txtchnk = (TextChunk*)((char*)&txtchnk->dwIdentifier + txtchnk->chunkSize);
		txtchnk = (TextChunk*)((char*)&txtchnk->dwIdentifier + txtchnk->chunkSize);
	}
	/*
	for ( start = wave+sizeof(*header), i = cue->dwCuePoints; i--; ){ 
		/*for(n = 4, e, p = &buff[strlen(buff)]; --n; ){
			e = strcspn(&txtchnk->dwText[3-n], "%@^0?'#");
			*p++ =  e !=0 ? txtchnk->dwText[3-n] : 0;
			if ( !e )
				p--;
		}
		while ( strncmp(hold-1, buff, strlen(buff)) != NULL ){
			hold++;
		}
		get_word_bound(txtchnk, cue, hold = _strdup(data));
		strcat(buff, &txtchnk->dwText[1]);
		/*if ( *buff == '?' )
			memmove(buff, buff+1, strlen(buff));
		if (txt[strlen(buff)] == ' ') {
			strtok(data, " ");
			e = strlen(data);
			Event.ullAudioStreamOffset = cue->points[cue->dwCuePoints-i].dwSampleOffset;	//Offset audio bufferu
			Event.lParam               = Event.wParam;	// Offset zacatku Eventu
			Event.wParam               = e; // Delka Eventu (pocet hlasek)
			hr = pOutputSite->AddEvents( &Event, 1 );
			hr = pOutputSite->Write( start,	Event.ullAudioStreamOffset, NULL);
			start += Event.ullAudioStreamOffset;
			Event.wParam += 1;
			data += e+1;
			*buff = 0;
		}

		txtchnk = (TextChunk*)((char*)&txtchnk->dwIdentifier + txtchnk->chunkSize);
		txtchnk = (TextChunk*)((char*)&txtchnk->dwIdentifier + txtchnk->chunkSize);
	}
	// Add events*/

	return hr;
	free(data);
	free(txt);
	free(wave);
}

void swide_to_schar(wchar_t *wchar, char *text){
	//for (int i=0;*wchar;  );
	for (int i=0;*wchar; )
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
	case 0x0160: *text = 0x8A; break;	//#LATIN CAPITAL LETTER S WITH CARON
	case 0x0164: *text = 0x8D; break;	//#LATIN CAPITAL LETTER T WITH CARON
	case 0x017D: *text = 0x8E; break;	//#LATIN CAPITAL LETTER Z WITH CARON
	case 0x0161: *text = 0x9A; break;	//#LATIN SMALL LETTER S WITH CARON
	case 0x0165: *text = 0x9D; break;	//#LATIN SMALL LETTER T WITH CARON
	case 0x017E: *text = 0x9E; break;	//#LATIN SMALL LETTER Z WITH CARON
	case 0x010C: *text = 0xC8; break;	//#LATIN CAPITAL LETTER C WITH CARON
	case 0x011A: *text = 0xCC; break;	//#LATIN CAPITAL LETTER E WITH CARON
	case 0x0147: *text = 0xD2; break;	//#LATIN CAPITAL LETTER N WITH CARON
	case 0x0158: *text = 0xD8; break;	//#LATIN CAPITAL LETTER R WITH CARON
	case 0x016E: *text = 0xD9; break;	//#LATIN CAPITAL LETTER U WITH RING ABOVE
	case 0x010D: *text = 0xE8; break;	//#LATIN SMALL LETTER C WITH CARON
	case 0x011B: *text = 0xEC; break;	//#LATIN SMALL LETTER E WITH CARON
	case 0x010F: *text = 0xEF; break;	//#LATIN SMALL LETTER D WITH CARON
	case 0x0148: *text = 0xF2; break;	//#LATIN SMALL LETTER N WITH CARON
	case 0x0159: *text = 0xF8; break;	//#LATIN SMALL LETTER R WITH CARON
	case 0x016F: *text = 0xF9; break;	//#LATIN SMALL LETTER U WITH RING ABOVE
	default: wctomb(text, (wchar_t)wchar );
	}
}
