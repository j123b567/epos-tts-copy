/* SAPI DEFINITIONS */
#include "sapi.h"

void define_sapi_tokens(){
	
	HKEY	hkey;
	DWORD	msg, lng;
	char	*lpSubKey = (char*)calloc(80,1), *active_voice, *data, buff[40] = {0},
			*sapi_token_info1[] = {"", "405", "Voice", "CLSID", "SampleFormat", "Options", "F0", ":"},
			*sapi_token_info2[] = {"Age", "Gender", "Language", "Name", "Vendor", ":"},
            *sapi_token_value2[] = {"Adult", "Male", "L", "V", "EPOS, ÚFE AV ÈR", ":"};
	int		i, n, k = 0;

    for ( int j = cfg->n_langs; j--; )
    {
        switch ( *cfg->langs[j]->name )
        {
        case 'e':
            lng = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
            break;
        case 'g':
            lng = MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN);
            break;
        case 's':
            lng = MAKELANGID(LANG_SLOVAK, SUBLANG_SLOVAK_SLOVAKIA);
            break;
        default:
            lng = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
        }
        for ( i = n = cfg->langs[j]->n_voices; i--; )
        { //i = n = cfg->langs[j]->n_voices
            //if (!cfg->langs[j]->voices[n-i-1]->out_rate)
            //continue;
            active_voice = (char *) cfg->langs[j]->voicetab[n - i - 1]->name;
            strcpy(lpSubKey, SAPI_TOKEN_SUBKEY);
            strcat(lpSubKey, active_voice);
            if (ERROR_SUCCESS != RegCreateKeyEx( SAPI_TOKEN_HKEY, (LPCTSTR)lpSubKey, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &msg))
                shriek(418, "Unable to register voices for SAPI"); // opravit nekde musi byt definice kdy zapnout sapi
            switch (msg)
            {
                case REG_CREATED_NEW_KEY: break;
                case REG_OPENED_EXISTING_KEY: break;
                default: shriek(418, "Chyba pri registraci SAPI tokenu");
            }

            if ( REG_CREATED_NEW_KEY == msg || REG_OPENED_EXISTING_KEY == msg) 
            {	//vytvori popis TOKEN
                for (k = 0, data = active_voice; *sapi_token_info1[k] != ':'; ) {
                    switch (k)
                    {
                    case 3:
                        data = SAPI_ENGINE_CLSID; break;
                    case 4:
                        if (cfg->langs[j]->voicetab[n - i - 1]->out_sampling_rate)
                            itoa(cfg->langs[j]->voicetab[n - i - 1]->out_sampling_rate/1000, buff, 10);
                        else
                            itoa(cfg->langs[j]->voicetab[n - i - 1]->inv_sampling_rate/1000, buff, 10);
                        strcat(buff, ":");
                        itoa(cfg->langs[j]->voicetab[n - i - 1]->sample_size, buff+strlen(buff), 10);
                        strcat(buff, ":");
                        
                        switch (cfg->langs[j]->voicetab[n - i - 1]->channel)
                        {
                        case CT_MONO: strcat(buff, "Mono"); break;
                        case CT_BOTH: strcat(buff, "Stereo"); break;
                        default:shriek(418, "Chyba pri registraci SAPI tokenu");
                        }
                        data = buff; 
                        break;
                    case 5: 
                        if (cfg->langs[j]->voicetab[n - i - 1]->mbrola_like)
                            strcpy(data, cfg->langs[j]->voicetab[n - i - 1]->mbrola_like);
                        else
                            strcpy(data, ""); 
                        break;
                    case 6:
                        itoa(cfg->langs[j]->voicetab[n - i - 1]->init_f, data, 10);
                        break;
                    }

                    if (ERROR_SUCCESS != RegSetValueEx( hkey, sapi_token_info1[k++], NULL, REG_SZ, (const unsigned char *) data, strlen(data)) )
                        shriek(418, "Chyba pri registraci SAPI tokenu");
                    }
            }
            else
                shriek(418, "Chyba pri registraci SAPI tokenu");

            //vytvori popis ATTRIBUTES
            strcat(lpSubKey, "\\Attributes");
            if (ERROR_SUCCESS != RegCreateKeyEx( SAPI_TOKEN_HKEY, (LPCTSTR)lpSubKey, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &msg))
                shriek(418, "Unable to register voices for SAPI"); // opravit nekde musi byt definice kdy zapnout sapi
            if ( REG_CREATED_NEW_KEY == msg ) 
            {
                for (k = 0; *sapi_token_info2[k] != ':'; ) 
                {
                    switch ( *sapi_token_value2[k] )
                    {                    
                    case 'L':
                        sprintf(buff, "%x", lng);
                        data = buff;
                        break;
                    case 'V':
                        data = active_voice;
                        break;
                    default:
                        data = sapi_token_value2[k];
                    }                        
                    if (ERROR_SUCCESS != RegSetValueEx( hkey, sapi_token_info2[k++], NULL, REG_SZ, (const unsigned char *) data, strlen(data)) )
                        shriek(418, "Chyba pri registraci SAPI tokenu");
                }
            }
        }
    }
}

