// dllmain.h : Declaration of module class.

class CWebRTCIEPluginModule : public CAtlDllModuleT< CWebRTCIEPluginModule >
{
public :
	DECLARE_LIBID(LIBID_WebRTCIEPluginLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_WEBRTCIEPLUGIN, "{A017B13C-07F9-471F-8C0F-F9C2B2D8C92C}")
};

extern class CWebRTCIEPluginModule _AtlModule;
