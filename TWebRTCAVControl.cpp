// WebRTCAVControl.cpp : Implementation of TWebRTCAVControl
#include "stdafx.h"
#include "TWebRTCAVControl.h"
#include "peerconnection/samples/client/peer_connection_client.h"
#include "peerconnection/samples/client/defaults.h"
#include "system_wrappers/source/trace_impl.h"
#include "talk/base/win32window.h"
#include "talk/session/phone/videocommon.h"
#include "talk/session/phone/videorendererfactory.h"

static const int kDefaultWidth = 320;
static const int kDefaultHeight = 240;

TVideoRenderer::TVideoRenderer()
    : m_hwnd(NULL),
      m_x(0),
      m_y(0)
{
}

TVideoRenderer::~TVideoRenderer() 
{
}

bool TVideoRenderer::RenderFrame(const cricket::VideoFrame* frame) 
{
    if (!m_hwnd || !frame) 
        return false;

    // Convert frame to ARGB format, which is accepted by GDI
    frame->ConvertToRgbBuffer(cricket::FOURCC_ARGB, image_.get(),
        bmi_.bmiHeader.biSizeImage,
        bmi_.bmiHeader.biWidth * 4);

    Paint();

    return true;
}

void TVideoRenderer::Paint() 
{
    HDC hdc = GetDC(m_hwnd);

    // strectching/compressing doesn't work
    //int lines = StretchDIBits(hdc,
    //    m_rect.left, m_rect.top, m_rect.right - m_rect.left, m_rect.bottom - m_rect.top,  // destination rect
    //    0, 0, bmi_.bmiHeader.biWidth, -bmi_.bmiHeader.biHeight,  // source rect
    //    image_.get(), &bmi_, DIB_RGB_COLORS, SRCCOPY);

    int lines = StretchDIBits(hdc, m_x, m_y, bmi_.bmiHeader.biWidth, -bmi_.bmiHeader.biHeight, 0, 0, bmi_.bmiHeader.biWidth, 
        -bmi_.bmiHeader.biHeight,  image_.get(), &bmi_, DIB_RGB_COLORS, SRCCOPY);
}

void TVideoRenderer::OnSize(int width, 
                            int height,
                            bool frame_changed) 
{
    if (frame_changed && (width != bmi_.bmiHeader.biWidth ||
        height != -bmi_.bmiHeader.biHeight)) 
    {
        // Update the bmi and image buffer
        bmi_.bmiHeader.biWidth = width;
        bmi_.bmiHeader.biHeight = -height;
        bmi_.bmiHeader.biSizeImage = width * height * 4;
        image_.reset(new uint8[bmi_.bmiHeader.biSizeImage]);
    }
}

bool TVideoRenderer::SetSize(int width, int height, int reserved) 
{
    if (width != bmi_.bmiHeader.biWidth ||
        height != -bmi_.bmiHeader.biHeight) 
    {
        OnSize(width, height, true);
    }
    return true;
}

bool TVideoRenderer::OnDraw(const RECT& rect)
{
    //CopyRect(&m_rect, &rect);
    return true;
}

bool TVideoRenderer::Init(int x, int y, int width, int height, HWND hwnd)
{
    m_x = x;
    m_y = y;

    m_hwnd = hwnd;

    memset(&bmi_.bmiHeader, 0, sizeof(bmi_.bmiHeader));
    bmi_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi_.bmiHeader.biPlanes = 1;
    bmi_.bmiHeader.biBitCount = 32;
    bmi_.bmiHeader.biCompression = BI_RGB;
    bmi_.bmiHeader.biWidth = width;
    bmi_.bmiHeader.biHeight = -height;
    bmi_.bmiHeader.biSizeImage = width * height * 4;

    image_.reset(new uint8[bmi_.bmiHeader.biSizeImage]);

    OnSize(bmi_.bmiHeader.biWidth, -bmi_.bmiHeader.biHeight, false);
    return true;
}

std::string ToUtf8(const std::wstring& wstring)
{
    int count = ::WideCharToMultiByte(CP_UTF8, 0, wstring.c_str(), wstring.length(), NULL, 0, NULL, NULL);
    char* buf = new char[count+1];

    count = ::WideCharToMultiByte(CP_UTF8, 0, wstring.c_str(), wstring.length(), buf, count, NULL, NULL);
    buf[count] = '\0';
    std::string string(buf);
    delete[] buf;

    return string;
}

// TWebRTCAVControl
TWebRTCAVControl::TWebRTCAVControl()
    : m_hWndContainer(NULL)
{
}

HRESULT TWebRTCAVControl::FinalConstruct()
{
    webrtc::Trace::CreateTrace();
    webrtc::Trace::SetTraceFile("webrtc_plugin.log");
    webrtc::Trace::SetLevelFilter(webrtc::kTraceAll);

    return S_OK;
}

