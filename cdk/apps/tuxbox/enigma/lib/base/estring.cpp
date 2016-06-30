#include <lib/base/estring.h>
#include <ctype.h>
#include <limits.h>
#include <lib/system/elock.h>
#include <lib/base/gb18030.h>
#include <lib/base/big5.h>
#include <lib/system/econfig.h>

///////////////////////////////////////// eString sprintf /////////////////////////////////////////////////
eString& eString::sprintf(char *fmt, ...)
{
// Implements the normal sprintf method, to use format strings with eString
// The max length of the result string is 1024 char.
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	std::vsnprintf(buf, 1024, fmt, ap);
	va_end(ap);
	assign(buf);
	return *this;
}

///////////////////////////////////////// eString setNum(uint, uint) ///////////////////////////////////////
eString& eString::setNum(int val, int sys)
{
//	Returns a string that contain the value val as string
//	if sys == 16 than hexadezimal if sys == 10 than decimal
	char buf[12];

	if (sys == 10)
		std::snprintf(buf, 12, "%i", val);
	else if (sys == 16)
		std::snprintf(buf, 12, "%X", val);		
	
	assign(buf);
	return *this;
}

///////////////////////////////////////// eString replaceChars(char, char) /////////////////////////////
eString& eString::removeChars(char fchar)
{
//	Remove all chars that equal to fchar, and returns a reference to itself
	unsigned int index=0;

	while ( ( index = find(fchar, index) ) != npos )
		erase(index, 1);

	return *this;
}

/////////////////////////////////////// eString upper() ////////////////////////////////////////////////
eString& eString::upper()
{
//	convert all lowercase characters to uppercase, and returns a reference to itself
	for (iterator it = begin(); it != end(); it++)
		switch(*it)
		{
			case 'a' ... 'z' :
				*it -= 32;
			break;
/*
			case '\xe4' : 		//ä
				*it = '\xc4';	//Ä
			break;
			
			case '\xfc' :		//ü
				*it = '\xdc';	//Ü
			break;
			
			case '\xf6' :		//ö
				*it = '\xd6';	//Ö
			break;
*/		}

	return *this;
}

eString& eString::strReplace(const char* fstr, const eString& rstr,int encode)
{
//	replace all occurrence of fstr with rstr and, and returns a reference to itself
	unsigned int index=0;
	unsigned int fstrlen = strlen(fstr);
	int rstrlen=rstr.size();

	switch(encode){
	case UTF8_ENCODING:
		while(index<length()){
			if( (fstrlen+index)<=length() && !strcmp(mid(index,fstrlen).c_str(),fstr) ){
				replace(index,fstrlen,rstr);
				index+=rstrlen;
				continue;
			}
			if((at(index) & 0xE0)==0xC0)
				index+=2;
			else
			if((at(index) & 0xF0)==0xE0)
				index+=3;
			else
			if((at(index) & 0xF8)==0xF0)
				index+=4;
			else
				index++;
		}
		break;
	case BIG5_ENCODING:
	case GB18030_ENCODING:
		while(index<length()){
			if((fstrlen+index)<=length() && !strcmp(mid(index,fstrlen).c_str(),fstr)){
				replace(index,fstrlen,rstr);
				index+=rstrlen;
				continue;
			}
			if((index+1)>=length())break;
			unsigned char c1=at(index);
			unsigned char c2=at(index+1);
			if ( (c1>0x80 && c1<0xff && c2>=0x40 && c2<0xff)	//GBK
				||(c1>0xa0 && c1<0xf9 && ((c2>=0x40 && c2<0x7f)||(c2>0xa0 && c2<0xff)))	//BIG5
			)
				index+=2;
			else
				index++;
		}
		break;
	case UNICODE_ENCODING:
		while(index<length()){
			if((fstrlen+index)<=length() && !strcmp(mid(index,fstrlen).c_str(),fstr)){
				replace(index,fstrlen,rstr);
				index+=rstrlen;
				continue;
			}
			index+=2;
		}
		break;

	default:
		while ( ( index = find(fstr, index) ) != npos )
		{
			replace(index, fstrlen, rstr);
			index+=rstrlen;
		}
		break;
	}
	return *this;
}

/////////////////////////////////////// eString icompare(const eString&) ////////////////////////////////////////////////
int eString::icompare(const eString& s)
{
//	makes a case insensitive string compare
	std::string::const_iterator p = begin(),
															p2 = s.begin();

	while ( p != end() && p2 != s.end() )
		if ( tolower(*p) != tolower(*p2) )
			return tolower(*p) < tolower(*p2) ? -1 : 1;
		else
			p++, p2++;

	return length() == s.length() ? 0 : length() < s.length() ? -1 : 1;
}

std::map<eString, int> eString::CountryCodeDefaultMapping;
std::map<int, int> eString::TransponderDefaultMapping;
std::set<int> eString::TransponderUseTwoCharMapping;
std::map<int, int> eString::ChineseTradAndSimpMapping;
int eString::convertChineseTradToSimp = 0;
int eString::defaultCharsetEncoding = 0;

int parseEncoding(char *encoding)
{
	int icode=0,flag_assign=0;
	if (encoding[0] == '='){
		flag_assign = 1;
		encoding += 1;
	}
	if (strcasecmp(encoding, "VideoTexSuppl") == 0)
		icode = VIDEOTEXSUPPL_ENCODING;
	else if (strcasecmp(encoding, "GB2312") == 0 || strcasecmp(encoding, "GBK") == 0 
		|| strcasecmp(encoding, "GB18030") == 0 || strcasecmp(encoding, "CP936") == 0 )
		icode = GB18030_ENCODING;
	else if (strcasecmp(encoding, "BIG5") == 0 || strcasecmp(encoding, "CP950") == 0 )
		icode = BIG5_ENCODING;
	else if (strcasecmp(encoding,"UTF8") == 0 || strcasecmp(encoding,"UTF-8") == 0 )
		icode = UTF8_ENCODING;
	else if (strcasecmp(encoding, "UTF16BE")==0)
		icode = UTF16BE_ENCODING;
	else if (strcasecmp(encoding, "UTF16LE")==0)
		icode = UTF16LE_ENCODING;
	else if (strcasecmp(encoding, "UNICODE")==0)
		icode = UNICODE_ENCODING;
	else
	{
		int enc;
		if (sscanf(toupper(encoding), "ISO8859-%d", &enc) == 1)
			icode = enc;
	}
	if (flag_assign == 1)
		icode |= 0x80;
	return icode;
}