void clear_sapi_tokens(){
	HKEY	hkey, hSubkey;
	DWORD	SubKey, SubSubKey,  SubValues, cbName = MAX_KEY_LENGTH;
	TCHAR    nameKey[MAX_KEY_LENGTH], allName[MAX_KEY_LENGTH], value[] = "Vendor";
	LONG msg;
	int i;

	if (ERROR_SUCCESS != RegOpenKeyEx(
		SAPI_TOKEN_HKEY,         // handle to open key
		(LPCTSTR) SAPI_TOKEN_SUBKEY,  // address of name of subkey to open
		NULL,   // reserved
		KEY_ALL_ACCESS, // security access mask
		&hkey)    // address of handle to open key
		)
		shriek(418, "Unable to check voices for SAPI"); // opravit nekde musi byt definice kdy zapnout sapi

	if (ERROR_SUCCESS != RegQueryInfoKey(hkey,                // handle to key to query
		NULL,	//LPTSTR lpClass,           // address of buffer for class string
		NULL,	//LPDWORD lpcbClass,        // address of size of class string buffer
		NULL,	//LPDWORD lpReserved,       // reserved
		&SubKey, //LPDWORD lpcSubKeys,       // address of buffer for number of subkeys
		NULL,	//LPDWORD lpcbMaxSubKeyLen,  // address of buffer for longest subkey name length
		NULL,	//LPDWORD lpcbMaxClassLen,  // address of buffer for longest class string length
		&SubValues,//LPDWORD lpcValues,        // address of buffer for number of value entries
		NULL, //LPDWORD lpcbMaxValueNameLen,  // address of buffer for longest value name length
		NULL,	//LPDWORD lpcbMaxValueLen,  // address of buffer for longest value data length
		NULL,	//LPDWORD lpcbSecurityDescriptor,		// address of buffer for security descriptor length
		NULL)	//PFILETIME lpftLastWriteTime   // address of buffer for last write time
		)
		shriek(418, "Unable to check voices for SAPI"); // opravit nekde musi byt definice kdy zapnout sapi

	for (i=0; i<SubKey;){
		if (ERROR_SUCCESS != RegEnumKeyEx(hkey, i++, nameKey, &cbName, 
			NULL, 
			NULL, 
			NULL, 
			NULL)
			)
			shriek(418, "Unable to check voices for SAPI"); // opravit nekde musi byt definice kdy zapnout sapi

		cbName = MAX_KEY_LENGTH;
		strcpy(allName, SAPI_TOKEN_SUBKEY);
		strcat(allName, nameKey);
		strcat(allName, "\\Attributes");
		if (ERROR_SUCCESS != RegOpenKeyEx( SAPI_TOKEN_HKEY, (LPCTSTR) allName, 
			NULL,   // reserved
			KEY_ALL_ACCESS, // security access mask
			&hSubkey))
			continue;
			//shriek(418, "Unable to check voices for SAPI"); // opravit nekde musi byt definice kdy zapnout sapi
		if (ERROR_SUCCESS != RegQueryValueEx( 
			hSubkey, 
			value, 
			NULL, 
			NULL, 
			(LPBYTE) allName, 
			&cbName))
			shriek(418, "Unable to check voices for SAPI"); // opravit nekde musi byt definice kdy zapnout sapi
		RegCloseKey( hSubkey ); 
		cbName = MAX_KEY_LENGTH;
		if ( !strncmp(allName, "EPOS, ", 6) ||  !strncmp(allName+1, "FE AV ", 6)) {
			strcat(nameKey, "\\Attributes");
			msg = RegDeleteKey( hkey, nameKey);
			nameKey[strlen(nameKey)-11] = 0;
			msg |= RegDeleteKey( hkey, nameKey );
			if (ERROR_SUCCESS != msg)
				shriek(418, "Unable to delete voices for SAPI"); // opravit nekde musi byt definice kdy zapnout sapi
			i--;
			SubKey--;
		}
	}
	RegCloseKey(hkey); 
}

