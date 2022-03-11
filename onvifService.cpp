#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <errno.h>
#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/tcp.h>
#else
#include <winsock2.h>
#endif
#include "onvif.h"
#include "onvifService.h"
#include "deviceinformation.nsmap"
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
/*#include "networkinterfaces.nsmap"
#include "getdns.nsmap"
#include "getscopes.nsmap"
#include "getcapabilities.nsmap"
#include "getservices.nsmap"
#include "wsdlgetvideosources.nsmap"
#include "getprofiles.nsmap"
#include "getsnapshoturi.nsmap"
#include "wsdlgetprofile.nsmap"
#include "getstreamuri.nsmap"
#include "getvideosourceconfiguration.nsmap"
#include "getterminationtime.nsmap"*/


//FIXME remove param.h,ptz.h,vadcDrv.h
/*#include "../param.h"
#include "../ptz.h"
#include "../vadcDrv.h"
*/
//FIXME Add common.h
#include "common.h"

const char *http_ok = "HTTP/1.1 200 OK";
const char *http_server= "gSOAP/2.8";
const char *http_connection = "close";
const char *http_fail405="HTTP/1.1 405";
const char *http_notallowed="Method Not Allowed";
extern Onvif_Server g_OnvifServer;
extern unsigned short g_wOnvifPort;
unsigned int g_OnvifServiceRunning = 1;
//FIXME remove SYS_PARAM
//extern SYS_PARAM g_sys_param;
int GetLocalAddress(char *szIPAddr, char* ETH_NAME /* = "eth0" */, char * def)
{
	int   sock;
	struct   sockaddr_in   sin;
//	struct   sockaddr   sa;
	struct   ifreq   ifr;
	sock = socket(AF_INET,SOCK_DGRAM,0);
	if(sock == -1)
	{
		close(sock);
		sprintf(szIPAddr,"%s",def);
		printf("Error:get local IP socket fail!\n");
		return  -1;
	}
	strncpy(ifr.ifr_name, ETH_NAME,IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ - 1] = 0;
	if(ioctl(sock, SIOCGIFADDR, &ifr) < 0)
	{
		close(sock);
		sprintf(szIPAddr,"%s",def);
		printf("Error:get local IP ioctl fail!");
		return  -1;
	}
	memcpy(&sin,   &ifr.ifr_addr,   sizeof(sin));
	sprintf(szIPAddr,"%s",inet_ntoa(sin.sin_addr));
	close(sock);
	return  0;
}



void ONVIF_GETMAC(unsigned char * macaddr, char * eth_device)
{
	int sock;
		//	int i;
		struct sockaddr_in sin;
		struct sockaddr sa;
		struct ifreq ifr;
		//unsigned char mac[6];

		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock == -1)
		{
			close(sock);
			perror("socket");
			return ;
		}

		strncpy(ifr.ifr_name, eth_device, IFNAMSIZ);
		ifr.ifr_name[IFNAMSIZ - 1] = 0;

		memset(macaddr, 0, sizeof(macaddr));
		if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0)
		{
			close(sock);
			perror("ioctl");
			return ;
		}

		memcpy(&sa, &ifr.ifr_addr, sizeof(sin));
		memcpy(macaddr,sa.sa_data,6);
		//	sprintf((char*)vmac,"%0.2X:%0.2X:%0.2X:%0.2X:%0.2X:%0.2X\n",mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		//sprintf((char*)vmac,"%02X:%02X:%02X:%02X:%02X:%02X",mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		close(sock);
}

int BuildHttpHeaderString(const struct Http_Buffer *pBuf,char *sBuffer,int nLen)
{
	if(pBuf->nHasAction)
		sprintf(sBuffer,"%s\r\nServer: %s\r\nContent-Type: application/soap+xml; charset=utf-8; action=\"%s\"\r\nContent-Length: %d\r\nConnection: %s\r\n\r\n",http_ok,http_server,pBuf->action,nLen,http_connection);
	else
		sprintf(sBuffer,"%s\r\nServer: %s\r\nContent-Type: application/soap+xml; charset=utf-8;\r\nContent-Length: %d\r\nConnection: %s\r\n\r\n",http_ok,http_server,nLen,http_connection);
	return strlen(sBuffer);
}

int BuildHttpFailHeaderString(const struct Http_Buffer *pBuf,char *sBuffer,int nLen)
{
	if(pBuf->nHasAction)
		sprintf(sBuffer,"%s %s\r\nServer: %s\r\nContent-Type: application/soap+xml; charset=utf-8; action=\"%s\"\r\nContent-Length: %d\r\nConnection: %s\r\n\r\n",http_fail405,http_notallowed,http_server,pBuf->action,nLen,http_connection);
	else
		sprintf(sBuffer,"%s %s\r\nServer: %s\r\nContent-Type: application/soap+xml; charset=utf-8;\r\nContent-Length: %d\r\nConnection: %s\r\n\r\n",http_fail405,http_notallowed,http_server,nLen,http_connection);
	return strlen(sBuffer);
}

int BuildDevInfoHeaderString(char *sBuffer,struct Namespace *pNamespaces)
{
	return BuildCommonHeaderString(sBuffer,pNamespaces);
}

int BuildDeviceInfoString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char configbuff[1024];
	unsigned char mac[6];
	ONVIF_GETMAC(mac,"wlan0");
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<tds:GetDeviceInformationResponse>");
	strcat(sBuffer,"<tds:Manufacturer>Broadcom</tds:Manufacturer>");
	memset(configbuff,0,sizeof(configbuff));
	sprintf(configbuff,"RaspberryPi");
	sprintf(TmpBuffer,"<tds:Model>%s</tds:Model>",configbuff);
	strcat(sBuffer,TmpBuffer);
	//FIXME FirmwareVersion
	memset(configbuff,0,sizeof(configbuff));
	sprintf(configbuff,"0.0.1");
	sprintf(TmpBuffer,"<tds:FirmwareVersion>%s</tds:FirmwareVersion>",configbuff);
	strcat(sBuffer,TmpBuffer);
	//FIXME DeviceID
	sprintf(TmpBuffer,"<tds:SerialNumber>%02X%02X%02X%02X%02X%02X</tds:SerialNumber>",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);//,SYSTEM_HWVER);
	strcat(sBuffer,TmpBuffer);
	//FIXME HWID
	memset(configbuff,0,sizeof(configbuff));
	sprintf(configbuff,"5068");
	sprintf(TmpBuffer,"<tds:HardwareId>%s</tds:HardwareId>",configbuff);//,SYSTEM_HWVER);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tds:GetDeviceInformationResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}