int eString::readChineseMapFile()
{
	FILE *f = fopen(CONFIGDIR "/enigma/chinese.map","rb");
	if(!f)f = fopen(TUXBOXDATADIR "/enigma/chinese.map","rb");
	if(!f)return -1;
	int s=0,t=0;
	unsigned short lastcode=0;
	ChineseTradAndSimpMapping.clear();
	int endian=0;
	while (!feof(f)){
		char tmp[20];
		unsigned short code;
		if(!endian){
			if(!fread(tmp,10,1,f))break;
			if(memcmp(tmp,"\x00\x55\x00\x43\x00\x53\x00\x2D\x00\x32",10)==0)
				endian=1;	//big endian
			else if(memcmp(tmp,"\x55\x00\x43\x00\x53\x00\x2D\x00\x32\x00",10)==0)
				endian=2;	//little endian
			else break;
		}
		if(!fread(tmp,2,1,f))break;
		if(endian==1)
			code=tmp[0] << 8 | tmp[1];
		else
			code=tmp[1] <<8 | tmp[0];
		if(code==0x3A || code==0xFF1A)	// acsii char : or chinese :
			s=lastcode;

		if(lastcode==0x3A || lastcode==0xFF1A ){
			t=code;
			ChineseTradAndSimpMapping[t]=s;
			lastcode=0;
		}
		if(code>0x100 || code==0x3A  || code==0xFF1A)
			lastcode=code;
	}
	fclose(f);
}


int eString::readEncodingFile()
{
	FILE *f = fopen(CONFIGDIR "/enigma/encoding.conf", "rt");
	if (f)
	{

		CountryCodeDefaultMapping.clear();
		TransponderDefaultMapping.clear();
		TransponderUseTwoCharMapping.clear();
		char *line = (char*) malloc(256);
		size_t bufsize=256;
		int count, i;
		int tsid, onid, encoding;
		bool parsed;
		while( getline(&line, &bufsize, f) != -1 )
		{
			if ( line[0] == '#' )
				continue;

			char sec[4][64];
			count = sscanf(line, "%s%s%s%s", sec[0], sec[1], sec[2],sec[3]);
			for (i = 0; i < count; i++)
			{
				if (sec[i][0] == '#')
				{
					count = i;
					break;
				}
			}
			parsed = false;
			if (count == 2)
			{
				if ( (encoding = parseEncoding(sec[1])) >= 0)
				{
					if (sec[0][0] == '*')
						eString::defaultCharsetEncoding = encoding;					
					else if (strlen(sec[0]) == 3)
						CountryCodeDefaultMapping[sec[0]] = encoding;
					parsed = true;
				}
				else
				{
					strcpy(sec[2], "VideoTexSuppl");
					count++;
				}
			}
			if (count == 3 &&
				((sscanf(sec[0], "0x%x", &tsid) == 1 && sscanf(sec[1], "0x%x", &onid) == 1) ||
				(sscanf(sec[0], "%d", &tsid) == 1 && sscanf(sec[1], "%d", &onid) == 1)) &&
				(encoding = parseEncoding(sec[2])) >= 0)
			{
				if (encoding == VIDEOTEXSUPPL_ENCODING)
					TransponderUseTwoCharMapping.insert((tsid<<16)|onid);
				else{
					TransponderDefaultMapping[(tsid<<16)|onid]=encoding;
					}
				parsed = true;
			}
			if (!parsed)
				eDebug("couldn't parse %s", line);
		}
		fclose(f);
		free(line);
		return 0;

	}
	return -1;
}


		// 8859-x to ucs-16 coding tables. taken from www.unicode.org/Public/MAPPINGS/ISO8859/

static unsigned long c88592[96]={
0x00A0, 0x0104, 0x02D8, 0x0141, 0x00A4, 0x013D, 0x015A, 0x00A7, 0x00A8, 0x0160, 0x015E, 0x0164, 0x0179, 0x00AD, 0x017D, 0x017B,
0x00B0, 0x0105, 0x02DB, 0x0142, 0x00B4, 0x013E, 0x015B, 0x02C7, 0x00B8, 0x0161, 0x015F, 0x0165, 0x017A, 0x02DD, 0x017E, 0x017C,
0x0154, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7, 0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD, 0x00CE, 0x010E,
0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7, 0x0158, 0x016E, 0x00DA, 0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF,
0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7, 0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F,
0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x00F7, 0x0159, 0x016F, 0x00FA, 0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9};
	
static unsigned long c88593[96]={
0x00A0, 0x0126, 0x02D8, 0x00A3, 0x00A4, 0x0000, 0x0124, 0x00A7, 0x00A8, 0x0130, 0x015E, 0x011E, 0x0134, 0x00AD, 0x0000, 0x017B,
0x00B0, 0x0127, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x0125, 0x00B7, 0x00B8, 0x0131, 0x015F, 0x011F, 0x0135, 0x00BD, 0x0000, 0x017C,
0x00C0, 0x00C1, 0x00C2, 0x0000, 0x00C4, 0x010A, 0x0108, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
0x0000, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x0120, 0x00D6, 0x00D7, 0x011C, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x016C, 0x015C, 0x00DF,
0x00E0, 0x00E1, 0x00E2, 0x0000, 0x00E4, 0x010B, 0x0109, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
0x0000, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x0121, 0x00F6, 0x00F7, 0x011D, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x016D, 0x015D, 0x02D9};