void define_phone_alts()
{
    
	
	HKEY	hkey;
	DWORD	msg;
	char	*lpSubKey = (char*)calloc(80,1), *active_voice, *data, buff[40] = {0},
			*sapi_token_info1[] = {"", "405", "Voice", "CLSID", "SampleFormat", "Options", "F0", ":"},
			*sapi_token_info2[] = {"Age", "Gender", "Language", "Name", "Vendor", ":"},
			*sapi_token_value2[] = {"Adult", "Male", "405;409", "", "EPOS, ÚFE AV ÈR", ":"};
	int		i, n, k = 0;
	voice *v;

	//if ( lpVersion.dwMajorVersion == 5 ) sapi_token_value2[2] = "409;405";	//FIXME pro XP OTHERWISE WINDOWS XP SAPI 5.1 WAS NOT WORKING

    strcpy(lpSubKey, SAPI_PHONE_ALTS);
    strcat(lpSubKey, "Epos_English");
    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR)lpSubKey, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &msg))
			shriek(418, "Unable to register voices for SAPI"); // opravit nekde musi byt definice kdy zapnout sapi
    if ( msg == REG_OPENED_EXISTING_KEY ) return;


    data = _strdup("Epos English Sampa Alternate");
    if (ERROR_SUCCESS != RegSetValueEx( hkey, "", NULL, REG_SZ, (const unsigned char *) data, strlen(data)) )
        shriek(418, "Chyba pri registraci SAPI tokenu");
    else free(data);

    //data = _strdup("{C39615D8-839C-4136-8B2B-64004FEF88F2}"); //MAJNE INTERFACE
    data = _strdup(SAPI_PHCONV_CLSID);
    if (ERROR_SUCCESS != RegSetValueEx( hkey, "CLSID", NULL, REG_SZ, (const unsigned char *) data, strlen(data)) )
        shriek(418, "Chyba pri registraci SAPI tokenu");
    else free(data);

    data = _strdup("- 0001 ! 0002 & 0003 , 0004 . 0005 ? 0006 _ 0007 1 0008 2 0009 aa 000a ae 000b ah 000c ao 000d aw 000e ax 000f ay 0010 b 0011 ch 0012 d 0013 dh 0014 eh 0015 er 0016 ey 0017 f 0018 g 0019 hh 001a ih 001b iy 001c jh 001d k 001e l 001f m 0020 n 0021 ng 0022 ow 0023 oy 0024 p 0025 r 0026 s 0027 sh 0028 t 0029 th 002a uh 002b uw 002c v 002d w 002e y 002f z 0030 zh 0031");
    if (ERROR_SUCCESS != RegSetValueEx( hkey, "PhoneMap", NULL, REG_SZ, (const unsigned char *) data, strlen(data)) )
        shriek(418, "Chyba pri registraci SAPI tokenu");
    else free(data);

    RegCloseKey( hkey );
    strcat(lpSubKey, "\\Attributes");
    if (ERROR_SUCCESS != RegCreateKeyEx( HKEY_LOCAL_MACHINE, (LPCTSTR)lpSubKey, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &msg))
			shriek(418, "Unable to register voices for SAPI"); // opravit nekde musi byt definice kdy zapnout sapi

    data = _strdup("409");
    if (ERROR_SUCCESS != RegSetValueEx( hkey, "Language", NULL, REG_SZ, (const unsigned char *) data, strlen(data)) )
        shriek(418, "Chyba pri registraci SAPI tokenu");
    else free(data);

    data = _strdup("Epos English Sampa Alternate");
    if (ERROR_SUCCESS != RegSetValueEx( hkey, "Name", NULL, REG_SZ, (const unsigned char *) data, strlen(data)) )
        shriek(418, "Chyba pri registraci SAPI tokenu");
    else free(data);

    RegCloseKey( hkey );
}
