

#ifndef CRG4UTILITY_H_
#define CRG4UTILITY_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <vector>


#include "md5.h"



typedef struct T_SYSTEMTIME {
	unsigned short 	wYear;
	unsigned short 	wMonth;
	unsigned short 	wDayOfWeek;
	unsigned short 	wDay;
	unsigned short 	wHour;
	unsigned short 	wMinute;
	unsigned short 	wSecond;
	unsigned short 	wMilliseconds;
} SYSTEMTIME_;



#define MAX_CHARACTER 1024
#define KEY_VALUE_MAX 1001
#define MEM_SIZE	1024
#define MEM_HALT	512

using namespace std;

namespace BAYCOM
{
	namespace CONF
	{
		char 	GetKeyValue(char*key_value,char *key_out,char*value_out);
		//"aa bb cc dd",retern value point to tail of the word
		char *	GetNextWord(char*value,char *word_out);
		//format: "aa bb cc dd",retern value point to tail of the word
		char *	GetNextNWord(char*value,char *word_out,int size);
		//GetNextParameter format : "a=1&b=2&c=3&d=4"
		char *	GetNextParameter(char*input,char *key_out,char*value_out);
		void 	getConfigureString(char *filename,char*key,char *output,char*defaul);
		void 	getProcStatusString(char *filename,char*key,char *output,char*defaul);
		void 	setConfigureString(char*filename,char*key,char*value);
		void 	RemoveKey(char*para,FILE*fp);
		int 	findStrInFile(char *filename, int offset, unsigned char *str, int str_len);
		void *	getMemInFile(char *filename, int offset, int len);
		int 	findMarkerInFile(char *filename, int offset, unsigned char *str,int str_len);
		char *	ltrim(char*buf);
		char *	ltrim_all(char*buf);
		void 	rtrim(char*buf);
		char *	trim(char*buf);
	};
	namespace HTTP
	{
		string ComposeHttpCommand(string szCmd, string szPara, string szMethod, string strIP, int nPort, string strUser, string strPass);
		string ComposeHttpCommand(string szCmd, string szPara, string szMethod, string strIP, int nPort, string strUser, string strPass, string strRefer);
		/*
		RTSP/1.0 200 OK
		CSeq: 4
		Transport: RTP/AVP/TCP;unicast;interleaved=2-3
		Session: 9016221062;timeout=60
		*/
		unsigned int GetHTTPResponseCode(string strHTML);
		/* *
		 * Not a formal/full supported version, now only support:
		 * %20 --> BLANK
		 * &amp; --> &
		 * &gt; --> >
		 * &lt; --> <
		 * */
		string URLDecode(string strURL);
	};


	namespace JPEG2
	{
		struct jpeghdr {
			//unsigned int tspec:8;   /* type-specific field */
			unsigned int off:32;    /* fragment byte offset */
			uint8_t type;            /* id of jpeg decoder params */
			uint8_t q;               /* quantization factor (or table id) */
			uint8_t width;           /* frame width in 8 pixel blocks */
			uint8_t height;          /* frame height in 8 pixel blocks */
		};

		struct jpeghdr_rst {
			uint16_t dri;
			unsigned int f:1;
			unsigned int l:1;
			unsigned int count:14;
		};

		struct jpeghdr_qtable {
			uint8_t  mbz;
			uint8_t  precision;
			uint16_t length;
		};

		u_char *MakeQuantHeader(u_char *p, u_char *qt, int tableNo, int table_len);
		u_char *MakeHuffmanHeader(u_char *p, u_char *codelens, int ncodes, u_char *symbols, int nsymbols, int tableNo, int tableClass);
		u_char *MakeDRIHeader(u_char *p, u_short dri);

		/*
		 *  Arguments:
		 *    type, width, height: as supplied in RTP/JPEG header
		 *    lqt, cqt: quantization tables as either derived from
		 *         the Q field using MakeTables() or as specified
		 *         in section 4.2.
		 *    dri: restart interval in MCUs, or 0 if no restarts.
		 *
		 *    p: pointer to return area
		 *
		 *  Return value:
		 *    The length of the generated headers.
		 *
		 *    Generate a frame and scan headers that can be prepended to the
		 *    RTP/JPEG data payload to produce a JPEG compressed image in
		 *    interchange format (except for possible trailing garbage and
		 *    absence of an EOI marker to terminate the scan).
		 */
		int MakeHeaders(u_char *p, int type, int w, int h, u_char *lqt, u_char *cqt, unsigned table_len, u_short dri);
		/*
		 * Call MakeTables with the Q factor and two u_char[64] return arrays
		 */
		void MakeTables(int q, u_char *lqt, u_char *cqt);
	}