static unsigned long c88594[96]={
0x00A0, 0x0104, 0x0138, 0x0156, 0x00A4, 0x0128, 0x013B, 0x00A7, 0x00A8, 0x0160, 0x0112, 0x0122, 0x0166, 0x00AD, 0x017D, 0x00AF,
0x00B0, 0x0105, 0x02DB, 0x0157, 0x00B4, 0x0129, 0x013C, 0x02C7, 0x00B8, 0x0161, 0x0113, 0x0123, 0x0167, 0x014A, 0x017E, 0x014B,
0x0100, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x012E, 0x010C, 0x00C9, 0x0118, 0x00CB, 0x0116, 0x00CD, 0x00CE, 0x012A,
0x0110, 0x0145, 0x014C, 0x0136, 0x00D4, 0x00D5, 0x00D6, 0x00D7, 0x00D8, 0x0172, 0x00DA, 0x00DB, 0x00DC, 0x0168, 0x016A, 0x00DF,
0x0101, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x012F, 0x010D, 0x00E9, 0x0119, 0x00EB, 0x0117, 0x00ED, 0x00EE, 0x012B,
0x0111, 0x0146, 0x014D, 0x0137, 0x00F4, 0x00F5, 0x00F6, 0x00F7, 0x00F8, 0x0173, 0x00FA, 0x00FB, 0x00FC, 0x0169, 0x016B, 0x02D9};

static unsigned long c88595[96]={
0x00A0, 0x0401, 0x0402, 0x0403, 0x0404, 0x0405, 0x0406, 0x0407, 0x0408, 0x0409, 0x040A, 0x040B, 0x040C, 0x00AD, 0x040E, 0x040F,
0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
0x2116, 0x0451, 0x0452, 0x0453, 0x0454, 0x0455, 0x0456, 0x0457, 0x0458, 0x0459, 0x045A, 0x045B, 0x045C, 0x00A7, 0x045E, 0x045F};
	
static unsigned long c88596[96]={
0x00A0, 0x0000, 0x0000, 0x0000, 0x00A4, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x060C, 0x00AD, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x061B, 0x0000, 0x0000, 0x0000, 0x061F,
0x0000, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627, 0x0628, 0x0629, 0x062A, 0x062B, 0x062C, 0x062D, 0x062E, 0x062F,
0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x0637, 0x0638, 0x0639, 0x063A, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0640, 0x0641, 0x0642, 0x0643, 0x0644, 0x0645, 0x0646, 0x0647, 0x0648, 0x0649, 0x064A, 0x064B, 0x064C, 0x064D, 0x064E, 0x064F,
0x0650, 0x0651, 0x0652, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};
	
static unsigned long c88597[96]={
0x00A0, 0x2018, 0x2019, 0x00A3, 0x20AC, 0x20AF, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x037A, 0x00AB, 0x00AC, 0x00AD, 0x0000, 0x2015,
0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x0384, 0x0385, 0x0386, 0x00B7, 0x0388, 0x0389, 0x038A, 0x00BB, 0x038C, 0x00BD, 0x038E, 0x038F,
0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F,
0x03A0, 0x03A1, 0x0000, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7, 0x03A8, 0x03A9, 0x03AA, 0x03AB, 0x03AC, 0x03AD, 0x03AE, 0x03AF,
0x03B0, 0x03B1, 0x03B2, 0x03B3, 0x03B4, 0x03B5, 0x03B6, 0x03B7, 0x03B8, 0x03B9, 0x03BA, 0x03BB, 0x03BC, 0x03BD, 0x03BE, 0x03BF,
0x03C0, 0x03C1, 0x03C2, 0x03C3, 0x03C4, 0x03C5, 0x03C6, 0x03C7, 0x03C8, 0x03C9, 0x03CA, 0x03CB, 0x03CC, 0x03CD, 0x03CE, 0x0000};

static unsigned long c88598[96]={
0x00A0, 0x0000, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x00D7, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00F7, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2017,
0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7, 0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF,
0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA, 0x0000, 0x0000, 0x200E, 0x200F, 0x0000};

static unsigned long c88599[96]={
0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
0x011E, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7, 0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x0130, 0x015E, 0x00DF,
0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
0x011F, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7, 0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x0131, 0x015F, 0x00FF};

static unsigned long c885910[96]={
0x00A0, 0x0104, 0x0112, 0x0122, 0x012A, 0x0128, 0x0136, 0x00A7, 0x013B, 0x0110, 0x0160, 0x0166, 0x017D, 0x00AD, 0x016A, 0x014A,
0x00B0, 0x0105, 0x0113, 0x0123, 0x012B, 0x0129, 0x0137, 0x00B7, 0x013C, 0x0111, 0x0161, 0x0167, 0x017E, 0x2015, 0x016B, 0x014B,
0x0100, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x012E, 0x010C, 0x00C9, 0x0118, 0x00CB, 0x0116, 0x00CD, 0x00CE, 0x00CF,
0x00D0, 0x0145, 0x014C, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x0168, 0x00D8, 0x0172, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
0x0101, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x012F, 0x010D, 0x00E9, 0x0119, 0x00EB, 0x0117, 0x00ED, 0x00EE, 0x00EF,
0x00F0, 0x0146, 0x014D, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x0169, 0x00F8, 0x0173, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x0138};
	