void TWebRTCAVControl::FinalRelease()
{
    Hangup();
    return;
}

STDMETHODIMP TWebRTCAVControl::Init(BSTR host)
{
    m_hWndContainer = GetHWND();
    m_peerConnectionClient.Init(this, m_hWndContainer);
    m_peerConnectionClient.Connect(ToUtf8(host), kDefaultServerPort, m_userName);
    return S_OK;
}

STDMETHODIMP TWebRTCAVControl::SetUserName(BSTR userName)
{
    m_userName = userName;
    return S_OK;
}

STDMETHODIMP TWebRTCAVControl::Call(BSTR callee)
{
    m_peerName = callee;
    return S_OK;
}

STDMETHODIMP TWebRTCAVControl::Hangup()
{
    if (m_peerConnectionClient.is_connected())
        m_peerConnectionClient.SignOut();
    return S_OK;
}

STDMETHODIMP TWebRTCAVControl::OnSignalingMessage()
{
    bool ret = m_peerConnectionClient.SendToPeer();
    return ret ? S_OK : S_FALSE;
}

STDMETHODIMP TWebRTCAVControl::OnSignedIn()
{
    m_peerConnectionClient.Call(m_peerName);
    return S_OK;
}

HRESULT TWebRTCAVControl::OnDraw(ATL_DRAWINFO& di)
{
    RECT& rect = *(RECT*)di.prcBounds;
    if (rect.top > 0)
        m_peerConnectionClient.OnDraw(di);
    return S_OK;
}

HWND TWebRTCAVControl::GetHWND()
{
    HWND hwnd = NULL;

    CComPtr<IOleClientSite> spiClientSite;
    HRESULT hr = GetClientSite(&spiClientSite);
    if (spiClientSite)
    {
        CComPtr<IOleInPlaceSiteWindowless> spiIOleInPlaceSiteWindowless; 
        hr = spiClientSite->QueryInterface(IID_IOleInPlaceSiteWindowless, (void**)&spiIOleInPlaceSiteWindowless);

        if (spiIOleInPlaceSiteWindowless)
            spiIOleInPlaceSiteWindowless->GetWindow(&hwnd);
    }

    return hwnd;
}

TWebRTCAVControl::TPeerConnectionClientWarpper::TPeerConnectionClientWarpper()
    : m_peerConnection("STUN stun.l.google.com:19302"),
      m_waiting_for_audio(false),
      m_waiting_for_video(false),
      m_videoInitialized(false),
      m_initialized(false),
      m_hWndContainer(NULL),
      m_peerId(-1)
{
    m_rect.left = m_rect.right = m_rect.top = m_rect.bottom = 0;
    Create(HWND_MESSAGE, L"PeerConnectionClientWarpper", 0, 0, 0, 0, 0, 0);
    m_peerConnectionClient.RegisterObserver(this);
}        

void TWebRTCAVControl::TPeerConnectionClientWarpper::Init(IWebRTCAVControl* piWebRTCAVControl,
                                                          HWND hWndContainer)
{
    m_spiWebRTCAVControl = piWebRTCAVControl;
    m_hWndContainer = hWndContainer;
    m_videoRenderer.Init(100, 100, kDefaultWidth, kDefaultHeight, hWndContainer);
    m_previewRenderer.Init(300, 300, kDefaultWidth, kDefaultHeight, hWndContainer);
}

bool TWebRTCAVControl::TPeerConnectionClientWarpper::Connect(const std::string& server, int port,
                                                             const std::wstring& client_name)
{
    return m_peerConnectionClient.Connect(server, port, ToUtf8(client_name));
}

void TWebRTCAVControl::TPeerConnectionClientWarpper::Call(const std::wstring& peerName) 
{
    m_peerName = peerName;
    if (m_peerName.length())
    {
        Setup(true);
    }
}

bool TWebRTCAVControl::TPeerConnectionClientWarpper::SendToPeer()
{
    return m_peerConnectionClient.SendToPeer(m_peerId, m_message);
}

bool TWebRTCAVControl::TPeerConnectionClientWarpper::SignOut()
{
    return m_peerConnectionClient.SignOut();
}