	namespace JPEG
	{
		typedef struct T_JPEG_HEADER
		{
			char			TypeSpecific;
			char			nOffset[3];
			unsigned char	type;
			unsigned char	Q;
			unsigned char	nWidth;
			unsigned char	nHeight;
		}JPEG_HEADER;

		typedef struct T_JPEG_HEADER_RST
		{
			unsigned short	DRI;
		}JPEG_HEADER_RST;
		void 			MakeTables(int q, u_char *lum_q, u_char *chr_q);
		unsigned char* 	MakeHuffmanHeader(unsigned char* p, unsigned char* codelens, int ncodes,unsigned char* symbols, int nsymbols, int tableNo,int tableClass);
		unsigned char* 	MakeQuantHeader(unsigned char* p, unsigned char* qt, int tableNo);
		int 			MakeJpegHeader(unsigned char* p, int type, int w, int h, unsigned char* lqt, unsigned char* cqt, short dri);
		unsigned char* 	MakeDRIHeader(unsigned char* p, short dri);
		void 			GetJpegResolution(unsigned char* buf, int len, int &width, int &height);
	}
	namespace MPEG4
	{
		bool 	CheckVOL(unsigned char* pBuffer, unsigned nSize);
		//nFrameType: 	0 --> I frame
		//				1 --> B/P frame
		bool 	CheckVOP(unsigned char* pBuffer, unsigned nSize, int &nFrameType);
		bool 	GetMPEG4Resolution(unsigned char* pBuffer, int nSize, int &nWidth, int &nHeight);
	}
	namespace H264
	{
		int 	GetNALUType(unsigned char* pBuffer,int& nFrameBegin,int& nFrameEnd,int& nFrameType);
		bool 	GetSPSPPS(char* pDescribe, unsigned char** sps, int& sps_size, unsigned char** pps, int& pps_size);
		void 	GetH264Resolution(unsigned char *pInBuf, int nSize, int &nWidth, int &nHeight, unsigned int &nNum_Ref_Frames);
		void 	MyShift(int nCount,int&nIndex,int &nShiftCount,unsigned char &nShiftBuffer,unsigned int &nRecv,unsigned char *pInBuf);
		void 	SE_V(int &nIndex,int &nShiftCount,unsigned char &nShiftBuffer,int &nRecv, unsigned char *pInBuf);
		void 	UE_V(int &nIndex,int &nShiftCount,unsigned char &nShiftBuffer,unsigned int &nRecv,unsigned char *pInBuf);
	}
	namespace RTSP
	{
		int 	GetPayloadType(char *pszStreamName,char * pszRtpMap, int& nVideoType, int& nAudioType);
		int 	GetPayloadType_unisvr(char *pszStreamName,char * pszRtpMap, int& nVideoType, int& nAudioType);
		void 	GetPayloadTypeTag(char *pszStreamID, unsigned short& nVideoPT, unsigned short& nAudioPT);
		string 	ComposeRtspCommand(string szCmdID, string szURL, int nSeq, string szAuth, string szTransport, string strSession, string sztrackerURL, int nAuthType);
		string 	ComposeRtspCommand_old(string szCmdID, string szURL, int nSeq, string szAuth, string szTransport, string szSession, string sztrackerURL, int nAuthType);
		int 	GetAudioFormatByCodeID(int codecid, int& channels, int& bitspersample, int& samplerate, int& bitrate);
		void 	GenerateSSRC(int& nSSRC);
		//buffer pointer sps && pps should be allocated first, and with a buffer large enough(e.g. 1024 bytes)
		bool 	GetSPSPPSfromRTSPdescribe(const char* pDescribe, unsigned char* sps, int& sps_size, unsigned char* pps, int &pps_size);
		bool	GetRTSPReturnCode(string szData, int nSize, int& nCode);
	}

	namespace util
	{
		string& toLower(string& s);
		string& toUpper(string& s);

		string 	GetTagValue(string szContent, string szTagName);
		string	GetTagValueEx(string szContent, string szTagName, string szEndTag);
		string 	GetXMLValueByTag(string strXML, string strTag);