static unsigned long c885911[96]={
0x00A0, 0x0E01, 0x0E02, 0x0E03, 0x0E04, 0x0E05, 0x0E06, 0x0E07, 0x0E08, 0x0E09, 0x0E0A, 0x0E0B, 0x0E0C, 0x0E0D, 0x0E0E, 0x0E0F,
0x0E10, 0x0E11, 0x0E12, 0x0E13, 0x0E14, 0x0E15, 0x0E16, 0x0E17, 0x0E18, 0x0E19, 0x0E1A, 0x0E1B, 0x0E1C, 0x0E1D, 0x0E1E, 0x0E1F,
0x0E20, 0x0E21, 0x0E22, 0x0E23, 0x0E24, 0x0E25, 0x0E26, 0x0E27, 0x0E28, 0x0E29, 0x0E2A, 0x0E2B, 0x0E2C, 0x0E2D, 0x0E2E, 0x0E2F,
0x0E30, 0x0E31, 0x0E32, 0x0E33, 0x0E34, 0x0E35, 0x0E36, 0x0E37, 0x0E38, 0x0E39, 0x0E3A, 0x0000, 0x0000, 0x0000, 0x0000, 0x0E3F,
0x0E40, 0x0E41, 0x0E42, 0x0E43, 0x0E44, 0x0E45, 0x0E46, 0x0E47, 0x0E48, 0x0E49, 0x0E4A, 0x0E4B, 0x0E4C, 0x0E4D, 0x0E4E, 0x0E4F,
0x0E50, 0x0E51, 0x0E52, 0x0E53, 0x0E54, 0x0E55, 0x0E56, 0x0E57, 0x0E58, 0x0E59, 0x0E5A, 0x0E5B, 0x0000, 0x0000, 0x0000, 0x0000};

static unsigned long c885913[96]={
0x00A0, 0x201D, 0x00A2, 0x00A3, 0x00A4, 0x201E, 0x00A6, 0x00A7, 0x00D8, 0x00A9, 0x0156, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00C6,
0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x201C, 0x00B5, 0x00B6, 0x00B7, 0x00F8, 0x00B9, 0x0157, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00E6,
0x0104, 0x012E, 0x0100, 0x0106, 0x00C4, 0x00C5, 0x0118, 0x0112, 0x010C, 0x00C9, 0x0179, 0x0116, 0x0122, 0x0136, 0x012A, 0x013B,
0x0160, 0x0143, 0x0145, 0x00D3, 0x014C, 0x00D5, 0x00D6, 0x00D7, 0x0172, 0x0141, 0x015A, 0x016A, 0x00DC, 0x017B, 0x017D, 0x00DF,
0x0105, 0x012F, 0x0101, 0x0107, 0x00E4, 0x00E5, 0x0119, 0x0113, 0x010D, 0x00E9, 0x017A, 0x0117, 0x0123, 0x0137, 0x012B, 0x013C,
0x0161, 0x0144, 0x0146, 0x00F3, 0x014D, 0x00F5, 0x00F6, 0x00F7, 0x0173, 0x0142, 0x015B, 0x016B, 0x00FC, 0x017C, 0x017E, 0x2019};

static unsigned long c885914[96]={
0x00A0, 0x1E02, 0x1E03, 0x00A3, 0x010A, 0x010B, 0x1E0A, 0x00A7, 0x1E80, 0x00A9, 0x1E82, 0x1E0B, 0x1EF2, 0x00AD, 0x00AE, 0x0178,
0x1E1E, 0x1E1F, 0x0120, 0x0121, 0x1E40, 0x1E41, 0x00B6, 0x1E56, 0x1E81, 0x1E57, 0x1E83, 0x1E60, 0x1EF3, 0x1E84, 0x1E85, 0x1E61,
0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
0x0174, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x1E6A, 0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x0176, 0x00DF,
0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
0x0175, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x1E6B, 0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x0177, 0x00FF};

static unsigned long c885915[96]={
0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x20AC, 0x00A5, 0x0160, 0x00A7, 0x0161, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x017D, 0x00B5, 0x00B6, 0x00B7, 0x017E, 0x00B9, 0x00BA, 0x00BB, 0x0152, 0x0153, 0x0178, 0x00BF,
0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7, 0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7, 0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF};

static unsigned long c885916[96]={
0x00A0, 0x0104, 0x0105, 0x0141, 0x20AC, 0x201E, 0x0160, 0x00A7, 0x0161, 0x00A9, 0x0218, 0x00AB, 0x0179, 0x00AD, 0x017A, 0x017B,
0x00B0, 0x00B1, 0x010C, 0x0142, 0x017D, 0x201D, 0x00B6, 0x00B7, 0x017E, 0x010D, 0x0219, 0x00BB, 0x0152, 0x0153, 0x0178, 0x017C,
0x00C0, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0106, 0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
0x0110, 0x0143, 0x00D2, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x015A, 0x0170, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x0118, 0x021A, 0x00DF,
0x00E0, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x0107, 0x00E6, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
0x0111, 0x0144, 0x00F2, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x015B, 0x0171, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x0119, 0x021B, 0x00FF};