bool TWebRTCAVControl::TPeerConnectionClientWarpper::OnDraw(ATL_DRAWINFO& di)
{
    if (!m_hWndContainer)
        return true;

    RECT rectClient;
    ::GetClientRect(m_hWndContainer, &rectClient);

    RECT rect;
    ::CopyRect(&rect, &rectClient);

    if(EqualRect(&m_rect, &rect))
        return true;

    int height = rectClient.bottom;
    int width = rectClient.right;
    if (height > kDefaultHeight)
    {
        rect.top = (height - kDefaultHeight) / 2;
        rect.bottom = rect.top + kDefaultHeight;
        height = kDefaultHeight;
    }
    if (width > kDefaultWidth * 2)
    {
        rect.left = (width - kDefaultWidth * 2) / 2;
        rect.right = rect.left + kDefaultWidth * 2;
        width = kDefaultWidth * 2;
    }
    RECT previewRect, videoRect;
    CopyRect(&previewRect, &rect);
    previewRect.right = width / 2 + previewRect.left;
    CopyRect(&videoRect, &rect);
    videoRect.left =  previewRect.right;

    m_videoRenderer.OnDraw(videoRect);
    m_previewRenderer.OnDraw(previewRect);
    return true;
}

void TWebRTCAVControl::TPeerConnectionClientWarpper::OnSignedIn() 
{
    if (m_spiWebRTCAVControl)
        m_spiWebRTCAVControl->OnSignedIn();
}

void TWebRTCAVControl::TPeerConnectionClientWarpper::OnDisconnected() 
{
}

void TWebRTCAVControl::TPeerConnectionClientWarpper::OnPeerConnected(int id, const std::string& name) 
{
    m_peerIdMap.insert(std::pair<std::string, int>(name, id));
}

void TWebRTCAVControl::TPeerConnectionClientWarpper::OnPeerDisconnected(int id, const std::string& name) 
{
    m_peerIdMap.erase(name);
}

void TWebRTCAVControl::TPeerConnectionClientWarpper::OnMessageFromPeer(int peer_id, const std::string& message) 
{
    m_peerId = peer_id;
    Setup(false);
    m_peerConnection.SignalingMessage(message);
}

void TWebRTCAVControl::TPeerConnectionClientWarpper::OnSignalingMessage(const std::string& msg)
{
    StartVideo();
    m_message = msg;
    int peerId = m_peerId;
    if (peerId < 0)
    {
        std::string peer = ToUtf8(m_peerName);
        peerId = m_peerIdMap[peer];
        if (peerId)
            m_peerId = peerId;
    }

    SendMessage(handle(), SEND_MESSAGE_TO_PEER, 0, 0);
}

void TWebRTCAVControl::TPeerConnectionClientWarpper::OnAddStream(const std::string& stream_id, int channel_id, bool video)
{
    if (video)
    {
        m_waiting_for_video = false;
        m_peerConnection.SetVideoRenderer(stream_id, &m_videoRenderer);
    }
    else
    {
        m_waiting_for_audio = false;
    }

    if (!m_waiting_for_video && !m_waiting_for_audio && m_peerName.length())
    {
        SendMessage(handle(), MEDIA_CHANNELS_INITIALIZED, 0, 0);
    }
}

void TWebRTCAVControl::TPeerConnectionClientWarpper::OnRemoveStream(const std::string& stream_id, int channel_id, bool video)
{

}

bool TWebRTCAVControl::TPeerConnectionClientWarpper::OnMessage(UINT msg, WPARAM wp, LPARAM lp, LRESULT& result) 
{
    bool ret = true;
    switch(msg)
    {
    case SEND_MESSAGE_TO_PEER:
        if (m_spiWebRTCAVControl)
            m_spiWebRTCAVControl->OnSignalingMessage();
        break;
    case MEDIA_CHANNELS_INITIALIZED:
        OnMediaChannelsReady();
        break;
    default:
        ret = false;
        break;
    }

    return true;
}

HRESULT TWebRTCAVControl::TPeerConnectionClientWarpper::StartVideo()
{
    if (m_videoInitialized)
        return S_OK;

    m_peerConnection.SetVideoCapture("");
    m_peerConnection.SetLocalVideoRenderer(&m_previewRenderer);
    m_videoInitialized = true;

    return S_OK;
}

HRESULT TWebRTCAVControl::TPeerConnectionClientWarpper::OnMediaChannelsReady()
{
    if (m_peerName.length())
    {
        m_peerConnection.Connect();
        StartVideo();
    }

    return S_OK;
}

HRESULT TWebRTCAVControl::TPeerConnectionClientWarpper::Setup(bool all)
{
    if (m_initialized)
        return S_OK;

    m_peerConnection.Init();
    m_peerConnection.RegisterObserver(this);
    m_peerConnection.SetAudioDevice("", "", 0);

    if (all)
    {
        m_peerConnection.AddStream(kVideoLabel, true);
        m_peerConnection.AddStream(kAudioLabel, false);
        m_waiting_for_video = m_waiting_for_audio = true;
        m_initialized = true;
    }
    return S_OK;
}
