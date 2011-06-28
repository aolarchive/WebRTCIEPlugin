// TWebRTCAVControl.h : Declaration of the TWebRTCAVControl
#pragma once
#include "resource.h"       // main symbols
#include <atlctl.h>
#include "WebRTCIEPlugin_i.h"
#include "_IWebRTCAVControlEvents_CP.h"
#include <string>
#include "talk/app/peerconnection.h"
#include "peerconnection/samples/client/conductor.h"
#include "peerconnection/samples/client/peer_connection_client.h"

class TVideoRenderer : public cricket::VideoRenderer
{
public:
    TVideoRenderer();
    virtual ~TVideoRenderer();
    virtual bool SetSize(int width, int height, int reserved);
    virtual bool RenderFrame(const cricket::VideoFrame *frame);
    bool Init(int, int, int, int, HWND);
    bool OnDraw(const RECT&);

private:
    void Paint();
    void OnSize(int width, int height, bool frame_changed);

private:
    int m_x;
    int m_y;

    BITMAPINFO bmi_;
    talk_base::scoped_array<uint8> image_;
    HWND m_hwnd;
    //RECT m_rect;
    PAINTSTRUCT m_ps;
};

// TWebRTCAVControl
class ATL_NO_VTABLE TWebRTCAVControl :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CStockPropImpl<TWebRTCAVControl, IWebRTCAVControl>,
	public IPersistStreamInitImpl<TWebRTCAVControl>,
	public IOleControlImpl<TWebRTCAVControl>,
	public IOleObjectImpl<TWebRTCAVControl>,
	public IOleInPlaceActiveObjectImpl<TWebRTCAVControl>,
	public IViewObjectExImpl<TWebRTCAVControl>,
	public IOleInPlaceObjectWindowlessImpl<TWebRTCAVControl>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<TWebRTCAVControl>,
	public CProxy_IWebRTCAVControlEvents<TWebRTCAVControl>,
	public IPersistStorageImpl<TWebRTCAVControl>,
	public ISpecifyPropertyPagesImpl<TWebRTCAVControl>,
	public IQuickActivateImpl<TWebRTCAVControl>,
#ifndef _WIN32_WCE
	public IDataObjectImpl<TWebRTCAVControl>,
#endif
	public IProvideClassInfo2Impl<&CLSID_WebRTCAVControl, &__uuidof(_IWebRTCAVControlEvents), &LIBID_WebRTCIEPluginLib>,
#ifdef _WIN32_WCE // IObjectSafety is required on Windows CE for the control to be loaded correctly
	public IObjectSafetyImpl<TWebRTCAVControl, INTERFACESAFE_FOR_UNTRUSTED_CALLER>,
#endif
	public CComCoClass<TWebRTCAVControl, &CLSID_WebRTCAVControl>,
	public CComControl<TWebRTCAVControl>
{
public:

	DECLARE_PROTECT_FINAL_CONSTRUCT()
    TWebRTCAVControl();
    HRESULT FinalConstruct();
    void FinalRelease();

DECLARE_OLEMISC_STATUS(OLEMISC_RECOMPOSEONRESIZE |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_INSIDEOUT |
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST
)

DECLARE_REGISTRY_RESOURCEID(IDR_WEBRTCAVCONTROL)


BEGIN_COM_MAP(TWebRTCAVControl)
	COM_INTERFACE_ENTRY(IWebRTCAVControl)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IViewObjectEx)
	COM_INTERFACE_ENTRY(IViewObject2)
	COM_INTERFACE_ENTRY(IViewObject)
	COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceObject)
	COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY(IOleControl)
	COM_INTERFACE_ENTRY(IOleObject)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
	COM_INTERFACE_ENTRY(IQuickActivate)
	COM_INTERFACE_ENTRY(IPersistStorage)
#ifndef _WIN32_WCE
	COM_INTERFACE_ENTRY(IDataObject)
#endif
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
#ifdef _WIN32_WCE // IObjectSafety is required on Windows CE for the control to be loaded correctly
	COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafety)