// Two Char Mapping ( many polish services and UPC Direct/HBO services)
// get from http://mitglied.lycos.de/buran/charsets/videotex-suppl.html
static inline unsigned int doVideoTexSuppl(int c1, int c2)
{
	switch (c1)
	{
		case 0xC1: // grave
			switch (c2)
			{
				case 0x61: return 224;				case 0x41: return 192;
				case 0x65: return 232;				case 0x45: return 200;
				case 0x69: return 236;				case 0x49: return 204;
				case 0x6f: return 242;				case 0x4f: return 210;
				case 0x75: return 249;				case 0x55: return 217;
				default: return 0;
			}
		case 0xC2: // acute
			switch (c2)
			{
				case 0x61: return 225;				case 0x41: return 193;
				case 0x65: return 233;				case 0x45: return 201;
				case 0x69: return 237;				case 0x49: return 205;
				case 0x6f: return 243;				case 0x4f: return 211;
				case 0x75: return 250;				case 0x55: return 218;
				case 0x79: return 253;				case 0x59: return 221;
				case 0x63: return 263;				case 0x43: return 262;
				case 0x6c: return 314;				case 0x4c: return 313;
				case 0x6e: return 324;				case 0x4e: return 323;
				case 0x72: return 341;				case 0x52: return 340;
				case 0x73: return 347;				case 0x53: return 346;
				case 0x7a: return 378;				case 0x5a: return 377;
				default: return 0;
			}
		case 0xC3: // cedilla
			switch (c2)
			{
				case 0x61: return 226;				case 0x41: return 194;
				case 0x65: return 234;				case 0x45: return 202;
				case 0x69: return 238;				case 0x49: return 206;
				case 0x6f: return 244;				case 0x4f: return 212;
				case 0x75: return 251;				case 0x55: return 219;
				case 0x79: return 375;				case 0x59: return 374;
				case 0x63: return 265;				case 0x43: return 264;
				case 0x67: return 285;				case 0x47: return 284;
				case 0x68: return 293;				case 0x48: return 292;
				case 0x6a: return 309;				case 0x4a: return 308;
				case 0x73: return 349;				case 0x53: return 348;
				case 0x77: return 373;				case 0x57: return 372;
				default: return 0;
			}
		case 0xC4: // tilde
			switch (c2)
			{
				case 0x61: return 227;				case 0x41: return 195;
				case 0x6e: return 241;				case 0x4e: return 209;
				case 0x69: return 297;				case 0x49: return 296;
				case 0x6f: return 245;				case 0x4f: return 213;
				case 0x75: return 361;				case 0x55: return 360;
				default: return 0;
			}
		case 0xC6: // breve
			switch (c2)
			{
				case 0x61: return 259;				case 0x41: return 258;
				case 0x67: return 287;				case 0x47: return 286;
				case 0x75: return 365;				case 0x55: return 364;
				case 0x73: return 353;				case 0x53: return 352;
				default: return 0;
			}
		case 0xC7: // dot above
			switch (c2)
			{
				case 0x63: return 267;				case 0x43: return 266;
				case 0x65: return 279;				case 0x45: return 278;
				case 0x67: return 289;				case 0x47: return 288;
				case 0x49: return 304;				case 0x7a: return 380;
				case 0x5a: return 379; 
				default: return 0;
			}
		case 0xC8: // diaeresis
			switch (c2)
			{
				case 0x61: return 228;				case 0x41: return 196;
				case 0x65: return 235;				case 0x45: return 203;
				case 0x69: return 239;				case 0x49: return 207;
				case 0x6f: return 246;				case 0x4f: return 214;
				case 0x75: return 252;				case 0x55: return 220;
				case 0x79: return 255;				case 0x59: return 376;
				default: return 0;
				}
		case 0xCA: // ring above
			switch (c2)
			{
				case 0x61: return 229;				case 0x41: return 197;
				case 0x75: return 367;				case 0x55: return 366;
				default: return 0;
			}
		case 0xCB: // cedilla
			switch (c2)
			{
				case 0x63: return 231;				case 0x43: return 199;
				case 0x67: return 291;				case 0x47: return 290;
				case 0x6b: return 311;				case 0x4b: return 310;
				case 0x6c: return 316;				case 0x4c: return 315;
				case 0x6e: return 326;				case 0x4e: return 325;
				case 0x72: return 343;				case 0x52: return 342;
				case 0x73: return 351;				case 0x53: return 350;
				case 0x74: return 355;				case 0x54: return 354;
				default: return 0;
			}
		case 0xCD: // double acute accent
			switch (c2)
			{
				case 0x6f: return 337;				case 0x4f: return 336;
				case 0x75: return 369;				case 0x55: return 368;
				default: return 0;
			}
		case 0xCE: // ogonek
			switch (c2)
			{
				case 0x61: return 261;				case 0x41: return 260;
				case 0x65: return 281;				case 0x45: return 280;
				case 0x69: return 303;				case 0x49: return 302;
				case 0x75: return 371;				case 0x55: return 370;
				default: return 0;
			}
		case 0xCF: // caron
			switch (c2)
			{
				case 0x63: return 269;				case 0x43: return 268;
				case 0x64: return 271;				case 0x44: return 270;
				case 0x65: return 283;				case 0x45: return 282;
				case 0x6c: return 318;				case 0x4c: return 317;
				case 0x6e: return 328;				case 0x4e: return 327;
				case 0x72: return 345;				case 0x52: return 344;
				case 0x73: return 353;				case 0x53: return 352;
				case 0x74: return 357;				case 0x54: return 356;
				case 0x7a: return 382;				case 0x5a: return 381;
				default: return 0;
			}
	}                   
	return 0;
}

static inline unsigned int recode(unsigned char d, int cp)
{
	if (d < 0xA0)
		return d;
	switch (cp)
	{
	case 0:		// Latin1 <-> unicode mapping
	case 1:		// 8859-1 <-> unicode mapping
		return d;
	case 2:		// 8859-2 -> unicode mapping
		return c88592[d-0xA0];
	case 3:		// 8859-3 -> unicode mapping
		return c88593[d-0xA0];
	case 4:		// 8859-2 -> unicode mapping
		return c88594[d-0xA0];
	case 5:		// 8859-5 -> unicode mapping
		return c88595[d-0xA0];
	case 6:		// 8859-6 -> unicode mapping
		return c88596[d-0xA0];
	case 7:		// 8859-7 -> unicode mapping
		return c88597[d-0xA0];
	case 8:		// 8859-8 -> unicode mapping
		return c88598[d-0xA0];
	case 9:		// 8859-9 -> unicode mapping
		return c88599[d-0xA0];
	case 10:// 8859-10 -> unicode mapping
		return c885910[d-0xA0];
	case 11:// 8859-11 -> unicode mapping
		return c885911[d-0xA0];
/*	case 12:// 8859-12 -> unicode mapping  // reserved for indian use..
		return c885912[d-0xA0];*/
	case 13:// 8859-13 -> unicode mapping
		return c885913[d-0xA0];
	case 14:// 8859-14 -> unicode mapping
		return c885914[d-0xA0];
	case 15:// 8859-15 -> unicode mapping
		return c885915[d-0xA0];
	case 16:// 8859-16 -> unicode mapping
		return c885916[d-0xA0];
	default:
		return d;
	}
}