int BuildNetworkInterfaceString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char localIP[16];
	char configbuff[1024];
	unsigned char mac[6];
	ONVIF_GETMAC(mac,"wlan0");
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	GetLocalAddress(localIP,"wlan0",(char *)"127.0.0.1");
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<tds:GetNetworkInterfacesResponse>");
	strcat(sBuffer,"<tds:NetworkInterfaces token=\"eth2\">");
	strcat(sBuffer,"<tt:Enabled>true</tt:Enabled>");
	strcat(sBuffer,"<tt:Info>");
	strcat(sBuffer,"<tt:Name>eth2</tt:Name>");
	//FIXME MacAddress
	sprintf(TmpBuffer,"<tt:HwAddress>%02X:%02X:%02X:%02X:%02X:%02X</tt:HwAddress>",mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:MTU>1500</tt:MTU>");
	strcat(sBuffer,"</tt:Info>");
	strcat(sBuffer,"<tt:Link>");
	strcat(sBuffer,"<tt:AdminSettings>");
	strcat(sBuffer,"<tt:AutoNegotiation>true</tt:AutoNegotiation>");
	strcat(sBuffer,"<tt:Speed>100</tt:Speed>");
	strcat(sBuffer,"<tt:Duplex>Full</tt:Duplex>");
	strcat(sBuffer,"</tt:AdminSettings>");
	strcat(sBuffer,"<tt:OperSettings>");
	strcat(sBuffer,"<tt:AutoNegotiation>true</tt:AutoNegotiation>");
	strcat(sBuffer,"<tt:Speed>100</tt:Speed>");
	strcat(sBuffer,"<tt:Duplex>Half</tt:Duplex>");
	strcat(sBuffer,"</tt:OperSettings>");
	strcat(sBuffer,"<tt:InterfaceType>0</tt:InterfaceType>");
	strcat(sBuffer,"</tt:Link>");
	strcat(sBuffer,"<tt:IPv4>");
	strcat(sBuffer,"<tt:Enabled>true</tt:Enabled>");
	strcat(sBuffer,"<tt:Config>");
	//strcat(sBuffer,"<tt:Manual>");
	strcat(sBuffer,"<tt:FromDHCP>");
	sprintf(TmpBuffer,"<tt:Address>%s</tt:Address>",localIP);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:PrefixLength>23</tt:PrefixLength>");
	strcat(sBuffer,"</tt:FromDHCP>");
	//strcat(sBuffer,"</tt:Manual>");
	//FIXME hard code DHCP
	/*if(g_sys_param.network.nDhcpOnFlag)*/
	memset(configbuff,0,sizeof(configbuff));
	sprintf(configbuff,"true");
	sprintf(TmpBuffer,"<tt:DHCP>%s</tt:DHCP>",configbuff);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tt:Config>");
	strcat(sBuffer,"</tt:IPv4>");
	strcat(sBuffer,"</tds:NetworkInterfaces>");
	strcat(sBuffer,"</tds:GetNetworkInterfacesResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);

}

int BuildGetDNSString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char DNSI[512];
	char DNSII[512];
	char configbuff[512];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	sprintf(DNSI,"8.8.8.8");
	sprintf(DNSII,"169.95.1.1");
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");

	strcat(sBuffer,"<tds:GetDNSResponse>");
	strcat(sBuffer,"<tds:DNSInformation>");
	memset(configbuff,0,sizeof(configbuff));
	sprintf(configbuff,"true");
	sprintf(TmpBuffer,"<tt:FromDHCP>%s</tt:FromDHCP>",configbuff);
	strcat(sBuffer,"<tt:DNSManual>");
	strcat(sBuffer,"<tt:Type>IPv4</tt:Type>");
	//FIXME hard code address
	sprintf(TmpBuffer,"<tt:IPv4Address>%s</tt:IPv4Address>",DNSI);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tt:DNSManual>");
	strcat(sBuffer,"</tds:DNSInformation>");
	strcat(sBuffer,"</tds:GetDNSResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);

}

int BuildGetScopesString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char configbuff[1024];
	//Device name
	memset(configbuff,0,sizeof(configbuff));
	sprintf(configbuff,"RaspberryPi");
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");

	strcat(sBuffer,"<tds:GetScopesResponse>");
	strcat(sBuffer,"<tds:Scopes>");
	strcat(sBuffer,"<tt:ScopeDef>Fixed</tt:ScopeDef>");
	strcat(sBuffer,"<tt:ScopeItem>onvif://www.onvif.org/type/video_encoder</tt:ScopeItem>");
	strcat(sBuffer,"</tds:Scopes>");
	//FIXME remove ptz support
	/*strcat(sBuffer,"<tds:Scopes>");
	strcat(sBuffer,"<tt:ScopeDef>Fixed</tt:ScopeDef>");
	strcat(sBuffer,"<tt:ScopeItem>onvif://www.onvif.org/type/ptz</tt:ScopeItem>");
	strcat(sBuffer,"</tds:Scopes>");*/
	strcat(sBuffer,"<tds:Scopes>");
	strcat(sBuffer,"<tt:ScopeDef>Fixed</tt:ScopeDef>");
	strcat(sBuffer,"<tt:ScopeItem>onvif://www.onvif.org/Profile/Streaming</tt:ScopeItem>");
	strcat(sBuffer,"</tds:Scopes>");
	strcat(sBuffer,"<tds:Scopes>");
	strcat(sBuffer,"<tt:ScopeDef>Fixed</tt:ScopeDef>");
	strcat(sBuffer,"<tt:ScopeItem>onvif://www.onvif.org/type/audio_encoder</tt:ScopeItem>");
	strcat(sBuffer,"</tds:Scopes>");
	//FIXME remove location
	strcat(sBuffer,"<tds:Scopes>");
	strcat(sBuffer,"<tt:ScopeDef>Fixed</tt:ScopeDef>");
	strcat(sBuffer,"<tt:ScopeItem>onvif://www.onvif.org/location/</tt:ScopeItem>");
	strcat(sBuffer,"</tds:Scopes>");
	strcat(sBuffer,"<tds:Scopes>");
	strcat(sBuffer,"<tt:ScopeDef>Fixed</tt:ScopeDef>");
	strcat(sBuffer,"<tt:ScopeItem>onvif://www.onvif.org/hardware/5</tt:ScopeItem>");
	strcat(sBuffer,"</tds:Scopes>");
	strcat(sBuffer,"<tds:Scopes>");
	strcat(sBuffer,"<tt:ScopeDef>Fixed</tt:ScopeDef>");
	sprintf(TmpBuffer,"<tt:ScopeItem>onvif://www.onvif.org/name/%s</tt:ScopeItem>",configbuff);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tds:Scopes>");
	strcat(sBuffer,"</tds:GetScopesResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);

}

int BuildGetCapabilitiesString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char localIP[16];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	GetLocalAddress(localIP,"wlan0",(char *)"127.0.0.1");
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");

	strcat(sBuffer,"<tds:GetCapabilitiesResponse>");
	strcat(sBuffer,"<tds:Capabilities>");
	strcat(sBuffer,"<tt:Device>");
	if(g_wOnvifPort == 80)
		sprintf(TmpBuffer,"<tt:XAddr>http://%s/onvif/Device_service</tt:XAddr>",localIP);
	else
		sprintf(TmpBuffer,"<tt:XAddr>http://%s:%d/onvif/Device_service</tt:XAddr>",localIP,g_wOnvifPort);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:Network>");
	strcat(sBuffer,"<tt:IPFilter>false</tt:IPFilter>");
	strcat(sBuffer,"<tt:ZeroConfiguration>false</tt:ZeroConfiguration>");
	strcat(sBuffer,"<tt:IPVersion6>false</tt:IPVersion6>");
	strcat(sBuffer,"<tt:DynDNS>false</tt:DynDNS>");
	strcat(sBuffer,"</tt:Network>");
	strcat(sBuffer,"<tt:System>");
	strcat(sBuffer,"<tt:DiscoveryResolve>true</tt:DiscoveryResolve>");
	strcat(sBuffer,"<tt:DiscoveryBye>true</tt:DiscoveryBye>");
	strcat(sBuffer,"<tt:RemoteDiscovery>false</tt:RemoteDiscovery>");
	strcat(sBuffer,"<tt:SystemBackup>false</tt:SystemBackup>");
	strcat(sBuffer,"<tt:SystemLogging>false</tt:SystemLogging>");
	strcat(sBuffer,"<tt:FirmwareUpgrade>false</tt:FirmwareUpgrade>");
	strcat(sBuffer,"<tt:SupportedVersions><tt:Major>1</tt:Major><tt:Minor>0</tt:Minor></tt:SupportedVersions>");
	strcat(sBuffer,"<tt:SupportedVersions><tt:Major>2</tt:Major><tt:Minor>0</tt:Minor></tt:SupportedVersions>");
	strcat(sBuffer,"</tt:System>");
	strcat(sBuffer,"<tt:IO>");
	strcat(sBuffer,"<tt:InputConnectors>1</tt:InputConnectors>");
	strcat(sBuffer,"<tt:RelayOutputs>1</tt:RelayOutputs>");
	strcat(sBuffer,"</tt:IO>");
	strcat(sBuffer,"<tt:Security>");
	strcat(sBuffer,"<tt:TLS1.1>false</tt:TLS1.1><tt:TLS1.2>false</tt:TLS1.2>");
	strcat(sBuffer,"<tt:OnboardKeyGeneration>false</tt:OnboardKeyGeneration>");
	strcat(sBuffer,"<tt:AccessPolicyConfig>false</tt:AccessPolicyConfig>");
	strcat(sBuffer,"<tt:X.509Token>false</tt:X.509Token>");
	strcat(sBuffer,"<tt:SAMLToken>false</tt:SAMLToken>");
	strcat(sBuffer,"<tt:KerberosToken>false</tt:KerberosToken>");
	strcat(sBuffer,"<tt:RELToken>false</tt:RELToken>");
	strcat(sBuffer,"</tt:Security>");
	strcat(sBuffer,"</tt:Device>");
	strcat(sBuffer,"<tt:Events>");
	if(g_wOnvifPort == 80)
		sprintf(TmpBuffer,"<tt:XAddr>http://%s/onvif/Events_service</tt:XAddr>",localIP);
	else
		sprintf(TmpBuffer,"<tt:XAddr>http://%s:%d/onvif/Events_service</tt:XAddr>",localIP,g_wOnvifPort);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:WSSubscriptionPolicySupport>false</tt:WSSubscriptionPolicySupport>");
	strcat(sBuffer,"<tt:WSPullPointSupport>false</tt:WSPullPointSupport>");
	strcat(sBuffer,"<tt:WSPausableSubscriptionManagerInterfaceSupport>false</tt:WSPausableSubscriptionManagerInterfaceSupport>");
	strcat(sBuffer,"</tt:Events>");
	if(g_wOnvifPort == 80)
		sprintf(TmpBuffer,"<tt:Imaging><tt:XAddr>http://%s/onvif/Imaging_service</tt:XAddr></tt:Imaging>",localIP);
	else
		sprintf(TmpBuffer,"<tt:Imaging><tt:XAddr>http://%s:%d/onvif/Imaging_service</tt:XAddr></tt:Imaging>",localIP,g_wOnvifPort);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:Media>");
	if(g_wOnvifPort == 80)
		sprintf(TmpBuffer,"<tt:XAddr>http://%s/onvif/Media_service</tt:XAddr>",localIP);
	else
		sprintf(TmpBuffer,"<tt:XAddr>http://%s:%d/onvif/Media_service</tt:XAddr>",localIP,g_wOnvifPort);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:StreamingCapabilities>");
	strcat(sBuffer,"<tt:RTPMulticast>false</tt:RTPMulticast>");
	strcat(sBuffer,"<tt:RTP_TCP>true</tt:RTP_TCP>");
	strcat(sBuffer,"<tt:RTP_RTSP_TCP>false</tt:RTP_RTSP_TCP>");
	strcat(sBuffer,"</tt:StreamingCapabilities>");
	strcat(sBuffer,"<tt:Extension>");
	strcat(sBuffer,"<tt:ProfileCapabilities><tt:MaximumNumberOfProfiles>2</tt:MaximumNumberOfProfiles></tt:ProfileCapabilities>");
	strcat(sBuffer,"</tt:Extension>");
	strcat(sBuffer,"</tt:Media>");
	//FIXME remove PTZ
	/*if(g_wOnvifPort == 80)
		sprintf(TmpBuffer,"<tt:PTZ><tt:XAddr>http://%s/onvif/PTZ_service</tt:XAddr></tt:PTZ>",localIP);
	else
		sprintf(TmpBuffer,"<tt:PTZ><tt:XAddr>http://%s:%d/onvif/PTZ_service</tt:XAddr></tt:PTZ>",localIP,g_wOnvifPort);
	strcat(sBuffer,TmpBuffer);*/
	strcat(sBuffer,"</tds:Capabilities>");
	strcat(sBuffer,"</tds:GetCapabilitiesResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);

}

int BuildWsdlGetVideoSourcesString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char configbuf[1024];
	//H264Profile
	memset(configbuf,0,sizeof(configbuf));
	sprintf(configbuf,"1");
	int profile;
	profile=atoi(configbuf);
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");

	strcat(sBuffer,"<trt:GetVideoSourcesResponse>");
	strcat(sBuffer,"<trt:VideoSources token=\"0_VSC\">");
	//FIXME Hard code Width Height
	if(profile==1)
	{
		//BAYCOM::CONF::getConfigureString((char *)VIDEO_CONF,(char *)"H264FPSP1",configbuf,(char *)"");
		sprintf(configbuf,"25");
		sprintf(TmpBuffer,"<tt:Framerate>%s</tt:Framerate>",configbuf);
		strcat(sBuffer,TmpBuffer);
		sprintf(TmpBuffer,"<tt:Resolution><tt:Width>1280</tt:Width><tt:Height>720</tt:Height></tt:Resolution>");
		strcat(sBuffer,TmpBuffer);
	}
	else if(profile==2)
	{
		//BAYCOM::CONF::getConfigureString((char *)VIDEO_CONF,(char *)"H264FPSP2",configbuf,(char *)"");
		sprintf(configbuf,"25");
		sprintf(TmpBuffer,"<tt:Framerate>%s</tt:Framerate>",configbuf);
		strcat(sBuffer,TmpBuffer);
		sprintf(TmpBuffer,"<tt:Resolution><tt:Width>640</tt:Width><tt:Height>480</tt:Height></tt:Resolution>");
		strcat(sBuffer,TmpBuffer);
	}
	else if(profile==3)
	{
		//BAYCOM::CONF::getConfigureString((char *)VIDEO_CONF,(char *)"H264FPSP3",configbuf,(char *)"");
		sprintf(configbuf,"25");
		sprintf(TmpBuffer,"<tt:Framerate>%s</tt:Framerate>",configbuf);
		strcat(sBuffer,TmpBuffer);
		sprintf(TmpBuffer,"<tt:Resolution><tt:Width>320</tt:Width><tt:Height>240</tt:Height></tt:Resolution>");
		strcat(sBuffer,TmpBuffer);
	}
	strcat(sBuffer,"<tt:Imaging>");
	char configbuff[32];
	//BAYCOM::CONF::getConfigureString((char *)VIDEO_CONF,(char *)"H264Brightness",configbuff,(char *)"128");
	sprintf(configbuff,"128");
	sprintf(TmpBuffer,"<tt:Brightness>%s</tt:Brightness>",configbuff);
	strcat(sBuffer,TmpBuffer);
	//BAYCOM::CONF::getConfigureString((char *)VIDEO_CONF,(char *)"H264Saturation",configbuff,(char *)"128");
	sprintf(configbuff,"128");
	sprintf(TmpBuffer,"<tt:ColorSaturation>%s</tt:ColorSaturation>",configbuff);
	strcat(sBuffer,TmpBuffer);
	//BAYCOM::CONF::getConfigureString((char *)VIDEO_CONF,(char *)"H264Contrast",configbuff,(char *)"128");
	sprintf(configbuff,"128");
	sprintf(TmpBuffer,"<tt:Contrast>%s</tt:Contrast>",configbuff);
	strcat(sBuffer,TmpBuffer);
	//BAYCOM::CONF::getConfigureString((char *)VIDEO_CONF,(char *)"H264Sharpness",configbuff,(char *)"128");
	sprintf(configbuff,"128");
	sprintf(TmpBuffer,"<tt:Sharpness>%s</tt:Sharpness>",configbuff);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tt:Imaging>");
	strcat(sBuffer,"</trt:VideoSources>");
	strcat(sBuffer,"</trt:GetVideoSourcesResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);

}
int GetVideoEncoderConfigurationString( char *sBuffer,struct Namespace *pNameSpace )
{
	int nLen;
		char TmpBuffer[1024];
		char configbuf[1024];
		//BAYCOM::CONF::getConfigureString((char *)VIDEO_CONF,(char *)"H264Profile",configbuf,(char *)"1");
		sprintf(configbuf,"1");
		int profile;
		profile=atoi(configbuf);
		nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
		BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

		//start soap-env body
		strcat(sBuffer,"<SOAP-ENV:Body>");

		strcat(sBuffer,"<trt:GetVideoEncoderConfigurationResponse>");
		strcat(sBuffer,"<trt:Configuration>");
		strcat(sBuffer,"<tt:Name>0_VEC_main</tt:Name>");
		strcat(sBuffer,"<tt:token>0_VEC_main</tt:token>");
		strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
		strcat(sBuffer,"<tt:Encoding>H264</tt:Encoding>");
		strcat(sBuffer,"<tt:Resolution>");
		strcat(sBuffer,"<tt:Width>1280</tt:Width>");
		strcat(sBuffer,"<tt:Height>720</tt:Height>");
		strcat(sBuffer,"</tt:Resolution>");
		strcat(sBuffer,"<tt:Quality>3.000000</tt:Quality>");
		strcat(sBuffer,"<tt:RateControl>");
		strcat(sBuffer,"<tt:FrameRateLimit>15</tt:FrameRateLimit>");
		strcat(sBuffer,"<tt:EncodingInterval>1</tt:EncodingInterval>");
		strcat(sBuffer,"<tt:BitrateLimit>512</tt:BitrateLimit>");
		strcat(sBuffer,"</tt:RateControl>");
		strcat(sBuffer,"<tt:H264>");
		strcat(sBuffer,"<tt:GovLength>25</tt:GovLength>");
		strcat(sBuffer,"<tt:H264Profile>Baseline</tt:H264Profile>");
		strcat(sBuffer,"</tt:H264>");
		strcat(sBuffer,"<tt:Multicast>");
		strcat(sBuffer,"<tt:Address>");
		strcat(sBuffer,"<tt:Type>IPv4</tt:Type>");
		//strcat(sBuffer,"<tt:IPv4Address>192.168.2.131</tt:IPv4Address>");
		strcat(sBuffer,"</tt:Address>");
		strcat(sBuffer,"<tt:Port>0</tt:Port>");
		strcat(sBuffer,"<tt:TTL>1</tt:TTL>");
		strcat(sBuffer,"<tt:AutoStart>false</tt:AutoStart>");
		strcat(sBuffer,"</tt:Multicast>");
		strcat(sBuffer,"<tt:SessionTimeout>PT0S</tt:SessionTimeout>");
		strcat(sBuffer,"</trt:Configuration>");

		strcat(sBuffer,"<trt:Configuration>");
		strcat(sBuffer,"<tt:Name>0_VEC_sub</tt:Name>");
		strcat(sBuffer,"<tt:token>0_VEC_sub</tt:token>");
		strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
		strcat(sBuffer,"<tt:Encoding>H264</tt:Encoding>");
		strcat(sBuffer,"<tt:Resolution>");
		strcat(sBuffer,"<tt:Width>640</tt:Width>");
		strcat(sBuffer,"<tt:Height>480</tt:Height>");
		strcat(sBuffer,"</tt:Resolution>");
		strcat(sBuffer,"<tt:Quality>3.000000</tt:Quality>");
		strcat(sBuffer,"<tt:RateControl>");
		strcat(sBuffer,"<tt:FrameRateLimit>10</tt:FrameRateLimit>");
		strcat(sBuffer,"<tt:EncodingInterval>1</tt:EncodingInterval>");
		strcat(sBuffer,"<tt:BitrateLimit>256</tt:BitrateLimit>");
		strcat(sBuffer,"</tt:RateControl>");
		strcat(sBuffer,"<tt:H264>");
		strcat(sBuffer,"<tt:GovLength>25</tt:GovLength>");
		strcat(sBuffer,"<tt:H264Profile>Baseline</tt:H264Profile>");
		strcat(sBuffer,"</tt:H264>");
		strcat(sBuffer,"<tt:Multicast>");
		strcat(sBuffer,"<tt:Address>");
		strcat(sBuffer,"<tt:Type>IPv4</tt:Type>");
		//strcat(sBuffer,"<tt:IPv4Address>192.168.2.131</tt:IPv4Address>");
		strcat(sBuffer,"</tt:Address>");
		strcat(sBuffer,"<tt:Port>0</tt:Port>");
		strcat(sBuffer,"<tt:TTL>1</tt:TTL>");
		strcat(sBuffer,"<tt:AutoStart>false</tt:AutoStart>");
		strcat(sBuffer,"</tt:Multicast>");
		strcat(sBuffer,"<tt:SessionTimeout>PT0S</tt:SessionTimeout>");
		strcat(sBuffer,"</trt:Configuration>");
		strcat(sBuffer,"</trt:GetVideoEncoderConfigurationResponse>");
		strcat(sBuffer,"</SOAP-ENV:Body>");
		strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
		return strlen(sBuffer);
}
int GetVideoEncoderConfigurationsString( char *sBuffer,struct Namespace *pNameSpace )
{
	int nLen;
		char TmpBuffer[1024];
		char configbuf[1024];
		sprintf(configbuf,"1");
		int profile;
		profile=atoi(configbuf);
		nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
		BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

		//start soap-env body
		strcat(sBuffer,"<SOAP-ENV:Body>");

		strcat(sBuffer,"<trt:GetVideoEncoderConfigurationsResponse>");
		strcat(sBuffer,"<trt:Configurations token=\"0_VEC_main\">");
		strcat(sBuffer,"<tt:Name>0_VEC_main</tt:Name>");
		strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
		strcat(sBuffer,"<tt:Encoding>H264</tt:Encoding>");
		strcat(sBuffer,"<tt:Resolution>");
		strcat(sBuffer,"<tt:Width>1280</tt:Width>");
		strcat(sBuffer,"<tt:Height>720</tt:Height>");
		strcat(sBuffer,"</tt:Resolution>");
		strcat(sBuffer,"<tt:Quality>3.000000</tt:Quality>");
		strcat(sBuffer,"<tt:RateControl>");
		strcat(sBuffer,"<tt:FrameRateLimit>15</tt:FrameRateLimit>");
		strcat(sBuffer,"<tt:EncodingInterval>1</tt:EncodingInterval>");
		strcat(sBuffer,"<tt:BitrateLimit>512</tt:BitrateLimit>");
		strcat(sBuffer,"</tt:RateControl>");
		strcat(sBuffer,"<tt:H264>");
		strcat(sBuffer,"<tt:GovLength>25</tt:GovLength>");
		strcat(sBuffer,"<tt:H264Profile>Baseline</tt:H264Profile>");
		strcat(sBuffer,"</tt:H264>");
		strcat(sBuffer,"<tt:Multicast>");
		strcat(sBuffer,"<tt:Address>");
		strcat(sBuffer,"<tt:Type>IPv4</tt:Type>");
		//strcat(sBuffer,"<tt:IPv4Address>192.168.2.131</tt:IPv4Address>");
		strcat(sBuffer,"</tt:Address>");
		strcat(sBuffer,"<tt:Port>0</tt:Port>");
		strcat(sBuffer,"<tt:TTL>1</tt:TTL>");
		strcat(sBuffer,"<tt:AutoStart>false</tt:AutoStart>");
		strcat(sBuffer,"</tt:Multicast>");
		strcat(sBuffer,"<tt:SessionTimeout>PT0S</tt:SessionTimeout>");
		strcat(sBuffer,"</trt:Configurations>");

		strcat(sBuffer,"<trt:Configurations token=\"0_VEC_sub\">");
		strcat(sBuffer," <tt:Name>0_VEC_sub</tt:Name>");
		strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
		strcat(sBuffer,"<tt:Encoding>H264</tt:Encoding>");
		strcat(sBuffer,"<tt:Resolution>");
		strcat(sBuffer,"<tt:Width>640</tt:Width>");
		strcat(sBuffer,"<tt:Height>480</tt:Height>");
		strcat(sBuffer,"</tt:Resolution>");
		strcat(sBuffer,"<tt:Quality>3.000000</tt:Quality>");
		strcat(sBuffer,"<tt:RateControl>");
		strcat(sBuffer,"<tt:FrameRateLimit>10</tt:FrameRateLimit>");
		strcat(sBuffer,"<tt:EncodingInterval>1</tt:EncodingInterval>");
		strcat(sBuffer,"<tt:BitrateLimit>256</tt:BitrateLimit>");
		strcat(sBuffer,"</tt:RateControl>");
		strcat(sBuffer,"<tt:H264>");
		strcat(sBuffer,"<tt:GovLength>25</tt:GovLength>");
		strcat(sBuffer,"<tt:H264Profile>Baseline</tt:H264Profile>");
		strcat(sBuffer,"</tt:H264>");
		strcat(sBuffer,"<tt:Multicast>");
		strcat(sBuffer,"<tt:Address>");
		strcat(sBuffer,"<tt:Type>IPv4</tt:Type>");
		//strcat(sBuffer,"<tt:IPv4Address>192.168.2.131</tt:IPv4Address >");
		strcat(sBuffer,"</tt:Address>");
		strcat(sBuffer,"<tt:Port>0</tt:Port>");
		strcat(sBuffer,"<tt:TTL>1</tt:TTL>");
		strcat(sBuffer,"<tt:AutoStart>false</tt:AutoStart>");
		strcat(sBuffer,"</tt:Multicast>");
		strcat(sBuffer,"<tt:SessionTimeout>PT0S</tt:SessionTimeout>");
		strcat(sBuffer,"</trt:Configurations>");
		strcat(sBuffer,"</trt:GetVideoEncoderConfigurationsResponse>");
		strcat(sBuffer,"</SOAP-ENV:Body>");
		strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
		return strlen(sBuffer);
}
int GetVideoEncoderConfigurationOptionsString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char configbuf[1024];
	sprintf(configbuf,"1");
	int profile;
	profile=atoi(configbuf);
	int subprofile;
	subprofile=atoi(configbuf);
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	/*
	 * <trt:GetVideoEncoderConfigurationOptionsResponse>
	 * 	<trt:Options>
	 * 		<tt:QualityRange>
	 * 			<tt:Min>10</tt:Min>
	 * 			<tt:Max>100</tt:Max>
	 * 		</tt:QualityRange>
	 * 		<tt:H264>
	 * 			<tt:ResolutionsAvailable>
	 * 				<tt:Width>1280</tt:Width>
	 * 				<tt:Height>720</tt:Height>
	 * 			</tt:ResolutionsAvailable>
	 * 			<tt:GovLengthRange>
	 * 				<tt:Min>1</tt:Min>
	 * 				<tt:Max>200</tt:Max>
	 * 			</tt:GovLengthRange>
	 * 			<tt:FrameRateRange>
	 * 				<tt:Min>1</tt:Min>
	 * 				<tt:Max>30</tt:Max>
	 * 			</tt:FrameRateRange>
	 * 			<tt:EncodingIntervalRange>
	 * 				<tt:Min>1</tt:Min>
	 * 				<tt:Max>1</tt:Max>
	 * 			</tt:EncodingIntervalRange>
	 * 			<tt:H264ProfilesSupported>Baseline</tt:H264ProfilesSupported>
	 * 			<tt:H264ProfilesSupported>Main</tt:H264ProfilesSupported>
	 * 			<tt:H264ProfilesSupported>High</tt:H264ProfilesSupported>
	 * 			</tt:H264>
	 * 			<tt:Extension>
	 * 				<tt:H264>
	 * 					<tt:ResolutionsAvailable>
	 * 						<tt:Width>1280</tt:Width>
	 * 						<tt:Height>720</tt:Height>
	 * 					</tt:ResolutionsAvailable>
	 * 					<tt:GovLengthRange>
	 * 						<tt:Min>1</tt:Min>
	 * 						<tt:Max>200</tt:Max>
	 * 					</tt:GovLengthRange>
	 * 					<tt:FrameRateRange>
	 * 						<tt:Min>1</tt:Min>
	 * 						<tt:Max>30</tt:Max>
	 * 					</tt:FrameRateRange>
	 * 					<tt:EncodingIntervalRange>
	 * 						<tt:Min>1</tt:Min>
	 * 						<tt:Max>1</tt:Max>
	 * 					</tt:EncodingIntervalRange>
	 * 					<tt:BitrateRange>
	 * 						<tt:Min>500</tt:Min>
	 * 						<tt:Max>8000</tt:Max>
	 * 					</tt:BitrateRange>
	 * 				</tt:H264>
	 * 			</tt:Extension>
	 * 		</trt:Options>
	 * 	</trt:GetVideoEncoderConfigurationOptionsResponse>
	 * **/

		 strcat(sBuffer,"<trt:GetVideoEncoderConfigurationOptionsResponse>");
		 strcat(sBuffer,"<trt:Options>");
		 strcat(sBuffer,"<tt:QualityRange>");
		 strcat(sBuffer,"<tt:Min>10</tt:Min>");
		 strcat(sBuffer,"<tt:Max>100</tt:Max>");
		 strcat(sBuffer,"</tt:QualityRange>");
		 strcat(sBuffer,"<tt:H264>");
		 strcat(sBuffer,"<tt:ResolutionsAvailable>");
		 strcat(sBuffer,"<tt:Width>1280</tt:Width>");
		 strcat(sBuffer,"<tt:Height>720</tt:Height>");
		 strcat(sBuffer,"</tt:ResolutionsAvailable>");
		 strcat(sBuffer,"<tt:GovLengthRange>");
		 strcat(sBuffer,"<tt:Min>1</tt:Min>");
		 strcat(sBuffer,"<tt:Max>200</tt:Max>");
		 strcat(sBuffer,"</tt:GovLengthRange>");
		 strcat(sBuffer,"<tt:FrameRateRange>");
		 strcat(sBuffer,"<tt:Min>1</tt:Min>");
		 strcat(sBuffer,"<tt:Max>30</tt:Max>");
		 strcat(sBuffer,"</tt:FrameRateRange>");
		 strcat(sBuffer,"<tt:EncodingIntervalRange>");
		 strcat(sBuffer,"<tt:Min>1</tt:Min>");
		 strcat(sBuffer,"<tt:Max>1</tt:Max>");
		 strcat(sBuffer,"</tt:EncodingIntervalRange>");
		 strcat(sBuffer,"<tt:H264ProfilesSupported>Baseline</tt:H264ProfilesSupported>");
		 strcat(sBuffer,"<tt:H264ProfilesSupported>Main</tt:H264ProfilesSupported>");
		 strcat(sBuffer,"<tt:H264ProfilesSupported>High</tt:H264ProfilesSupported>");
		 strcat(sBuffer,"</tt:H264>");
		 strcat(sBuffer,"<tt:Extension>");
		 strcat(sBuffer,"<tt:H264>");
		 strcat(sBuffer,"<tt:ResolutionsAvailable>");
		 strcat(sBuffer,"<tt:Width>1280</tt:Width>");
		 strcat(sBuffer,"<tt:Height>720</tt:Height>");
		 strcat(sBuffer,"</tt:ResolutionsAvailable>");
		 strcat(sBuffer,"<tt:GovLengthRange>");
		 strcat(sBuffer,"<tt:Min>1</tt:Min>");
		 strcat(sBuffer,"<tt:Max>200</tt:Max>");
		 strcat(sBuffer,"</tt:GovLengthRange>");
		 strcat(sBuffer,"<tt:FrameRateRange>");
		 strcat(sBuffer,"<tt:Min>1</tt:Min>");
		 strcat(sBuffer,"<tt:Max>30</tt:Max>");
		 strcat(sBuffer,"</tt:FrameRateRange>");
		 strcat(sBuffer,"<tt:EncodingIntervalRange>");
		 strcat(sBuffer,"<tt:Min>1</tt:Min>");
		 strcat(sBuffer,"<tt:Max>1</tt:Max>");
		 strcat(sBuffer,"</tt:EncodingIntervalRange>");
		 strcat(sBuffer,"<tt:BitrateRange>");
		 strcat(sBuffer,"<tt:Min>500</tt:Min>");
		 strcat(sBuffer,"<tt:Max>8000</tt:Max>");
		 strcat(sBuffer,"</tt:BitrateRange>");
		 strcat(sBuffer,"</tt:H264>");
		 strcat(sBuffer,"</tt:Extension>");
		 strcat(sBuffer,"</trt:Options>");
		 strcat(sBuffer,"</trt:GetVideoEncoderConfigurationOptionsResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}


int BuildGetProfilesString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char configbuf[1024];
	sprintf(configbuf,"1");
	int profile;
	profile=atoi(configbuf);
	int subprofile;
	subprofile=atoi(configbuf);
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<trt:GetProfilesResponse>");
	strcat(sBuffer,"<trt:Profiles fixed=\"true\" token=\"0_main\">");
	strcat(sBuffer,"<tt:Name>0_main</tt:Name>");
	strcat(sBuffer,"<tt:VideoSourceConfiguration token=\"0_VSC\">");
	strcat(sBuffer,"<tt:Name>0_VSC</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
	strcat(sBuffer,"<tt:SourceToken>0_VSC</tt:SourceToken>");
	//FIXME Hard code Height Width for Main
	if(profile==1)
		sprintf(TmpBuffer,"<tt:Bounds height=\"720\" width=\"1280\" y=\"0\" x=\"0\"></tt:Bounds>");
	else if(profile==2)
		sprintf(TmpBuffer,"<tt:Bounds height=\"480\" width=\"640\" y=\"0\" x=\"0\"></tt:Bounds>");
	else if(profile==3)
		sprintf(TmpBuffer,"<tt:Bounds height=\"240\" width=\"320\" y=\"0\" x=\"0\"></tt:Bounds>");
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tt:VideoSourceConfiguration>");
	strcat(sBuffer,"<tt:AudioSourceConfiguration token=\"0_ASC\">");
	strcat(sBuffer,"<tt:Name>0_ASC</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
	strcat(sBuffer,"<tt:SourceToken>0_ASC</tt:SourceToken>");
	strcat(sBuffer,"</tt:AudioSourceConfiguration>");
	strcat(sBuffer,"<tt:VideoEncoderConfiguration token=\"0_VEC_main\">");
	strcat(sBuffer,"<tt:Name>0_VEC_main</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
	strcat(sBuffer,"<tt:Encoding>H264</tt:Encoding>");
	//FIXME hard code width height
	if(profile==1)
		sprintf(TmpBuffer,"<tt:Resolution><tt:Width>1280</tt:Width><tt:Height>720</tt:Height></tt:Resolution>");
	else if(profile==2)
		sprintf(TmpBuffer,"<tt:Resolution><tt:Width>640</tt:Width><tt:Height>480</tt:Height></tt:Resolution>");
	else if(profile==3)
		sprintf(TmpBuffer,"<tt:Resolution><tt:Width>320</tt:Width><tt:Height>240</tt:Height></tt:Resolution>");
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:Quality>31</tt:Quality>");
	strcat(sBuffer,"<tt:RateControl>");
	//FIXME hard code
	if(profile==1)
	{
		sprintf(configbuf,"25");
		sprintf(TmpBuffer,"<tt:FrameRateLimit>%s</tt:FrameRateLimit>",configbuf);
	}
	else if(profile==2)
	{
		
		sprintf(configbuf,"25");
		sprintf(TmpBuffer,"<tt:FrameRateLimit>%s</tt:FrameRateLimit>",configbuf);
	}
	else if(profile==3)
	{
		sprintf(configbuf,"25");
		sprintf(TmpBuffer,"<tt:FrameRateLimit>%s</tt:FrameRateLimit>",configbuf);
	}
	strcat(sBuffer,TmpBuffer);
	//FIXME hard code
			/*
	sprintf(TmpBuffer,"<tt:EncodingInterval>%d</tt:EncodingInterval>",
	        (int)g_sys_param.videoEnc[0][0].nKeyInterval);
	  */
	sprintf(TmpBuffer,"<tt:EncodingInterval>0</tt:EncodingInterval>");
	strcat(sBuffer,TmpBuffer);
	if(profile==1)
	{
		sprintf(configbuf,"1024");
		sprintf(TmpBuffer,"<tt:BitrateLimit>%s</tt:BitrateLimit>",configbuf);
	}
	else if(profile==2)
	{
		sprintf(configbuf,"1024");
		sprintf(TmpBuffer,"<tt:BitrateLimit>%s</tt:BitrateLimit>",configbuf);
	}
	else if(profile==3)
	{
		sprintf(configbuf,"1024");
		sprintf(TmpBuffer,"<tt:BitrateLimit>%s</tt:BitrateLimit>",configbuf);
	}
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tt:RateControl>");
	strcat(sBuffer,"<tt:H264>");
	strcat(sBuffer,"<tt:GovLength>25</tt:GovLength>");
	strcat(sBuffer,"<tt:H264Profile>Baseline</tt:H264Profile>");
	strcat(sBuffer,"</tt:H264>");
	strcat(sBuffer,"<tt:Multicast>");
	strcat(sBuffer,"<tt:Address><tt:Type>IPv4</tt:Type></tt:Address>");
	strcat(sBuffer,"<tt:Port>0</tt:Port>");
	strcat(sBuffer,"<tt:TTL>0</tt:TTL>");
	strcat(sBuffer,"<tt:AutoStart>false</tt:AutoStart>");
	strcat(sBuffer,"</tt:Multicast>");
	strcat(sBuffer,"<tt:SessionTimeout>PT0S</tt:SessionTimeout>");
	strcat(sBuffer,"</tt:VideoEncoderConfiguration>");
	strcat(sBuffer,"<tt:AudioEncoderConfiguration token=\"0_AEC\">");
	strcat(sBuffer,"<tt:Name>0_AEC</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
	strcat(sBuffer,"<tt:Encoding>G726</tt:Encoding>");
	strcat(sBuffer,"<tt:Bitrate>64</tt:Bitrate>");
	strcat(sBuffer,"<tt:SampleRate>8000</tt:SampleRate>");
	strcat(sBuffer,"<tt:Multicast>");
	strcat(sBuffer,"<tt:Address><tt:Type>IPv4</tt:Type></tt:Address>");
	strcat(sBuffer,"<tt:Port>0</tt:Port>");
	strcat(sBuffer,"<tt:TTL>1</tt:TTL>");
	strcat(sBuffer,"<tt:AutoStart>false</tt:AutoStart>");
	strcat(sBuffer,"</tt:Multicast>");
	strcat(sBuffer,"<tt:SessionTimeout>PT0S</tt:SessionTimeout>");
	strcat(sBuffer,"</tt:AudioEncoderConfiguration>");
	//FIXME remove PTZ
	/*strcat(sBuffer,"<tt:PTZConfiguration token=\"0_PTZ\">");
	strcat(sBuffer,"<tt:Name>0_PTZ</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
	strcat(sBuffer,"<tt:NodeToken>0_PTZ</tt:NodeToken>");
	strcat(sBuffer,"<tt:DefaultAbsolutePantTiltPositionSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:DefaultAbsolutePantTiltPositionSpace>");
	strcat(sBuffer,"<tt:DefaultAbsoluteZoomPositionSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:DefaultAbsoluteZoomPositionSpace>");
	strcat(sBuffer,"<tt:DefaultRelativePanTiltTranslationSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:DefaultRelativePanTiltTranslationSpace>");
	strcat(sBuffer,"<tt:DefaultRelativeZoomTranslationSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:DefaultRelativeZoomTranslationSpace>");
	strcat(sBuffer,"<tt:DefaultContinuousPanTiltVelocitySpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:DefaultContinuousPanTiltVelocitySpace>");
	strcat(sBuffer,"<tt:DefaultContinuousZoomVelocitySpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:DefaultContinuousZoomVelocitySpace>");
	strcat(sBuffer,"<tt:DefaultPTZSpeed>");
	strcat(sBuffer,"<tt:PanTilt space=\"http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace\" y=\"1\" x=\"0\"></tt:PanTilt>");
	strcat(sBuffer,"<tt:Zoom space=\"http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace\" x=\"1\"></tt:Zoom>");
	strcat(sBuffer,"</tt:DefaultPTZSpeed>");
	strcat(sBuffer,"<tt:DefaultPTZTimeout>P16DT3H41M8.564S</tt:DefaultPTZTimeout>");
	strcat(sBuffer,"<tt:PanTiltLimits>");
	strcat(sBuffer,"<tt:Range>");
	strcat(sBuffer,"<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:URI>");
	strcat(sBuffer,"<tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange>");
	strcat(sBuffer,"<tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange>");
	strcat(sBuffer,"</tt:Range>");
	strcat(sBuffer,"</tt:PanTiltLimits>");
	strcat(sBuffer,"<tt:ZoomLimits>");
	strcat(sBuffer,"<tt:Range>");
	strcat(sBuffer,"<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:URI>");
	strcat(sBuffer,"<tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange>");
	strcat(sBuffer,"</tt:Range>");
	strcat(sBuffer,"</tt:ZoomLimits>");
	strcat(sBuffer,"</tt:PTZConfiguration>");*/
	strcat(sBuffer,"</trt:Profiles>");
	strcat(sBuffer,"<trt:Profiles fixed=\"true\" token=\"0_sub\">");
	strcat(sBuffer,"<tt:Name>0_sub</tt:Name>");
	strcat(sBuffer,"<tt:VideoSourceConfiguration token=\"0_VSC\">");
	strcat(sBuffer,"<tt:Name>0_VSC</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
	strcat(sBuffer,"<tt:SourceToken>0_VSC</tt:SourceToken>");
	//FIXME hard code
	if(subprofile==1)
			sprintf(TmpBuffer,"<tt:Bounds height=\"640\" width=\"480\" y=\"0\" x=\"0\"></tt:Bounds>");
		else if(subprofile==2)
			sprintf(TmpBuffer,"<tt:Bounds height=\"640\" width=\"480\" y=\"0\" x=\"0\"></tt:Bounds>");
		else if(subprofile==3)
			sprintf(TmpBuffer,"<tt:Bounds height=\"320\" width=\"240\" y=\"0\" x=\"0\"></tt:Bounds>");
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tt:VideoSourceConfiguration>");
	strcat(sBuffer,"<tt:AudioSourceConfiguration token=\"0_ASC\">");
	strcat(sBuffer,"<tt:Name>0_ASC</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
	strcat(sBuffer,"<tt:SourceToken>0_ASC</tt:SourceToken>");
	strcat(sBuffer,"</tt:AudioSourceConfiguration>");
	strcat(sBuffer,"<tt:VideoEncoderConfiguration token=\"0_VEC_sub\">");
	strcat(sBuffer,"<tt:Name>0_VEC_sub</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
	strcat(sBuffer,"<tt:Encoding>H264</tt:Encoding>");
	//FIXME hard code
	if(subprofile==1)
		sprintf(TmpBuffer,"<tt:Resolution><tt:Width>640</tt:Width><tt:Height>480</tt:Height></tt:Resolution>");
	else if(subprofile==2)
		sprintf(TmpBuffer,"<tt:Resolution><tt:Width>640</tt:Width><tt:Height>480</tt:Height></tt:Resolution>");
	else if(subprofile==3)
		sprintf(TmpBuffer,"<tt:Resolution><tt:Width>320</tt:Width><tt:Height>240</tt:Height></tt:Resolution>");
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:Quality>31</tt:Quality>");
	strcat(sBuffer,"<tt:RateControl>");
	//FIXME hard code
					/*
	sprintf(TmpBuffer,"<tt:FrameRateLimit>%d</tt:FrameRateLimit>",
	        (int)g_sys_param.videoEnc[0][1].nFramerate);
	*/
	if(subprofile==1)
		sprintf(TmpBuffer,"<tt:FrameRateLimit>20</tt:FrameRateLimit>");
	else if(subprofile==2)
		sprintf(TmpBuffer,"<tt:FrameRateLimit>15</tt:FrameRateLimit>");
	else if(subprofile==3)
		sprintf(TmpBuffer,"<tt:FrameRateLimit>10</tt:FrameRateLimit>");
	strcat(sBuffer,TmpBuffer);
	//FIXME hard code
					/*
	sprintf(TmpBuffer,"<tt:EncodingInterval>%d</tt:EncodingInterval>",
	        (int)g_sys_param.videoEnc[0][1].nKeyInterval);
	        */
	sprintf(TmpBuffer,"<tt:EncodingInterval>0</tt:EncodingInterval>");
	strcat(sBuffer,TmpBuffer);
	if(subprofile==1)
		sprintf(TmpBuffer,"<tt:BitrateLimit>1024</tt:BitrateLimit>");
	else if(subprofile==2)
		sprintf(TmpBuffer,"<tt:BitrateLimit>768</tt:BitrateLimit>");
	else if(subprofile==3)
		sprintf(TmpBuffer,"<tt:BitrateLimit>512</tt:BitrateLimit>");
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tt:RateControl>");
	strcat(sBuffer,"<tt:H264><tt:GovLength>25</tt:GovLength><tt:H264Profile>Baseline</tt:H264Profile></tt:H264>");
	strcat(sBuffer,"<tt:Multicast>");
	strcat(sBuffer,"<tt:Address><tt:Type>IPv4</tt:Type></tt:Address>");
	strcat(sBuffer,"<tt:Port>0</tt:Port>");
	strcat(sBuffer,"<tt:TTL>0</tt:TTL>");
	strcat(sBuffer,"<tt:AutoStart>false</tt:AutoStart>");
	strcat(sBuffer,"</tt:Multicast>");
	strcat(sBuffer,"<tt:SessionTimeout>PT0S</tt:SessionTimeout>");
	strcat(sBuffer,"</tt:VideoEncoderConfiguration>");
	strcat(sBuffer,"<tt:AudioEncoderConfiguration token=\"0_AEC\">");
	strcat(sBuffer,"<tt:Name>0_AEC</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
	strcat(sBuffer,"<tt:Encoding>G726</tt:Encoding>");
	strcat(sBuffer,"<tt:Bitrate>64</tt:Bitrate>");
	strcat(sBuffer,"<tt:SampleRate>8000</tt:SampleRate>");
	strcat(sBuffer,"<tt:Multicast>");
	strcat(sBuffer,"<tt:Address><tt:Type>IPv4</tt:Type></tt:Address>");
	strcat(sBuffer,"<tt:Port>0</tt:Port>");
	strcat(sBuffer,"<tt:TTL>1</tt:TTL>");
	strcat(sBuffer,"<tt:AutoStart>false</tt:AutoStart>");
	strcat(sBuffer,"</tt:Multicast>");
	strcat(sBuffer,"<tt:SessionTimeout>PT0S</tt:SessionTimeout>");
	strcat(sBuffer,"</tt:AudioEncoderConfiguration>");
	//FIXME remove PTZ
	/*strcat(sBuffer,"<tt:PTZConfiguration token=\"0_PTZ\">");
	strcat(sBuffer,"<tt:Name>0_PTZ</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
	strcat(sBuffer,"<tt:NodeToken>0_PTZ</tt:NodeToken>");
	strcat(sBuffer,"<tt:DefaultAbsolutePantTiltPositionSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:DefaultAbsolutePantTiltPositionSpace>");
	strcat(sBuffer,"<tt:DefaultAbsoluteZoomPositionSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:DefaultAbsoluteZoomPositionSpace>");
	strcat(sBuffer,"<tt:DefaultRelativePanTiltTranslationSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:DefaultRelativePanTiltTranslationSpace>");
	strcat(sBuffer,"<tt:DefaultRelativeZoomTranslationSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:DefaultRelativeZoomTranslationSpace>");
	strcat(sBuffer,"<tt:DefaultContinuousPanTiltVelocitySpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:DefaultContinuousPanTiltVelocitySpace>");
	strcat(sBuffer,"<tt:DefaultContinuousZoomVelocitySpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:DefaultContinuousZoomVelocitySpace>");
	strcat(sBuffer,"<tt:DefaultPTZSpeed>");
	strcat(sBuffer,"<tt:PanTilt space=\"http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace\" y=\"1\" x=\"0\"></tt:PanTilt>");
	strcat(sBuffer,"<tt:Zoom space=\"http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace\" x=\"1\"></tt:Zoom>");
	strcat(sBuffer,"</tt:DefaultPTZSpeed>");
	strcat(sBuffer,"<tt:DefaultPTZTimeout>P16DT3H41M8.564S</tt:DefaultPTZTimeout>");
	strcat(sBuffer,"<tt:PanTiltLimits>");
	strcat(sBuffer,"<tt:Range>");
	strcat(sBuffer,"<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:URI>");
	strcat(sBuffer,"<tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange>");
	strcat(sBuffer,"<tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange>");
	strcat(sBuffer,"</tt:Range>");
	strcat(sBuffer,"</tt:PanTiltLimits>");
	strcat(sBuffer,"<tt:ZoomLimits>");
	strcat(sBuffer,"<tt:Range>");
	strcat(sBuffer,"<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:URI>");
	strcat(sBuffer,"<tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange>");
	strcat(sBuffer,"</tt:Range>");
	strcat(sBuffer,"</tt:ZoomLimits>");
	strcat(sBuffer,"</tt:PTZConfiguration>");*/
	strcat(sBuffer,"</trt:Profiles>");
	strcat(sBuffer,"</trt:GetProfilesResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);

}

int BuildGetServicesString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
/*
 * <tds:GetServicesResponse>
 * 	 <tds:Service>
 * 		<tds:Namespace>http://www.onvif.org/ver10/events/wsdl</tds:Namespace>
 * 		<tds:XAddr>http://192.168.2.100:80/onvif/services</tds:XAddr>
 * 		<tds:Version>
 * 			<tt:Major>11</tt:Major>
 * 			<tt:Minor>8</tt:Minor>
 * 		</tds:Version>
 * 		<tds:Message>Face Detection, Motion Detection</tds:Message>
 * 		</tds:Service>
 * 		</tds:GetServicesResponse>
 *
 * */
	strcat(sBuffer,"<tds:GetServicesResponse>");
	/*strcat(sBuffer,"<tds:Service>");
	strcat(sBuffer,"<tds:Namespace>http://www.onvif.org/ver10/events/wsdl</tds:Namespace>");
	strcat(sBuffer,"<tds:XAddr>http://192.168.2.123:80/onvif/services</tds:XAddr>");
	strcat(sBuffer,"<tds:Message>PIR Detection, Motion Detection</tds:Message>");
	strcat(sBuffer,"</tds:Service>");*/
	strcat(sBuffer,"</tds:GetServicesResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}

int BuildGetSnapshotUriString(char *sBuffer,struct Namespace *pNameSpace)
{
	/**
	 * HTTP/1.1 200 OK
Server: gSOAP/2.8
Content-Type: application/soap+xml; charset=utf-8; action="http://www.onvif.org/ver10/media/wsdl/GetSnapshotUri"
Content-Length: 2466
Connection: close

<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://www.w3.org/2003/05/soap-envelope" xmlns:SOAP-ENC="http://www.w3.org/2003/05/soap-encoding" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:wsa="http://schemas.xmlsoap.org/ws/2004/08/addressing" xmlns:wsdd="http://schemas.xmlsoap.org/ws/2005/04/discovery" xmlns:chan="http://schemas.microsoft.com/ws/2005/02/duplex" xmlns:wsa5="http://www.w3.org/2005/08/addressing" xmlns:xmime="http://www.w3.org/2005/05/xmlmime" xmlns:xop="http://www.w3.org/2004/08/xop/include" xmlns:wsrfbf="http://docs.oasis-open.org/wsrf/bf-2" xmlns:tt="http://www.onvif.org/ver10/schema" xmlns:wstop="http://docs.oasis-open.org/wsn/t-1" xmlns:wsrfr="http://docs.oasis-open.org/wsrf/r-2" xmlns:ns1="http://www.onvif.org/ver10/actionengine/wsdl" xmlns:tad="http://www.onvif.org/ver10/analyticsdevice/wsdl" xmlns:tan="http://www.onvif.org/ver20/analytics/wsdl" xmlns:tdn="http://www.onvif.org/ver10/network/wsdl" xmlns:tds="http://www.onvif.org/ver10/device/wsdl" xmlns:tev="http://www.onvif.org/ver10/events/wsdl" xmlns:wsnt="http://docs.oasis-open.org/wsn/b-2" xmlns:c14n="http://www.w3.org/2001/10/xml-exc-c14n#" xmlns:wsu="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd" xmlns:xenc="http://www.w3.org/2001/04/xmlenc#" xmlns:wsc="http://schemas.xmlsoap.org/ws/2005/02/sc" xmlns:ds="http://www.w3.org/2000/09/xmldsig#" xmlns:wsse="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd" xmlns:timg="http://www.onvif.org/ver20/imaging/wsdl" xmlns:tls="http://www.onvif.org/ver10/display/wsdl" xmlns:tmd="http://www.onvif.org/ver10/deviceIO/wsdl" xmlns:tptz="http://www.onvif.org/ver20/ptz/wsdl" xmlns:trc="http://www.onvif.org/ver10/recording/wsdl" xmlns:trp="http://www.onvif.org/ver10/replay/wsdl" xmlns:trt="http://www.onvif.org/ver10/media/wsdl" xmlns:trv="http://www.onvif.org/ver10/receiver/wsdl" xmlns:ter="http://www.onvif.org/ver10/error" xmlns:tse="http://www.onvif.org/ver10/search/wsdl" xmlns:tns1="http://www.onvif.org/ver10/topics"><SOAP-ENV:Body><trt:GetSnapshotUriResponse><trt:MediaUri><tt:Uri>http://192.168.2.100:80/cgi-bin/snapshot.cgi?stream=0</tt:Uri><tt:InvalidAfterConnect>false</tt:InvalidAfterConnect><tt:InvalidAfterReboot>false</tt:InvalidAfterReboot><tt:Timeout>PT0H12M0S</tt:Timeout></trt:MediaUri></trt:GetSnapshotUriResponse></SOAP-ENV:Body></SOAP-ENV:Envelope>
	 *
	 *
	 *
	 *
	 *
	 *
	 * */
	/*
	 * trt:GetSnapshotUriResponse>
      <trt:MediaUri>
        <tt:Uri>http://192.168.68.111/onvif-cgi/jpg/image.cgi?resolution=800x600&amp;compression=20&amp;camera=1</tt:Uri>
        <tt:InvalidAfterConnect>false</tt:InvalidAfterConnect>
        <tt:InvalidAfterReboot>false</tt:InvalidAfterReboot>
        <tt:Timeout>PT0S</tt:Timeout>
      </trt:MediaUri>
    </trt:GetSnapshotUriResponse>
	 * */
	int nLen;
	char TmpBuffer[1024];
	char localIP[16];
	GetLocalAddress(localIP,"wlan0",(char *)"127.0.0.1");
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<trt:GetSnapshotUriResponse>");
	strcat(sBuffer,"<trt:MediaUri>");
	sprintf(TmpBuffer,"<tt:Uri>http://%s/cgi-bin/get_snapshot.cgi</tt:Uri>",localIP);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:InvalidAfterConnect>false</tt:InvalidAfterConnect>");
	strcat(sBuffer,"<tt:InvalidAfterReboot>false</tt:InvalidAfterReboot>");
	strcat(sBuffer,"<tt:Timeout>PT0S</tt:Timeout>");
	strcat(sBuffer,"</trt:MediaUri>");
	strcat(sBuffer,"</trt:GetSnapshotUriResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}

int BuildwsdlGetProfileString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char configbuf[1024];
	sprintf(configbuf,"1");
	int profile;
	profile=atoi(configbuf);
	int subprofile;
	subprofile=atoi(configbuf);
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<trt:GetProfileResponse>");
	strcat(sBuffer,"<trt:Profile fixed=\"true\" token=\"0_main\">");
	strcat(sBuffer,"<tt:Name>0_main</tt:Name>");
	strcat(sBuffer,"<tt:VideoSourceConfiguration token=\"0_VSC\">");
	strcat(sBuffer,"<tt:Name>0_VSC</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
	strcat(sBuffer,"<tt:SourceToken>0_VSC</tt:SourceToken>");
	//FIXME hard code
	if(profile==1)
		sprintf(TmpBuffer,"<tt:Bounds height=\"720\" width=\"1280\" y=\"0\" x=\"0\"></tt:Bounds>");
	else if(profile==2)
		sprintf(TmpBuffer,"<tt:Bounds height=\"480\" width=\"640\" y=\"0\" x=\"0\"></tt:Bounds>");
	else if(profile==3)
		sprintf(TmpBuffer,"<tt:Bounds height=\"240\" width=\"320\" y=\"0\" x=\"0\"></tt:Bounds>");
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tt:VideoSourceConfiguration>");
	strcat(sBuffer,"<tt:AudioSourceConfiguration token=\"0_ASC\">");
	strcat(sBuffer,"<tt:Name>0_ASC</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
	strcat(sBuffer,"<tt:SourceToken>0_ASC</tt:SourceToken>");
	strcat(sBuffer,"</tt:AudioSourceConfiguration>");
	strcat(sBuffer,"<tt:VideoEncoderConfiguration token=\"0_VEC_main\">");
	strcat(sBuffer,"<tt:Name>0_VEC_main</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
	strcat(sBuffer,"<tt:Encoding>H264</tt:Encoding>");
	//FIXME Hard code
	if(profile==1)
		sprintf(TmpBuffer,"<tt:Resolution><tt:Width>1280</tt:Width><tt:Height>720</tt:Height></tt:Resolution>");
	else if(profile==2)
		sprintf(TmpBuffer,"<tt:Resolution><tt:Width>640</tt:Width><tt:Height>480</tt:Height></tt:Resolution>");
	else if(profile==3)
		sprintf(TmpBuffer,"<tt:Resolution><tt:Width>320</tt:Width><tt:Height>240</tt:Height></tt:Resolution>");
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:Quality>31</tt:Quality>");
	strcat(sBuffer,"<tt:RateControl>");
	//FIXME hard code
	if(profile==1)
	{
		sprintf(configbuf,"25");
		sprintf(TmpBuffer,"<tt:FrameRateLimit>%s</tt:FrameRateLimit>",configbuf);
	}
	else if(profile==2)
	{
		sprintf(configbuf,"25");
		sprintf(TmpBuffer,"<tt:FrameRateLimit>%s</tt:FrameRateLimit>",configbuf);
	}
	else if(profile==3)
	{
		sprintf(configbuf,"25");
		sprintf(TmpBuffer,"<tt:FrameRateLimit>%s</tt:FrameRateLimit>",configbuf);
	}
	strcat(sBuffer,TmpBuffer);
	//FIXME hard code
	/*sprintf(TmpBuffer,"<tt:EncodingInterval>%d</tt:EncodingInterval>",
	        (int)g_sys_param.videoEnc[0][0].nKeyInterval);*/
	sprintf(TmpBuffer,"<tt:EncodingInterval>0</tt:EncodingInterval>");
	strcat(sBuffer,TmpBuffer);

	if(profile==1)
	{
		sprintf(configbuf,"1024");
		sprintf(TmpBuffer,"<tt:BitrateLimit>%s</tt:BitrateLimit>",configbuf);
	}
	else if(profile==2)
	{
		sprintf(configbuf,"512");
		sprintf(TmpBuffer,"<tt:BitrateLimit>%s</tt:BitrateLimit>",configbuf);
	}
	else if(profile==3)
	{
		sprintf(configbuf,"512");
		sprintf(TmpBuffer,"<tt:BitrateLimit>%s</tt:BitrateLimit>",configbuf);
	}
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tt:RateControl>");
	strcat(sBuffer,"<tt:H264>");
	strcat(sBuffer,"<tt:GovLength>25</tt:GovLength>");
	strcat(sBuffer,"<tt:H264Profile>Baseline</tt:H264Profile>");
	strcat(sBuffer,"</tt:H264>");
	strcat(sBuffer,"<tt:Multicast>");
	strcat(sBuffer,"<tt:Address><tt:Type>IPv4</tt:Type></tt:Address>");
	strcat(sBuffer,"<tt:Port>0</tt:Port>");
	strcat(sBuffer,"<tt:TTL>0</tt:TTL>");
	strcat(sBuffer,"<tt:AutoStart>false</tt:AutoStart>");
	strcat(sBuffer,"</tt:Multicast>");
	strcat(sBuffer,"<tt:SessionTimeout>PT0S</tt:SessionTimeout>");
	strcat(sBuffer,"</tt:VideoEncoderConfiguration>");
	strcat(sBuffer,"<tt:AudioEncoderConfiguration token=\"0_AEC\">");
	strcat(sBuffer,"<tt:Name>0_AEC</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
	strcat(sBuffer,"<tt:Encoding>G726</tt:Encoding>");
	strcat(sBuffer,"<tt:Bitrate>64</tt:Bitrate>");
	strcat(sBuffer,"<tt:SampleRate>8000</tt:SampleRate>");
	strcat(sBuffer,"<tt:Multicast>");
	strcat(sBuffer,"<tt:Address><tt:Type>IPv4</tt:Type></tt:Address>");
	strcat(sBuffer,"<tt:Port>0</tt:Port>");
	strcat(sBuffer,"<tt:TTL>0</tt:TTL>");
	strcat(sBuffer,"<tt:AutoStart>false</tt:AutoStart>");
	strcat(sBuffer,"</tt:Multicast>");
	strcat(sBuffer,"<tt:SessionTimeout>PT0S</tt:SessionTimeout>");
	strcat(sBuffer,"</tt:AudioEncoderConfiguration>");
	//FIXME remove PTZ
	/*strcat(sBuffer,"<tt:PTZConfiguration token=\"0_PTZ\">");
	strcat(sBuffer,"<tt:Name>0_PTZ</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
	strcat(sBuffer,"<tt:NodeToken>0_PTZ</tt:NodeToken>");
	strcat(sBuffer,"<tt:DefaultAbsolutePantTiltPositionSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:DefaultAbsolutePantTiltPositionSpace>");
	strcat(sBuffer,"<tt:DefaultAbsoluteZoomPositionSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:DefaultAbsoluteZoomPositionSpace>");
	strcat(sBuffer,"<tt:DefaultRelativePanTiltTranslationSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:DefaultRelativePanTiltTranslationSpace>");
	strcat(sBuffer,"<tt:DefaultRelativeZoomTranslationSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:DefaultRelativeZoomTranslationSpace>");
	strcat(sBuffer,"<tt:DefaultContinuousPanTiltVelocitySpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:DefaultContinuousPanTiltVelocitySpace>");
	strcat(sBuffer,"<tt:DefaultContinuousZoomVelocitySpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:DefaultContinuousZoomVelocitySpace>");
	strcat(sBuffer,"<tt:DefaultPTZSpeed>");
	strcat(sBuffer,"<tt:PanTilt space=\"http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace\" y=\"1\" x=\"0\"></tt:PanTilt>");
	strcat(sBuffer,"<tt:Zoom space=\"http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace\" x=\"1\"></tt:Zoom>");
	strcat(sBuffer,"</tt:DefaultPTZSpeed>");
	strcat(sBuffer,"<tt:DefaultPTZTimeout>P16DT3H41M8.564S</tt:DefaultPTZTimeout>");
	strcat(sBuffer,"<tt:PanTiltLimits>");
	strcat(sBuffer,"<tt:Range>");
	strcat(sBuffer,"<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:URI>");
	strcat(sBuffer,"<tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange>");
	strcat(sBuffer,"<tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange>");
	strcat(sBuffer,"</tt:Range>");
	strcat(sBuffer,"</tt:PanTiltLimits>");
	strcat(sBuffer,"<tt:ZoomLimits>");
	strcat(sBuffer,"<tt:Range>");
	strcat(sBuffer,"<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:URI>");
	strcat(sBuffer,"<tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange>");
	strcat(sBuffer,"</tt:Range>");
	strcat(sBuffer,"</tt:ZoomLimits>");
	strcat(sBuffer,"</tt:PTZConfiguration>");*/
	strcat(sBuffer,"</trt:Profile>");
	strcat(sBuffer,"</trt:GetProfileResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}


int BuildGetStreamUriString(char *sBuffer,char *sStreamString,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char localIP[16];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	GetLocalAddress(localIP,"wlan0",(char *)"127.0.0.1");
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<trt:GetStreamUriResponse>");
	strcat(sBuffer,"<trt:MediaUri>");
	//FIXME hard code
	if(0 == strcmp(sStreamString,"0_main"))
		sprintf(TmpBuffer,"<tt:Uri>rtsp://%s:%d/v1</tt:Uri>",localIP,554);
	else
		sprintf(TmpBuffer,"<tt:Uri>rtsp://%s:%d/v2</tt:Uri>",localIP,554);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:InvalidAfterConnect>false</tt:InvalidAfterConnect>");
	strcat(sBuffer,"<tt:InvalidAfterReboot>false</tt:InvalidAfterReboot>");
	strcat(sBuffer,"<tt:Timeout>PT0S</tt:Timeout>");
	strcat(sBuffer,"</trt:MediaUri>");
	strcat(sBuffer,"</trt:GetStreamUriResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}
/*
 *
 * HTTP/1.1 200 OK
Server: gSOAP/2.8
Content-Type: application/soap+xml; charset=utf-8; action="http://www.onvif.org/ver10/media/wsdl/GetVideoSourceConfiguration"
Content-Length: 2487
Connection: close

<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://www.w3.org/2003/05/soap-envelope" xmlns:SOAP-ENC="http://www.w3.org/2003/05/soap-encoding" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:wsa="http://schemas.xmlsoap.org/ws/2004/08/addressing" xmlns:wsdd="http://schemas.xmlsoap.org/ws/2005/04/discovery" xmlns:chan="http://schemas.microsoft.com/ws/2005/02/duplex" xmlns:wsa5="http://www.w3.org/2005/08/addressing" xmlns:xmime="http://www.w3.org/2005/05/xmlmime" xmlns:xop="http://www.w3.org/2004/08/xop/include" xmlns:wsrfbf="http://docs.oasis-open.org/wsrf/bf-2" xmlns:tt="http://www.onvif.org/ver10/schema" xmlns:wstop="http://docs.oasis-open.org/wsn/t-1" xmlns:wsrfr="http://docs.oasis-open.org/wsrf/r-2" xmlns:ns1="http://www.onvif.org/ver10/actionengine/wsdl" xmlns:tad="http://www.onvif.org/ver10/analyticsdevice/wsdl" xmlns:tan="http://www.onvif.org/ver20/analytics/wsdl" xmlns:tdn="http://www.onvif.org/ver10/network/wsdl" xmlns:tds="http://www.onvif.org/ver10/device/wsdl" xmlns:tev="http://www.onvif.org/ver10/events/wsdl" xmlns:wsnt="http://docs.oasis-open.org/wsn/b-2" xmlns:c14n="http://www.w3.org/2001/10/xml-exc-c14n#" xmlns:wsu="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd" xmlns:xenc="http://www.w3.org/2001/04/xmlenc#" xmlns:wsc="http://schemas.xmlsoap.org/ws/2005/02/sc" xmlns:ds="http://www.w3.org/2000/09/xmldsig#" xmlns:wsse="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd" xmlns:timg="http://www.onvif.org/ver20/imaging/wsdl" xmlns:tls="http://www.onvif.org/ver10/display/wsdl" xmlns:tmd="http://www.onvif.org/ver10/deviceIO/wsdl" xmlns:tptz="http://www.onvif.org/ver20/ptz/wsdl" xmlns:trc="http://www.onvif.org/ver10/recording/wsdl" xmlns:trp="http://www.onvif.org/ver10/replay/wsdl" xmlns:trt="http://www.onvif.org/ver10/media/wsdl" xmlns:trv="http://www.onvif.org/ver10/receiver/wsdl" xmlns:ter="http://www.onvif.org/ver10/error" xmlns:tse="http://www.onvif.org/ver10/search/wsdl" xmlns:tns1="http://www.onvif.org/ver10/topics"><SOAP-ENV:Body><trt:GetVideoSourceConfigurationResponse><trt:Configuration token="VideoSourceMain"><tt:Name>VideoSourceMain</tt:Name><tt:UseCount>2</tt:UseCount><tt:SourceToken>VideoSourceMain</tt:SourceToken><tt:Bounds height="720" width="1280" y="0" x="0"></tt:Bounds></trt:Configuration></trt:GetVideoSourceConfigurationResponse></SOAP-ENV:Body></SOAP-ENV:Envelope>
 *
 *
 *
 * */
int BuildGetVideoSourceConfigureString(char *sBuffer,char *sStreamString,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char localIP[16];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	GetLocalAddress(localIP,"wlan0",(char *)"127.0.0.1");
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<trt:GetVideoSourceConfigurationResponse>");
	sprintf(TmpBuffer,"<trt:Configuration token=\"%s\">",sStreamString);
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<tt:Name>%s</tt:Name>",sStreamString);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
	sprintf(TmpBuffer,"<tt:SourceToken>%s</tt:SourceToken>",sStreamString);
	strcat(sBuffer,TmpBuffer);
	//FIXME hard code
	char configbuf[1024];
	sprintf(configbuf,"1");
	int profile;
	profile=atoi(configbuf);
	sprintf(configbuf,"1");
	int subprofile;
	subprofile=atoi(configbuf);
	if(0 == strcmp(sStreamString,"0_VSC"))
	{
		if(profile==1)
			sprintf(TmpBuffer,"<tt:Bounds height=\"720\" width=\"1280\" y=\"0\" x=\"0\"></tt:Bounds>");
		else if(profile==2)
			sprintf(TmpBuffer,"<tt:Bounds height=\"480\" width=\"640\" y=\"0\" x=\"0\"></tt:Bounds>");
		else if(profile==3)
			sprintf(TmpBuffer,"<tt:Bounds height=\"240\" width=\"320\" y=\"0\" x=\"0\"></tt:Bounds>");
	}
	else
	{
		if(subprofile==1)
				sprintf(TmpBuffer,"<tt:Resolution><tt:Width>640</tt:Width><tt:Height>480</tt:Height></tt:Resolution>");
			else if(subprofile==2)
				sprintf(TmpBuffer,"<tt:Resolution><tt:Width>640</tt:Width><tt:Height>480</tt:Height></tt:Resolution>");
			else if(subprofile==3)
				sprintf(TmpBuffer,"<tt:Resolution><tt:Width>320</tt:Width><tt:Height>240</tt:Height></tt:Resolution>");
	}
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</trt:Configuration>");
	strcat(sBuffer,"</trt:GetVideoSourceConfigurationResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}
int GetVideoSourceConfigurationsString(char *sBuffer,char *sStreamString,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char localIP[16];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	GetLocalAddress(localIP,"wlan0",(char *)"127.0.0.1");
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<trt:GetVideoSourceConfigurationsResponse>");
	sprintf(TmpBuffer,"<trt:Configuration token=\"%s\">",sStreamString);
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<tt:Name>%s</tt:Name>",sStreamString);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
	sprintf(TmpBuffer,"<tt:SourceToken>%s</tt:SourceToken>",sStreamString);
	strcat(sBuffer,TmpBuffer);
	//FIXME hard code
	char configbuf[1024];
	sprintf(configbuf,"1");
	int profile;
	profile=atoi(configbuf);
	int subprofile;
	subprofile=atoi(configbuf);
	if(0 == strcmp(sStreamString,"0_VSC"))
	{
		if(profile==1)
				sprintf(TmpBuffer,"<tt:Bounds height=\"720\" width=\"1280\" y=\"0\" x=\"0\"></tt:Bounds>");
			else if(profile==2)
				sprintf(TmpBuffer,"<tt:Bounds height=\"480\" width=\"640\" y=\"0\" x=\"0\"></tt:Bounds>");
			else if(profile==3)
				sprintf(TmpBuffer,"<tt:Bounds height=\"240\" width=\"320\" y=\"0\" x=\"0\"></tt:Bounds>");
	}
	else
	{
		if(subprofile==1)
				sprintf(TmpBuffer,"<tt:Resolution><tt:Width>640</tt:Width><tt:Height>480</tt:Height></tt:Resolution>");
			else if(subprofile==2)
				sprintf(TmpBuffer,"<tt:Resolution><tt:Width>640</tt:Width><tt:Height>480</tt:Height></tt:Resolution>");
			else if(subprofile==3)
				sprintf(TmpBuffer,"<tt:Resolution><tt:Width>320</tt:Width><tt:Height>240</tt:Height></tt:Resolution>");
	}
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</trt:Configuration>");
	strcat(sBuffer,"</trt:GetVideoSourceConfigurationsResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}


int BuildGetUsersConfigureString(char *sBuffer,char *sStreamString,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char configbuff[1024];
	char localIP[16];
	//USER_INFO_PARAM userInfo;
	int ret = 0;
	int i = 0;
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	//FIXME hard code
	char configbuf[1024];
	sprintf(configbuf,"88888888");
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<tds:GetUsersResponse>");
	sprintf(TmpBuffer,"<tds:User>");
	strcat(sBuffer,TmpBuffer);
	//FIXME hard code
	sprintf(TmpBuffer,"<tt:Username>admin</tt:Username>");
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<tt:Password>%s</tt:Password>",configbuf);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:UserLevel>Administrator");
	strcat(sBuffer,"</tt:UserLevel>");
	strcat(sBuffer,"</tds:User>");
//FIXME hard code
	/*while(i++ <= 9)
	{
		sprintf(TmpBuffer,"<tds:User>");
		strcat(sBuffer,TmpBuffer);
		sprintf(TmpBuffer,"<tt:Username>%s</tt:Username>", userInfo.Users[i].strName);
		strcat(sBuffer,TmpBuffer);
		sprintf(TmpBuffer,"<tt:Password>%s</tt:Password>", userInfo.Users[i].strPsw);
		strcat(sBuffer,TmpBuffer);
		strcat(sBuffer,"<tt:UserLevel>Operator");
		
		strcat(sBuffer,"</tt:UserLevel>");
		strcat(sBuffer,"</tds:User>");
	}*/
	sprintf(TmpBuffer,"<tds:User>");
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<tt:Username>user</tt:Username>");
	strcat(sBuffer,TmpBuffer);
	sprintf(configbuf,"88888888");
	sprintf(TmpBuffer,"<tt:Password>%s</tt:Password>",configbuf);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:UserLevel>Operator");

	strcat(sBuffer,"</tt:UserLevel>");
	strcat(sBuffer,"</tds:User>");

	strcat(sBuffer,"</tds:GetUsersResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}


int BuildGetImagingSettingsConfigureString(char *sBuffer,char *sStreamString,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char configbuff[1024];
	char localIP[16];
	//FIXME hard code
	//VIDEO_IN_ATTR vinAttr;
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

	//FIXME hard code
	//getVideoInAttrParam(0, &vinAttr);
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<timg:GetImagingSettingsResponse>");
	strcat(sBuffer,"<timg:ImagingSettings>");
	//FIXME hard code
	/*sprintf(TmpBuffer,"<tt:Brightness>%d</tt:Brightness>", vinAttr.nBrightness);
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<tt:ColorSaturation>%d</tt:ColorSaturation>", vinAttr.nSaturation);
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<tt:Contrast>%d</tt:Contrast>", vinAttr.nContrast);
	strcat(sBuffer,TmpBuffer);*/
	sprintf(configbuff,"128");
	sprintf(TmpBuffer,"<tt:Brightness>%s</tt:Brightness>",configbuff);
	strcat(sBuffer,TmpBuffer);
	sprintf(configbuff,"128");
	sprintf(TmpBuffer,"<tt:ColorSaturation>%s</tt:ColorSaturation>",configbuff);
	strcat(sBuffer,TmpBuffer);
	sprintf(configbuff,"128");
	sprintf(TmpBuffer,"<tt:Contrast>%s</tt:Contrast>",configbuff);
	strcat(sBuffer,TmpBuffer);

	strcat(sBuffer,"</timg:ImagingSettings>");
	strcat(sBuffer,"</timg:GetImagingSettingsResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}

int BuildSetImagingSettingsConfigureString(char *sBuffer,char *sStreamBrightness, char *sStreamSaturation, char *sStreamContrast,struct Namespace *pNameSpace)
{
	//FIXME Hard code
	/*int nLen;
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	VIDEO_IN_ATTR vinAttr;
	getVideoInAttrParam(0, &vinAttr);
	vinAttr.nBrightness = atoi(sStreamBrightness);
	vinAttr.nSaturation= atoi(sStreamSaturation);
	vinAttr.nContrast= atoi(sStreamContrast);
	vadcDrv_SetBrightness(0, vinAttr.nBrightness);
	vadcDrv_SetContrast(0, vinAttr.nContrast);
	vadcDrv_SetSaturation(0, vinAttr.nSaturation);
	setVideoInAttrParam(0, &vinAttr);
	printf("vinAttr.nBrightness = %d,vinAttr.nContrast = %d,vinAttr.nSaturation= %d \n", vinAttr.nBrightness, vinAttr.nContrast, vinAttr.nSaturation);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<timg:SetImagingSettingsResponse>");
	strcat(sBuffer,"</timg:SetImagingSettingsResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");*/
	return strlen(sBuffer);
}

int SetVideoEncoderConfiguration(char *sBuffer,struct Namespace *pNameSpace)
{
	//FIXME Hard code
	int nLen;
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<trt:SetVideoEncoderConfigurationResponse>");
	strcat(sBuffer,"</trt:SetVideoEncoderConfigurationResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}

int BuildGetOptionsConfigureString(char *sBuffer,char *sStreamString,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char localIP[16];
	//FIXME hard code
	//VIDEO_IN_ATTR vinAttr;
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<timg:GetOptionsResponse>");
	strcat(sBuffer,"<timg:ImagingOptions>");
    
	strcat(sBuffer,"<tt:Brightness><tt:Min>0.0</tt:Min><tt:Max>255</tt:Max></tt:Brightness>");
    strcat(sBuffer,"<tt:ColorSaturation><tt:Min>0.0</tt:Min><tt:Max>255</tt:Max></tt:ColorSaturation>");
    strcat(sBuffer,"<tt:Contrast><tt:Min>0.0</tt:Min><tt:Max>255</tt:Max></tt:Contrast>");
    strcat(sBuffer,"<tt:Exposure><tt:Min>0.0</tt:Min><tt:Max>255</tt:Max></tt:Exposure>");
	strcat(sBuffer,"</timg:ImagingOptions>");
	strcat(sBuffer,"</timg:GetOptionsResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}



int BuildGetInitialTerminationTimeString(char *sBuffer,char *suuid,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char localIP[16];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	GetLocalAddress(localIP,"wlan0",(char *)"127.0.0.1");
	strcat(sBuffer,"<SOAP-ENV:Header>");
	sprintf(TmpBuffer,"<wsa5:MessageID>%s</wsa5:MessageID>",suuid);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<wsa5:ReplyTo SOAP-ENV:mustUnderstand=\"true\">");
	strcat(sBuffer,"<wsa5:Address>http://www.w3.org/2005/08/addressing/anonymous</wsa5:Address>");
	strcat(sBuffer,"</wsa5:ReplyTo>");
	if(g_wOnvifPort == 80)
		sprintf(TmpBuffer,"<wsa5:To SOAP-ENV:mustUnderstand=\"true\">http://%s/onvif/Events_service</wsa5:To>",localIP);
	else
		sprintf(TmpBuffer,"<wsa5:To SOAP-ENV:mustUnderstand=\"true\">http://%s:%d/onvif/Events_service</wsa5:To>",localIP,g_wOnvifPort);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<wsa5:Action SOAP-ENV:mustUnderstand=\"true\">http://www.onvif.org/ver10/events/wsdl/EventPortType/CreatePullPointSubscriptionRequest</wsa5:Action>");
	strcat(sBuffer,"</SOAP-ENV:Header>");

	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<tev:CreatePullPointSubscriptionResponse>");
	strcat(sBuffer,"<tev:SubscriptionReference><wsa5:Address xsi:nil=\"true\"/></tev:SubscriptionReference>");
	sprintf(TmpBuffer,"<wsnt:CurrentTime>1970-01-01T00:00:00Z</wsnt:CurrentTime>");
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<wsnt:TerminationTime>1970-01-01T00:00:00Z</wsnt:TerminationTime>");
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tev:CreatePullPointSubscriptionResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
}

int BuildGetFailString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"SOAP-ENV:Fault>");
	strcat(sBuffer,"<faultcode>SOAP-ENV:Client</faultcode>");
	strcat(sBuffer,"<faultstring>HTTP GET method not implemented</faultstring>");
	strcat(sBuffer,"</SOAP-ENV:Fault>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}
int GetNTPString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char localIP[16];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	GetLocalAddress(localIP,"wlan0",(char *)"127.0.0.1");
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<tds:GetNTPResponse><tds:NTPInformation><tt:FromDHCP>false</tt:FromDHCP><tt:NTPManual><tt:Type>IPv4</tt:Type><tt:IPv4Address>10.1.1.1</tt:IPv4Address></tt:NTPManual></tds:NTPInformation></tds:GetNTPResponse></SOAP-ENV:Body></SOAP-ENV:Envelope>");
	return strlen(sBuffer);
}
int GetHostnameString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char hostname[32];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	sprintf(hostname,"pi");
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<tds:GetHostnameResponse><tds:HostnameInformation>");
	strcat(sBuffer,"<tt:FromDHCP>false</tt:FromDHCP>");
	sprintf(TmpBuffer,"<tt:Name>%s</tt:Name>",hostname);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tds:HostnameInformation></tds:GetHostnameResponse></SOAP-ENV:Body></SOAP-ENV:Envelope>");
	return strlen(sBuffer);
}
int GetDiscoveryMode(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<tds:GetDiscoveryModeResponse><tds:DiscoveryMode>Discoverable</tds:DiscoveryMode></tds:GetDiscoveryModeResponse></SOAP-ENV:Body></SOAP-ENV:Envelope>");
	return strlen(sBuffer);
}
int GetNetworkDefaultGateway(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char defgateway[32];
	sprintf(defgateway,"192.168.1.1");
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<tds:GetNetworkDefaultGatewayResponse><tds:NetworkDefaultGateway>");
	sprintf(TmpBuffer,"<tt:IPv4Address>%s</tt:IPv4Address>",defgateway);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tds:NetworkDefaultGateway></tds:GetNetworkDefaultGatewayResponse></SOAP-ENV:Body></SOAP-ENV:Envelope>");
	return strlen(sBuffer);
}

int GetNetworkProtocols(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char httpport[32];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<tds:GetNetworkProtocolsResponse>");
	strcat(sBuffer,"<tds:NetworkProtocols><tt:Name>HTTP</tt:Name>");
	strcat(sBuffer,"<tt:Enabled>true</tt:Enabled>");
	sprintf(httpport,"80");
	sprintf(TmpBuffer,"<tt:Port>%s</tt:Port></tds:NetworkProtocols>",httpport);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tds:NetworkProtocols><tt:Name>RTSP</tt:Name>");
	strcat(sBuffer,"<tt:Enabled>true</tt:Enabled>");
	sprintf(httpport,"554");
	sprintf(TmpBuffer,"<tt:Port>%s</tt:Port></tds:NetworkProtocols>",httpport);
	strcat(sBuffer,TmpBuffer);
strcat(sBuffer,"</tds:GetNetworkProtocolsResponse></SOAP-ENV:Body></SOAP-ENV:Envelope>");
	return strlen(sBuffer);
}
int GetVideoSources(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<tds:GetVideoSourcesResponse>");
	strcat(sBuffer,"<tt:Token>0_VSC</tt:Token>");
strcat(sBuffer,"</tds:GetVideoSourcesResponse></SOAP-ENV:Body></SOAP-ENV:Envelope>");
	return strlen(sBuffer);
}
//<tds:GetDNSResponse><tds:DNSInformation><tt:FromDHCP>true</tt:FromDHCP><tt:DNSFromDHCP><tt:Type>IPv4</tt:Type><tt:IPv4Address>8.8.8.8</tt:IPv4Address></tt:DNSFromDHCP><tt:DNSFromDHCP><tt:Type>IPv4</tt:Type><tt:IPv4Address>168.95.1.1</tt:IPv4Address></tt:DNSFromDHCP></tds:DNSInformation></tds:GetDNSResponse></SOAP-ENV:Body></SOAP-ENV:Envelope>



void ProcessAction(const struct Http_Buffer *pBuf,int sock)
{
	int nLen,nLen2;
	char TmpBuf[65535],TmpBuf2[65535];
	//FIXME hard code
	//PTZ_CMD   ptzCmd;
	char strPTZ[100];
	char strUser[100];
	char strPasswd[100];
	string strbuff;
	//printf("pBuf->action   %s\n",pBuf->action);
	if(pBuf->nHasAction)
	{
		strbuff=pBuf->action;
		if(XmlContainString(pBuf->action,"GetDeviceInformation"))
		{
			printf("pBuf->action   %s\n","GetDeviceInformation");
			nLen = BuildDeviceInfoString(TmpBuf,devinfo_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			//printf("TmpBuf2    %s\n",TmpBuf2);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		/*
		 * ::ProcessAction COMMAND http://www.onvif.org/ver10/device/wsdl/GetNetworkDefaultGateway
client connected:192.168.2.115(57055)
client connected:192.168.2.115(57056)
pBuf->action   GetDNS
pBuf->action   GetNTP
onvif connect thread exit
pBuf->action   GetHostnaem
::ProcessAction COMMAND http://www.onvif.org/ver10/device/wsdl/GetDiscoveryMode
::ProcessAction COMMAND http://www.onvif.org/ver10/device/wsdl/GetNetworkProtocols
		 *
		 * */
		else if(XmlContainString(pBuf->action,"GetHostname"))
		{
			printf("pBuf->action   %s\n","GetHostnaem");
			nLen = GetHostnameString(TmpBuf,devinfo_namespaces);//networkinterface_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetNetworkDefaultGateway"))
		{
			printf("pBuf->action   %s\n","GetNetworkDefaultGateway");
			nLen = GetNetworkDefaultGateway(TmpBuf,devinfo_namespaces);//networkinterface_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetDiscoveryMode"))
		{
			printf("pBuf->action   %s\n","GetDiscoveryMode");
			nLen = GetDiscoveryMode(TmpBuf,devinfo_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetNetworkProtocols"))
		{
			printf("pBuf->action   %s\n","GetNetworkProtocols");
			nLen = GetNetworkProtocols(TmpBuf,devinfo_namespaces);//networkinterface_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetNTP"))
		{
			printf("pBuf->action   %s\n","GetNTP");
			nLen = GetNTPString(TmpBuf,devinfo_namespaces);//networkinterface_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetNetworkInterfaces"))
		{
			printf("pBuf->action   %s\n","GetNetworkInterfaces");
			nLen = BuildNetworkInterfaceString(TmpBuf,devinfo_namespaces);//networkinterface_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetDNS"))
		{
			printf("pBuf->action   %s\n","GetDNS");
			nLen = BuildGetDNSString(TmpBuf,devinfo_namespaces);//GetDNS_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetScopes"))
		{
			printf("pBuf->action   %s\n","GetScopes");
			nLen = BuildGetScopesString(TmpBuf,devinfo_namespaces);//GetScopes_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetCapabilities"))
		{
			printf("pBuf->action   %s\n","GetCapabilities");
			nLen = BuildGetCapabilitiesString(TmpBuf,devinfo_namespaces);//GetCapabilities_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetServices"))
		{
			printf("pBuf->action   %s\n","GetServices");
			nLen = BuildGetServicesString(TmpBuf,devinfo_namespaces);//GetServices_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"wsdlGetVideoSources") || XmlContainString(pBuf->action,"GetVideoSources")  )
		{
			printf("pBuf->action   %s\n","wsdlGetVideoSources");
			nLen = BuildWsdlGetVideoSourcesString(TmpBuf,devinfo_namespaces);//wsdlGetVideoSources_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetProfiles"))
		{
			printf("pBuf->action   %s\n","GetProfiles");
			nLen = BuildGetProfilesString(TmpBuf,devinfo_namespaces);//GetProfiles_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetSnapshotUri"))
		{
			printf("pBuf->action   %s\n","GetSnapshotUri");
			nLen = BuildGetSnapshotUriString(TmpBuf,devinfo_namespaces);//GetSnapshotUri_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		
		else if(XmlContainString(pBuf->action,"wsdlGetProfile") || strbuff.find("wsdlGetProfile")!=string::npos)
		{
			printf("pBuf->action   %s\n","wsdlGetProfile");
			nLen = BuildwsdlGetProfileString(TmpBuf,devinfo_namespaces);//WsdlGetProfile_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		//GetVideoEncoderConfigurationOptions
		else if(XmlContainString(pBuf->action,"GetVideoEncoderConfigurationOptions"))
		{
			printf("pBuf->action   %s\n","GetVideoEncoderConfigurationOptions");
			nLen = GetVideoEncoderConfigurationOptionsString(TmpBuf,devinfo_namespaces);//WsdlGetProfile_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		//GetVideoEncoderConfigurations
		else if(XmlContainString(pBuf->action,"GetVideoEncoderConfigurations"))
		{
			printf("pBuf->action   %s\n","GetVideoEncoderConfigurations");
			nLen = GetVideoEncoderConfigurationsString(TmpBuf,devinfo_namespaces);//WsdlGetProfile_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		//GetVideoEncoderConfiguration
		else if(XmlContainString(pBuf->action,"GetVideoEncoderConfiguration"))
		{
			printf("pBuf->action   %s\n","GetVideoEncoderConfiguration");
			nLen = GetVideoEncoderConfigurationString(TmpBuf,devinfo_namespaces);//WsdlGetProfile_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetStreamUri"))
		{
			printf("pBuf->action   %s\n","GetStreamUri");
			char strStream[1024]="0_main";
			XmlGetStringValue(pBuf->Buffer + pBuf->headerlen,"ProfileToken",strStream,1024);
			nLen = BuildGetStreamUriString(TmpBuf,strStream,devinfo_namespaces);//GetStreamUri_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetVideoSourceConfigurations"))
		{
			printf("pBuf->action   %s\n","GetVideoSourceConfigurations");
			char strStream[1024]="0_VSC";
			nLen = GetVideoSourceConfigurationsString(TmpBuf,strStream,devinfo_namespaces);//GetVideoSourceConfiguration_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetVideoSourceConfiguration"))
		{
			printf("pBuf->action   %s\n","GetVideoSourceConfiguration");
			char strStream[1024]="0_VSC";
			nLen = BuildGetVideoSourceConfigureString(TmpBuf,strStream,devinfo_namespaces);//GetVideoSourceConfiguration_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetVideoSources"))
		{
			printf("pBuf->action   %s\n","GetVideoSources");
			//char strStream[1024]="0_VSC";
			nLen = GetVideoSources(TmpBuf,devinfo_namespaces);//GetVideoSourceConfiguration_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetImagingSettings"))
		{
			printf("pBuf->action   %s\n","GetImagingSettings");
			char strStream[1024]="0_VSC";
			nLen = BuildGetImagingSettingsConfigureString(TmpBuf,strStream,devinfo_namespaces);//GetVideoSourceConfiguration_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
			//printf("TmpBuf2 = %s\n", TmpBuf2);
		}
		else if(XmlContainString(pBuf->action,"SetImagingSettings"))
		{
			printf("pBuf->action   %s\n","SetImagingSettings");
			//printf("SetImagingSettings::pBUf = %s\n", pBuf->Buffer);
			char strBrightness[10]={0};
			char strContrast[10]={0};
			char strColorSaturation[10]={0};

			XmlGetStringValue(pBuf->Buffer+pBuf->headerlen,"Brightness",strBrightness,1024);
			XmlGetStringValue(pBuf->Buffer+pBuf->headerlen,"Contrast",strContrast,1024);
			XmlGetStringValue(pBuf->Buffer+pBuf->headerlen,"ColorSaturation",strColorSaturation,1024);
			//printf("strBrightness = %s %s %s\n", strBrightness, strContrast, strColorSaturation);
			nLen = BuildSetImagingSettingsConfigureString(TmpBuf,strBrightness,strColorSaturation,strContrast, devinfo_namespaces);//GetVideoSourceConfiguration_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		
		else if(XmlContainString(pBuf->action,"GetOptions"))
		{
			printf("pBuf->action   %s\n","GetOptions");
			char strStream[1024]="0_VSC";
			nLen = BuildGetOptionsConfigureString(TmpBuf,strStream,devinfo_namespaces);//GetVideoSourceConfiguration_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
			//printf("TmpBuf2 = %s\n", TmpBuf2);
		}
		//[SetVideoEncoderConfigurationResponse]
		else if(XmlContainString(pBuf->action,"SetVideoEncoderConfiguration"))
		{
			printf("pBuf->action   %s\n","SetVideoEncoderConfiguration");
			nLen = SetVideoEncoderConfiguration(TmpBuf,devinfo_namespaces);//WsdlGetProfile_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		//FIXME hard code
		/*else if(XmlContainString(pBuf->action,"CreateUsers"))
		{
			int ret = 0;
			ret = XmlGetStringValue(pBuf,"Username",strUser,100);
			printf("strUser = %s ret = %d\n", strUser, ret );
			ret = XmlGetStringValue(pBuf,"Password",strPasswd,100);
			printf("strUser = %s ret = %d\n", strPasswd, ret );

			nLen = BuildwsdlGetProfileString(TmpBuf,devinfo_namespaces);//WsdlGetProfile_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
*/
		else if(XmlContainString(pBuf->action,"GetUsers"))
		{
			printf("pBuf->action   %s\n","GetUsers");
			char strStream[1024]="0_VSC";
			nLen = BuildGetUsersConfigureString(TmpBuf,strStream,devinfo_namespaces);//GetVideoSourceConfiguration_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
			printf("TmpBuf2 = %s\n", TmpBuf2);
		}
		//FIXME hard code
		/*else if(XmlContainString(pBuf->action,"ContinuousMove"))
		{
			XmlGetStringValue(pBuf,"Velocity",strPTZ,100);
			nLen = BuildwsdlGetProfileString(TmpBuf,devinfo_namespaces);//WsdlGetProfile_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			if(strstr(strPTZ,"x=\"-0.5\""))
				ptzCmd.nCmd = RIGHT_START;
			else if(strstr(strPTZ,"x=\"0.5\""))
				ptzCmd.nCmd = LEFT_START;
			else if(strstr(strPTZ,"y=\"0.5\""))
				ptzCmd.nCmd = DOWN_START;
			else if(strstr(strPTZ,"y=\"-0.5\""))
				ptzCmd.nCmd = UP_START;
			else
				printf("ptz control command error\n");
			ptzControl(0, ptzCmd);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"Stop"))
		{
			ptzCmd.nCmd = LEFT_STOP;
			ptzControl(0, ptzCmd);
			nLen = BuildwsdlGetProfileString(TmpBuf,devinfo_namespaces);//WsdlGetProfile_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}*/
		else
		{
			printf("::ProcessAction COMMAND %s\n", pBuf->action);
			nLen = BuildGetFailString(TmpBuf,devinfo_namespaces);
			nLen2 = BuildHttpFailHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
	}
	else
	{
		char Value[1024];
		if(0 == XmlGetStringValue(pBuf->Buffer+pBuf->headerlen,"InitialTerminationTime",Value,1024))
		{
			char suuid[1024];
			if(0 == XmlGetStringValue(pBuf->Buffer+pBuf->headerlen,"a:MessageID",suuid,1024))
			{
				nLen = BuildGetInitialTerminationTimeString(TmpBuf,suuid,devinfo_namespaces);//GetTerminationTime_namespaces);
				nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
				strcat(TmpBuf2,TmpBuf);
				send(sock,TmpBuf2,nLen + nLen2,0);
			}
		}
		else
		{
			nLen = BuildGetFailString(TmpBuf,devinfo_namespaces);
			nLen2 = BuildHttpFailHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
	}
	//printf("recved %d bytes:%s\n\n\n",nLen+nLen2,pBuf->Buffer);
	//printf("send %d bytes:%s\n\n\n",nLen+nLen2,TmpBuf2);
}


void * ONVIF_NewConnThread(void *pPara)
{
	struct ONVIF_ConnThread *pConnThread = (struct ONVIF_ConnThread *)pPara;
	int sock = pConnThread->sock;
	int ret = 0;
	fd_set fset;
	struct timeval to;
	struct sockaddr_in addr;
	struct Http_Buffer RecvBuf;
	int   nNoDataCount = 0;
	memset(&RecvBuf,0,sizeof(RecvBuf));
	while(1)
	{
		if(nNoDataCount >= 5)
			break;
		FD_ZERO(&fset);
		FD_SET(sock,&fset);
		memset(&to,0,sizeof(to));
		to.tv_sec = 5;
		to.tv_usec = 0;
		ret = select(sock + 1,&fset,NULL,NULL,&to);
		if(ret == 0)
		{
			nNoDataCount ++;
			continue;
		}
		if(ret < 0 && errno == EINTR)
		{
			nNoDataCount ++;
			continue;
		}
		if(ret < 0)
			break;
		if(!FD_ISSET(sock,&fset))
			break;
		ret = recv(sock,RecvBuf.Buffer + RecvBuf.nBufLen,65535 - RecvBuf.nBufLen,0);
		//printf("%s(%d) onvif tcp recved %d bytes\n",inet_ntoa(pConnThread->remote_addr.sin_addr),ntohs(pConnThread->remote_addr.sin_port),ret);
		if(ret <= 0)
			break;
		nNoDataCount = 0;
		RecvBuf.nBufLen += ret;
		//printf("%s(%d) onvif tcp recved total %d bytes:%s\n",inet_ntoa(pConnThread->remote_addr.sin_addr),ntohs(pConnThread->remote_addr.sin_port),RecvBuf.nBufLen,RecvBuf.Buffer);
		ret = HttpParse(&RecvBuf);
		if(ret >= 0)
		{

			ProcessAction(&RecvBuf,sock);
			break;
		}

	}
	printf("onvif connect thread exit\n");
	close(sock);
	free(pConnThread);
	pthread_exit(NULL);
	return NULL;
}

int ONVIFNewConnect(int hsock,struct sockaddr_in *pAddr)
{
	int ret;
	struct ONVIF_ConnThread *pConnThread = (struct ONVIF_ConnThread *)malloc(sizeof(struct ONVIF_ConnThread));
	if(pConnThread == NULL)
	{
		printf("onvif new connect malloc memeory error\n");
		return -1;
	}
	memset(pConnThread,0,sizeof(struct ONVIF_ConnThread));
	pConnThread->sock = hsock;
	memcpy(&pConnThread->remote_addr,pAddr,sizeof(struct sockaddr_in));
	ret = pthread_create(&pConnThread->hthread,NULL,&ONVIF_NewConnThread,pConnThread);
	if(ret < 0)
	{
		printf("create onvif new connect thread error\n");
		close(hsock);
		free(pConnThread);
		return -1;
	}
	return 0;
}

void * ONVIF_ServiceThread(void * param)
{
	int ret = 0;
	int len = 0;
	int opt;
	int hConnSock = -1;
	int hListenSock = -1;
	fd_set fset;
	struct timeval to;
	struct sockaddr_in addr;
	FD_ZERO(&fset);
	memset(&to,0,sizeof(to));
	memset(&addr,0,sizeof(addr));
	len = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(g_wOnvifPort);
	hListenSock = socket(AF_INET,SOCK_STREAM,0);
	if(hListenSock < 0)
	{
		printf("create onvif service listen sock error\n");
		pthread_exit(NULL);
		return NULL;
	}
	int nOne = 1;
	ret = setsockopt(hListenSock, SOL_SOCKET, SO_REUSEADDR, &nOne, sizeof(nOne));
	if (0 > ret)
	{
		perror("Setting REUSEADDR failed");
		return NULL;
	}
	ret = bind(hListenSock,(struct sockaddr *)&addr,sizeof(addr));
	if(ret < 0)
	{
		printf("bind onvif service listen sock error\n");
		close(hListenSock);
		pthread_exit(NULL);
		return NULL;
	}
	ret = listen(hListenSock,200);
	if(ret < 0)
	{
		printf("listen onvif sock error\n");
		close(hListenSock);
		pthread_exit(NULL);
		return NULL;
	}
	while(g_OnvifServiceRunning)
	{
		hConnSock = -1;
		to.tv_sec = 3;
		to.tv_usec = 0;
		FD_SET(hListenSock,&fset);
		ret = select(hListenSock + 1,&fset,NULL,NULL,&to);
		if(ret == 0)
			continue;
		if(ret < 0)
		{
			if(errno == EINTR)
				continue;
			else
			{
				printf("onvif listen sock select error\n");
				break;
			}
		}
		if(!FD_ISSET(hListenSock,&fset))
			continue;
		if(!g_OnvifServiceRunning)
			break;
		hConnSock = accept(hListenSock,(struct sockaddr *)&addr,(socklen_t *)&len);
		if(hConnSock < 0)
		{
			if(errno == 1000)
			{
				printf("too many file opened\n");
				continue;
			}
		}
		printf("client connected:%s(%d)\n",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
		opt = 1;
		ret = setsockopt(hConnSock,IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
		if (ret < 0)
		{
			//printf("set onvif tcp sockopt error\n");
			close(hConnSock);
			continue;
		}
		ONVIFNewConnect(hConnSock,&addr);
	}
	close(hListenSock);
	pthread_exit(NULL);
	return NULL;
}

int ONVIF_ServiceStart()
{
	int ret;
	g_OnvifServiceRunning = 1;
	ret = pthread_create(&g_OnvifServer.hServiceThread,NULL,&ONVIF_ServiceThread,NULL);
	if(ret < 0)
	{
		printf("create onvif service thread error\n");
		return -1;
	}
	return 0;
}

void ONVIF_ServiceStop()
{
	g_OnvifServiceRunning = 0;
	pthread_join(g_OnvifServer.hServiceThread,NULL);
}

/*<trt:GetVideoEncoderConfigurationsResponse>
    <trt:Configurations token="VideoEncoderToken_1">
      <tt:Name>VideoEncoder_1</tt:Name>
      <tt:UseCount>1</tt:UseCount>
      <tt:Encoding>H264</tt:Encoding>
      <tt:Resolution>
        <tt:Width>640</tt:Width>
        <tt:Height>480</tt:Height>
      </tt:Resolution>
     <tt:Quality>3.000000</tt:Quality>
     <tt:RateControl>
       <tt:FrameRateLimit>25</tt:FrameRateLimit>
       <tt:EncodingInterval>1</tt:EncodingInterval>
       <tt:BitrateLimit>1536</tt:BitrateLimit>
     </tt:RateControl>
     <tt:H264>
       <tt:GovLength>25</tt:GovLength>
       <tt:H264Profile>Baseline</tt:H264Profile>
     </tt:H264>
     <tt:Multicast>
       <tt:Address>
         <tt:Type>IPv4</tt:Type>
         <tt:IPv4Address >224.1.2.3</tt:IPv4Address >
       </tt:Address>
       <tt:Port>8600</tt:Port>
       <tt:TTL>1</tt:TTL>
       <tt:AutoStart>false</tt:AutoStart>
     </tt:Multicast>
    <tt:SessionTimeout>PT5S</tt:SessionTimeout>
  </trt:Configurations>

  <trt:Configurations token="VideoEncoderToken_2">
    <tt:Name>VideoEncoder_2</tt:Name>
    <tt:UseCount>1</tt:UseCount>
    <tt:Encoding>H264</tt:Encoding>
    <tt:Resolution>
      <tt:Width>352</tt:Width>
      <tt:Height>288</tt:Height>
    </tt:Resolution>
    <tt:Quality>3.000000</tt:Quality>
    <tt:RateControl>
      <tt:FrameRateLimit>25</tt:FrameRateLimit>
      <tt:EncodingInterval>1</tt:EncodingInterval>
      <tt:BitrateLimit>512</tt:BitrateLimit>
    </tt:RateControl>
    <tt:H264>
      <tt:GovLength>25</tt:GovLength>
      <tt:H264Profile>Baseline</tt:H264Profile>
    </tt:H264>
    <tt:Multicast>
      <tt:Address>
        <tt:Type>IPv4</tt:Type>
        <tt:IPv4Address >224.1.2.3</tt:IPv4Address >
      </tt:Address>
      <tt:Port>8600</tt:Port>
      <tt:TTL>1</tt:TTL>
      <tt:AutoStart>false</tt:AutoStart>
    </tt:Multicast>
    <tt:SessionTimeout>PT5S</tt:SessionTimeout>
  </trt:Configurations>
</trt:GetVideoEncoderConfigurationsResponse>
 * */