#endif
END_COM_MAP()

BEGIN_PROP_MAP(TWebRTCAVControl)
END_PROP_MAP()

BEGIN_CONNECTION_POINT_MAP(TWebRTCAVControl)
	CONNECTION_POINT_ENTRY(__uuidof(_IWebRTCAVControlEvents))
END_CONNECTION_POINT_MAP()

BEGIN_MSG_MAP(TWebRTCAVControl)
	CHAIN_MSG_MAP(CComControl<TWebRTCAVControl>)
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()

    // ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)
	{
		static const IID* arr[] =
		{
			&IID_IWebRTCAVControl,
		};

		for (int i=0; i<sizeof(arr)/sizeof(arr[0]); i++)
		{
			if (InlineIsEqualGUID(*arr[i], riid))
				return S_OK;
		}
		return S_FALSE;
	}

    // IViewObjectEx
    DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

    // IAVControl
    STDMETHOD(Init)(BSTR);
    STDMETHOD(SetUserName)(BSTR);
    STDMETHOD(Call)(BSTR);
    STDMETHOD(Hangup)();

    // IWebRTCAVControl
    STDMETHOD(OnSignalingMessage)();
    STDMETHOD(OnSignedIn)();

public:
    HRESULT OnDraw(ATL_DRAWINFO& di);

private:
    HWND GetHWND();

private:
    class TPeerConnectionClientWarpper : public PeerConnectionClientObserver,
                                         public webrtc::PeerConnectionObserver,
                                         public talk_base::Win32Window 
    {
    public:
        TPeerConnectionClientWarpper();
        void Init(IWebRTCAVControl*, HWND);
        void Call(const std::wstring&);
        bool Connect(const std::string& server, int port, const std::wstring& client_name);
        bool SendToPeer();
        bool SignOut();
        bool is_connected() const { return m_peerConnectionClient.is_connected(); }
        bool OnDraw(ATL_DRAWINFO& di);

        const std::string& GetMessage() const { return m_message;}
        void SetMessage(const std::string& message) { m_message = message; }

        // PeerConnectionObserver 
        virtual void OnSignedIn();  // Called when we're logged on.
        virtual void OnDisconnected();
        virtual void OnPeerConnected(int id, const std::string& name);
        virtual void OnPeerDisconnected(int id, const std::string& name);
        virtual void OnMessageFromPeer(int peer_id, const std::string& message);

        // webrtc::PeerConnectionObserver 
        virtual void OnError(){};
        virtual void OnSignalingMessage(const std::string& msg);
        virtual void OnAddStream(const std::string& stream_id, int channel_id, bool video);
        virtual void OnRemoveStream(const std::string& stream_id, int channel_id, bool video);

        //
        // Win32Window implementation.
        //
        virtual bool OnMessage(UINT msg, WPARAM wp, LPARAM lp, LRESULT& result);

    private:
        enum WindowMessages {
            SEND_MESSAGE_TO_PEER = WM_APP + 1,
            MEDIA_CHANNELS_INITIALIZED
        };

    private:
        HRESULT Setup(bool);
        HRESULT StartVideo();
        HRESULT OnMediaChannelsReady();

    private:
        PeerConnectionClient m_peerConnectionClient;
        std::string m_message;
        CComPtr<IWebRTCAVControl> m_spiWebRTCAVControl;

        webrtc::PeerConnection m_peerConnection;
        bool m_waiting_for_audio;
        bool m_waiting_for_video;
        bool m_videoInitialized;
        bool m_initialized;
        std::map<std::string, int> m_peerIdMap;
        HWND m_hWndContainer;
        int m_peerId;
        std::wstring m_peerName;
        TVideoRenderer m_videoRenderer;
        TVideoRenderer m_previewRenderer;
        RECT m_rect;
    };

private:
    HWND m_hWndContainer;
    std::wstring m_userName;
    std::wstring m_peerName;
    TPeerConnectionClientWarpper m_peerConnectionClient;
};

OBJECT_ENTRY_AUTO(__uuidof(WebRTCAVControl), TWebRTCAVControl)