unsigned long ChineseTradToSimp(long c)
{
	if (eString::convertChineseTradToSimp==0)
		return c;
	std::map<int, int>::iterator i=eString::ChineseTradAndSimpMapping.find(c);
	if(i!=eString::ChineseTradAndSimpMapping.end())
		return i->second;
	else
		return c;
}

eString ChineseTradToSimp(const char *szIn,int len)
{
	if (eString::convertChineseTradToSimp==0)
		return eString(szIn,len);
	unsigned long code=0;
	unsigned char temp[4096];
	unsigned int j=0;
	for (int i=0; i < len; ++i)
	{
		if (!(szIn[i]&0x80)){ // normal ASCII
			temp[j++]=szIn[i];
			continue;
		}else if ((szIn[i] & 0xE0) == 0xC0) // one char following.
		{
				// first, length check:
			if (i+1 >= len){
				temp[j++]=szIn[i];
				break;
			}else	if ((szIn[i+1]&0xC0) != 0x80){
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i];
				continue;
			}
			code=(szIn[i] & 0x1F)<<6 | (szIn[i+1] & 0x3F);
			i++;
		} else if ((szIn[i] & 0xF0) == 0xE0)
		{
			if ((i+2) >= len){
				temp[j++]=szIn[i];
				break;
			}else	if ((szIn[i+1]&0xC0) != 0x80 || (szIn[i+2]&0xC0) != 0x80 ){
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i];
				continue;
			}
			code=((szIn[i] & 0x0F)<<12) | ((szIn[i+1] & 0x3F)<<6) | (szIn[i+2] & 0x3F);
			i+=2;
		} else if ((szIn[i] & 0xF8) == 0xF0)
		{
			if ((i+3) >= len){
				temp[j++]=szIn[i];
				break;
			}else	if ((szIn[i+1]&0xC0) != 0x80 || (szIn[i+2]&0xC0) != 0x80 || (szIn[i+3]&0xC0) != 0x80){
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i];
				continue;
			}
			code=((szIn[i] & 0x07)<<18)|((szIn[i+1] & 0x3F)<<12) | ((szIn[i+2] & 0x3F)<<6) | (szIn[i+3] & 0x3F);
			i+=3;
		} else if ((szIn[i] & 0xFC) == 0xF8)
		{
			if ((i+4) >= len){
				temp[j++]=szIn[i];
				break;
			}else if ((szIn[i+1]&0xC0) != 0x80 || (szIn[i+2]&0xC0) != 0x80 || (szIn[i+3]&0xC0) != 0x80
				|| (szIn[i+4]&0xC0) != 0x80){
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i];
				continue;
			}
			code=((szIn[i] & 0x03)<<24)|((szIn[i+1] & 0x3F)<<18)|((szIn[i+2] & 0x3F)<<12) | ((szIn[i+3] & 0x3F)<<6) | (szIn[i+4] & 0x3F);
			i+=4;
		} else if ((szIn[i] & 0xFD) == 0xFC)
		{
			if ((i+5) >= len){
				temp[j++]=szIn[i];
				break;
			}else if ((szIn[i+1]&0xC0) != 0x80 || (szIn[i+2]&0xC0) != 0x80 || (szIn[i+3]&0xC0) != 0x80
				|| (szIn[i+4]&0xC0) != 0x80 || (szIn[i+5]&0xC0) != 0x80){
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i];
				continue;
			}
			code=((szIn[i] & 0x01)<<30)|((szIn[i+1] & 0x03F)<<24)|((szIn[i+2] & 0x3F)<<18)|((szIn[i+3] & 0x3F)<<12) | ((szIn[i+4] & 0x3F)<<6) | (szIn[i+5] & 0x3F);
		i+=5;
		}else{
			temp[j++]=szIn[i];
			continue;
		}
		code=ChineseTradToSimp(code);
		int ret=UnicodeToUTF8(code,(char*)temp+j);
		j+=ret;
	}
	temp[j]='\0';
//	eDebug("[ChineseTradToSimp] %s",temp);
	return eString((const char *)temp,j);
}

int UnicodeToUTF8(long c, char *out)
{
	char *s = out;
	int ret = 0;

	if (c < 0x80){
		*(s++) = c;
		ret = 1;
	}else if (c < 0x800)
	{
		*(s++) = 0xc0 | (c >> 6);
		*(s++) = 0x80 | (c & 0x3f);
		ret = 2;
	}
	else if (c < 0x10000)
	{
		*(s++) = 0xe0 | (c >> 12);
		*(s++) = 0x80 | ((c >> 6) & 0x3f);
		*(s++) = 0x80 | (c & 0x3f);
		ret = 3;
	}
	else if (c < 0x200000)
	{
		*(s++) = 0xf0 | (c >> 18);
		*(s++) = 0x80 | ((c >> 12) & 0x3f);
		*(s++) = 0x80 | ((c >> 6) & 0x3f);
		*(s++) = 0x80 | (c & 0x3f);
		ret = 4;
	}
	return ret;
}


eString GB18030ToUTF8(const char *szIn, int len,int *pconvertedLen)
{
	char szOut[len * 2];
	unsigned long code=0;
	int t=0,i;

	for(i=0;i<len;){
		int cl=0,k=0;

		cl=gb18030_mbtowc((ucs4_t*)(&code),(const unsigned char *)szIn+i,len-i);
		if(cl>0)
			k=UnicodeToUTF8(code,szOut+t);
		t+=k;
		if(cl>0)
			i+=cl;
		else
			i++;
	}
	szOut[t]='\0';

	if(pconvertedLen)*pconvertedLen=i;
	return eString(szOut,t);
}