		time_t 	TimeStr_To_TimeT(const char* strTime);
		bool 	TimeT_To_TimeStr(time_t time1, char* strTime);
		string 	RsFormat1(const char *fmt, ...);
		string 	RsFormat2(const char* fmt, ...);
		string 	RsFormat(const wchar_t *fmt, ...);

		void	RsSplit(const string& src, const string& separator, vector<string>& dest);
		string	RsFormat(const char* fmt, ...);

		//difference between RsReplace_all & RsReplace_all_distinct
		//	replace_all(string("12212"),"12","21")  --> 22211
		//	replace_all_distinct(string("12212"),"12","21")  --> 21221
		//view http://www.rosoo.net/a/201209/16270.html for more info
		string	RsReplace_all(string& str,const string& old_value,const string& new_value);
		string	RsReplace_all_distinct(string& str,const string& old_value,const string& new_value);
		string	RsReplace2(string strInput, const char* pFind, const char* pRepl);
		string	GetRandomCode();

		void 	GetLocalTime_(SYSTEMTIME_* pTime);
		time_t 	SystimeToTime_t(SYSTEMTIME_* pTime);
		void 	Time_tToSystime(time_t* tTime, SYSTEMTIME_* pTime);
		int 	CompareDateTime(const SYSTEMTIME_& p1stDateTime, const SYSTEMTIME_& p2ndDateTime);
		/**
			*   计算两个时间的间隔，得到时间差
			*   @param   struct   timeval*   resule   返回计算出来的时间
			*   @param   struct   timeval*   x             需要计算的前一个时间
			*   @param   struct   timeval*   y             需要计算的后一个时间
			*   return   -1   failure   ,0   success
		**/
		int timeval_subtract(struct timeval* result, struct timeval* start, struct timeval* stop);
		bool SystemTimeAdd(SYSTEMTIME_* pSysTime, unsigned int nMilliSeconds);
	};

	namespace sys
	{
		bool 	isDirectoryExisting(const char* strDir);
		bool 	CreateDirectory(const char* strFolder, void* pAttr);
		bool 	RemoveDirectory(const char* strFolder);
		bool 	isDirEmpty(const char* strFolder);
		bool 	IsFileExisted(const char* szFileName);

		char 	getFileDrive(const char* filename);		//windows only
		u_int64_t getFileSize(const char* filename);
		bool 	isDirectory(const char* filename);
		bool 	IsDiskFull(const char* path);

		bool 	DeleteFile(const char* strFileName);
		bool 	GetRecordingFileSizeFromHDD(const char* szFileName, u_int64_t& nFileSize);
		bool 	GetFolderSize_to_be_coded(const char* strFolder, u_int64_t& nTotalSize);

		string 	getModifiedTime(string filename);
		string 	getCreateTime(string filename);
		//u_int64_t getFileSizeByHandle(FILE* fp);

		bool 	GetDiskFreeSpaceEx(const char* strPath, u_int64_t& FreeBytesAvailable, u_int64_t& TotalNumberOfBytes, u_int64_t& TotalNumberOfFreeBytes);

		//void find_file( string strDir );
	};

	namespace algorithm
	{
		string base64_encode(unsigned char const* , unsigned int len);
		string base64_decode(std::string const& s);

		///////////////////////////////////////
		//MD5 encrypt for HTTP digit authorization
		//(can also apply to RTSP protocal)
		class CEzMD5
		{
#define HASHLEN 16
typedef char HASH[HASHLEN];
#define HASHHEXLEN 32
typedef char HASHHEX[HASHHEXLEN+1];
#define IN
#define OUT
		public:
			CEzMD5();
			~CEzMD5();
		public:
			static string	EncryptAuthMD5(const char* szMethod, const char* szRealm, const char* szNonce, const char* szUser, const char* szPwd, const char* szURI, const char* szQop);
		private:
			static void		CvtHex(IN HASH Bin,   OUT HASHHEX Hex );
			/* calculate H(A1) as per HTTP Digest spec */
			static void		DigestCalcHA1(
				IN char * pszAlg,
				IN char * pszUserName,
				IN char * pszRealm,
				IN char * pszPassword,
				IN char * pszNonce,
				IN char * pszCNonce,
				OUT HASHHEX SessionKey
				);