eString Big5ToUTF8(const char *szIn, int len,int *pconvertedLen)
{
	char szOut[len * 2];
	unsigned long code=0;
	int t=0,i=0;

	for(;i<(len-1);i++){
		if((szIn[i]>0xA0) && szIn[i]<=0xF9 &&(
			((szIn[i+1]>=0x40)&&(szIn[i+1]<=0x7F)) || ((szIn[i+1]>0xA0)&&(szIn[i+1]<0xFF))
		    )){
			big5_mbtowc((ucs4_t*)(&code),(const unsigned char *)szIn+i,2);
			code=ChineseTradToSimp(code);
			int k=UnicodeToUTF8(code,szOut+t);
			t+=k;
			i++;
		     }
		else
			szOut[t++]=szIn[i];
	}

	if(i<len && szIn[i] && (szIn[i]<0xA0 || szIn[i]>0xF9))
		szOut[t++]=szIn[i++];

	if(pconvertedLen)*pconvertedLen=i;
	return eString(szOut,t);
}

eString convertDVBUTF8(const unsigned char *data, int len, int table, int tsidonid,int noEncodeID,int *pconvertedLen)
{
	int i=0, t=0;
	int encode=table;
	eString output="";

	if (!len){
		if(pconvertedLen)*pconvertedLen=0;
		return "";
	}

	// table & 0x80 indicate force assign the table.
	if ( tsidonid && ((table & 0x80) == 0) )
	{
		std::map<int, int>::iterator it =
			eString::TransponderDefaultMapping.find(tsidonid);
		if ( it != eString::TransponderDefaultMapping.end() )
			encode = it->second;
	}

	bool autoCheckTable=(encode & 0x80)?0:1;
//	eDebug("ConvertDVBUTF8-1:<data=%s><table=0x%x><tsidonid=%d>\n",data,table,tsidonid);
	if (autoCheckTable && table != UNICODE_ENCODING && table != UTF16BE_ENCODING && table != UTF16LE_ENCODING )
	{
	   switch(data[0])
	   {
		case 1 ... 11:
			encode=data[i++]+4;
//			eDebug("(1..12)text encoded in ISO-8859-%d",table);
			break;
		case 0x10:
		{
//			eDebug("(0x10)text encoded in ISO-8859-%d",n);
			int n=(data[++i]<<8)|(data[++i]);
			++i;
			switch(n)
			{
				case 12: 
					eDebug("unsup. ISO8859-12 enc.", n);
				default:
					encode=n;
					break;
			}
			break;
		}
		case 0x11: //  Basic Multilingual Plane of ISO/IEC 10646-1 enc  (UTF-16... Unicode)
			encode=UNICODE_ENCODING;
			tsidonid = 0;
			++i;
			break;
		case 0x12:
			++i;
			eDebug("unsup. KSC 5601 enc.");
			break;
		case 0x13:
			++i;
			encode=GB18030_ENCODING;
//			eDebug("unsup. GB-2312-1980 enc.");
			break;
		case 0x14:
			++i;
			encode=BIG5_ENCODING;
//			eDebug("unsup. Big5 subset of ISO/IEC 10646-1 enc.");
			break;
		case 0x15: // UTF-8 encoding of ISO/IEC 10646-1
			++i;
			encode=UTF8_ENCODING;
//			return std::string((char*)data+1, len-1);
			return eString((char*)data+1, len-1);
			break;
		case 0x16:
			encode=UTF16BE_ENCODING;
			++i;
			break;
		case 0x17:
			encode=UTF16LE_ENCODING;
			++i;
			break;
		case 0:
		case 0xD ... 0xF:
		case 0x18 ... 0x1F:
			eDebug("reserved %d", data[0]);
			++i;
			break;
	  }
	}
	else
		table &= 0x7F;

	int offset = i;
	switch(encode)
	{
		case GB18030_ENCODING:
			output = GB18030ToUTF8((const char *)(data + i), len - i,pconvertedLen);
			if(pconvertedLen)
				*pconvertedLen += i;
			break;
		case BIG5_ENCODING:
			output = Big5ToUTF8((const char *)(data + i), len - i,pconvertedLen);
			if(pconvertedLen)
				*pconvertedLen += i;
			break;
		default:
		    unsigned char res[4096];
		    while (i < len && (size_t)t < sizeof(res))
		    {
			unsigned long code=0;

			if ( i+1 < len && tsidonid &&
				eString::TransponderUseTwoCharMapping.find(tsidonid) != eString::TransponderUseTwoCharMapping.end() &&	(code=doVideoTexSuppl(data[i], data[i+1])) )
				i++;
			else if(encode==UTF16BE_ENCODING || encode==UNICODE_ENCODING){
				if((i+2)>len)break;
				unsigned long w1=((unsigned long)(data[i])<<8) |((unsigned long)(data[i+1]));
				if(w1<0xD800UL || w1>0xDFFFUL){
					code=w1;
					i+=2;
				}
				else if(w1>0xDBFFUL)
					break;
				else if((i+4)<len){
					unsigned long w2=((unsigned long)(data[i+2])<<8) |((unsigned long)(data[i+3]));
					if(w2<0xDC00UL || w2>0xDFFFUL)return eString("");
					code=0x10000UL + ((w1 & 0x03FFUL)<<10 ) | (w2 & 0x03FFUL);
					i+=4;
				}
				else
					break;
			}
			else if(encode==UTF16LE_ENCODING){
				if((i+2)>len)break;
				unsigned long w1=((unsigned long)(data[i+1])<<8) |((unsigned long)(data[i]));
				if(w1<0xD800UL || w1>0xDFFFUL){
					code=w1;
					i+=2;
				}
				else if(w1>0xDBFFUL)
					break;
				else if((i+4)<len){
					unsigned long w2=((unsigned long)(data[i+3])<<8) |((unsigned long)(data[i+2]));
					if(w2<0xDC00UL || w2>0xDFFFUL)break;
					code=0x10000UL + ((w2 & 0x03FFUL)<<10 ) | (w1 & 0x03FFUL);
					i+=4;
				}
				else
					break;
			}
			else if  (encode==0 || encode==UTF8_ENCODING){ 	// UTF-8 or default
				res[t++]=data[i++];
				continue;
			}
			else
				code=recode(data[i++], encode);

			if (!code)
				continue;

			if (code < 0x80){ // identity ascii <-> utf8 mapping
				res[t++]=char(code);
				}
			else if (code < 0x800 && ( (size_t) t ) + 1 < sizeof(res) ) // two byte mapping
			{
				res[t++]=(code>>6)|0xC0;
				res[t++]=(code&0x3F)|0x80;
			} else if (code < 0x10000 && ( (size_t) t ) + 2 < sizeof(res) ) // three bytes mapping
			{
				res[t++]=(code>>12)|0xE0;
				res[t++]=((code>>6)&0x3F)|0x80;
				res[t++]=(code&0x3F)|0x80;
			} else if (( (size_t) t ) + 3 < sizeof(res))
			{
				res[t++]=(code>>18)|0xF0;
				res[t++]=((code>>12)&0x3F)|0x80;
				res[t++]=((code>>6)&0x3F)|0x80;
				res[t++]=(code&0x3F)|0x80;
			}
		    }
		    if(pconvertedLen)*pconvertedLen=i;
		    output = eString((char*)res, t);
		    break;
	}
	if (pconvertedLen && *pconvertedLen < len)
		eDebug("convertDVBUTF8 converted %d chars and %d not converted!",*pconvertedLen,len - offset - *pconvertedLen);
	return output;
}

eString convertUTF8DVB(const eString &string, int table)
{
	unsigned long *coding_table=0;

	int len=string.length(), t=0;

	unsigned char buf[len];

	for(int i=0;i<len;i++)
	{
		unsigned char c1=string[i];
		unsigned int c;
		if(c1<0x80)
			c=c1;
		else
		{
			i++;
			unsigned char c2=string[i];
			c=((c1&0x3F)<<6) + (c2&0x3F);
			if (table==0||table==1||c1<0xA0)
				;
			else
			{
				if (!coding_table)
				{
					switch(table)
					{
						case 2:
							coding_table = c88592;
							break;
						case 3:
							coding_table = c88593;
							break;
						case 4:
							coding_table = c88594;
							break;
						case 5:
							coding_table = c88595;
							break;
						case 6:
							coding_table = c88596;
							break;
						case 7:
							coding_table = c88597;
							break;
						case 8:
							coding_table = c88598;
							break;
						case 9:
							coding_table = c88599;
							break;
						case 10:
							coding_table = c885910;
							break;
						case 11:
							coding_table = c885911;
							break;
/*				case 12:   // reserved.. for indian use
						coding_table = c885912;
						break;*/ 
						case 13:
							coding_table = c885913;
							break;
						case 14:
							coding_table = c885914;
							break;
						case 15:
							coding_table = c885915;
							break;
						case 16:
							coding_table = c885916;
							break;
						default:
							eFatal("unknown coding table %d", table);
							break;
					}
				}
				for(unsigned int j=0;j<96;j++)
				{       
					if(coding_table[j]==c)
					{
						c=0xA0+j;
						break;
					}
				}
			}
		}
		buf[t++]=(unsigned char)c;
	}
	return eString((char*)buf,t);
}

eString convertLatin1UTF8(const eString &string)
{
	unsigned int t=0, i=0, len=string.size();

	unsigned char res[2048];

	while (i < len)
	{
		unsigned long code=string[i++];
				// Unicode->UTF8 encoding
		if (code < 0x80) // identity latin <-> utf8 mapping
			res[t++]=char(code);
		else if (code < 0x800) // two byte mapping
		{
			res[t++]=(code>>6)|0xC0;
			res[t++]=(code&0x3F)|0x80;
		} else if (code < 0x10000) // three bytes mapping
		{
			res[t++]=(code>>12)|0xE0;
			res[t++]=((code>>6)&0x3F)|0x80;
			res[t++]=(code&0x3F)|0x80;
		} else
		{
			res[t++]=(code>>18)|0xF0;
			res[t++]=((code>>12)&0x3F)|0x80;
			res[t++]=((code>>6)&0x3F)|0x80;
			res[t++]=(code&0x3F)|0x80;
		}
		if (t+4 > 2047)
		{
			eDebug("convertLatin1UTF8 buffer to small.. break now");
			break;
		}
	}
	return eString((char*)res, t);
}

int isUTF8(const eString &string)
{
	unsigned int len=string.size();
	
	for (unsigned int i=0; i < len; ++i)
	{
		if (!(string[i]&0x80)) // normal ASCII
			continue;
		if ((string[i] & 0xE0) == 0xC0) // one char following.
		{
				// first, length check:
			if (i+1 >= len)
				return 0; // certainly NOT utf-8
			i++;
			if ((string[i]&0xC0) != 0x80)
				return 0; // no, not UTF-8.
		} else if ((string[i] & 0xF0) == 0xE0)
		{
			if ((i+1) >= len)
				return 0;
			i++;
			if ((string[i]&0xC0) != 0x80)
				return 0;
			i++;
			if ((string[i]&0xC0) != 0x80)
				return 0;
		}
	}
	return 1; // can be UTF8 (or pure ASCII, at least no non-UTF-8 8bit characters)
}

int isSpaceChar(char ch)
{
	unsigned char chin=(unsigned char)ch;
	return (chin<=' ');
}