			/* calculate request-digest/response-digest as per HTTP Digest spec */
			static void		DigestCalcResponse(
				IN HASHHEX HA1,           /* H(A1) */
				IN char * pszNonce,       /* nonce from server */
				IN char * pszNonceCount,  /* 8 hex digits */
				IN char * pszCNonce,      /* client nonce */
				IN char * pszQop,         /* qop-value: "", "auth", "auth-int" */
				IN char * pszMethod,      /* method from the request */
				IN char * pszDigestUri,   /* requested URL */
				IN HASHHEX HEntity,       /* H(entity body) if qop="auth-int" */
				OUT HASHHEX Response      /* request-digest or response-digest */
				);
		};

		class CBase64
		{
			static unsigned char DecToHex(unsigned char B);		//ÎªQuoted±àÂë²Ù×÷œøÐÐ×Ö·û×ª»»
			static unsigned char HexToDec(unsigned char C);		//ÎªQuotedœâÂë²Ù×÷œøÐÐ×Ö·û×ª»»
		public:
			static unsigned int m_LineWidth;						//Öž¶š±àÂëºóÃ¿ÐÐµÄ³€¶È£¬È±Ê¡ÊÇ76
			static const char BASE64_ENCODE_TABLE[64];				//Base64±àÂë±í
			static const unsigned int BASE64_DECODE_TABLE[256];		//Base64œâÂë±í
			static const unsigned char QUOTED_ENCODE_TABLE[256];	//Quoted±àÂë±í


			static int Base64EncodeSize(int iSize);		//žùŸÝÎÄŒþÊµŒÊ³€¶È»ñÈ¡±àÂëBase64ºóµÄ³€¶È
			static int Base64DecodeSize(int iSize);		//žùŸÝÒÑ±àÂëÎÄŒþ³€¶È»ñÈ¡Base64µÄœâÂë³€¶È
			static int UUEncodeSize(int iSize);			//žùŸÝÎÄŒþÊµŒÊ³€¶È»ñÈ¡UUCode±àÂëºóµÄ³€¶È
			static int UUDecodeSize(int iSize);			//žùŸÝÒÑ±àÂëÎÄŒþ³€¶È»ñÈ¡UUCodeœâÂëºóµÄ³€¶È
			static int QuotedEncodeSize(int iSize);		//žùŸÝÊµŒÊÎÄŒþµÄ³€¶È»ñÈ¡Quoted±àÂë

			static int base64_encode(char *pSrc, unsigned int nSize, char *pDest);

			static int  base64_decode(char *pSrc, unsigned int nSize, char *pDest);

			static int UU_encode(char *pSrc, unsigned int nSize, char *pDest);

			static int UU_decode(char *pSrc, unsigned int nSize, char *pDest);

			static int quoted_encode(char *pSrc, unsigned int nSize, char *pDest);

			static int quoted_decode(char *pSrc, unsigned int nSize, char *pDest);
		};
	}

	namespace NET
	{

#define PRINT_SOCKET_ERROR(x) perror(x)
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 			64
#endif

		int create_and_bind (int nPort);
		int make_socket_reuse_addr(int fd);
		int make_socket_non_blocking (int sfd);
		int make_socket_recv_timeout(int fd, int msec);
		int make_socket_send_timeout(int fd, int msec);

		/* connecthostport()
		 * return a socket connected (TCP) to the host and port
		 * or -1 in case of error */
		int connecthostport(const char * host, unsigned short port);

		/*
		 * Return :
		 * 0: peer closed
		 * -1: normal error, can get from errno
		 * -2: select error
		 * -3: FD_SET timeout
		 */
		int Receive(int hSocket, char* pBuffer, int nLen, struct timeval* timeout);

		int GetLocalMac(unsigned char *vmac, char* ETH_NAME /* = "eth0" */);
		int GetLocalMac2(unsigned char *vmac, char* ETH_NAME /* = "eth0" */);
		int GetLocalAddress(char *szIPAddr, char* ETH_NAME /* = "eth0" */,char * def);
		int GetLocalMask(unsigned char *vmask, char* ETH_NAME);
		int GetLocalGateway(unsigned char *vgate,char * ETH_NAME);
		char* GetLocalHostName(char* hostname, int nMaxSize);
		int GetIP_v4_and_v6_linux(int family, char *address, int size);
		bool CheckNETDev(char*dev_name);
		void GetGateway(char*dev_name,char*output,char*defaul);
		void GetDns(char*dev_name,char*dns1,char*dns2,char*defaul);
		char * getHostNameByIP();
		string GetExternalIP();

	};
}

#endif /* CRSUTILITY_H_ */
