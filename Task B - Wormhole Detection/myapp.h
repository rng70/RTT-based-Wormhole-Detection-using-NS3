#include "ns3/applications-module.h"
#include "ns3/core-module.h"

using namespace ns3;

class MyApp : public Application
{
public:
    MyApp();
    virtual ~MyApp();
    void Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);
    virtual void PktsAcked(Ptr<TcpSocketState>, uint32_t, const Time &);

    void ScheduleTx(void);
    void SendPacket(void);

    Ptr<Socket> m_socket;
    Address m_peer;
    uint32_t m_packetSize;
    uint32_t m_nPackets;
    DataRate m_dataRate;
    EventId m_sendEvent;
    bool m_running;
    uint32_t m_packetsSent;
};

MyApp::MyApp() : m_socket(0),
                 m_peer(),
                 m_packetSize(0),
                 m_nPackets(0),
                 m_dataRate(0),
                 m_sendEvent(),
                 m_running(false),
                 m_packetsSent(0)
{
}

MyApp::~MyApp()
{
    m_socket = 0;
}

void MyApp::Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_nPackets = nPackets;
    m_dataRate = dataRate;
}

void MyApp::StartApplication(void)
{
    m_running = true;
    m_packetsSent = 0;
    m_socket->Bind();
    m_socket->Connect(m_peer);
    SendPacket();
}

void MyApp::StopApplication(void)
{
    m_running = false;

    if (m_sendEvent.IsRunning())
    {
        Simulator::Cancel(m_sendEvent);
    }

    if (m_socket)
    {
        m_socket->Close();
    }
}

void MyApp::SendPacket(void)
{
    Ptr<Packet> packet = Create<Packet>(m_packetSize);
    m_socket->Send(packet);

    if (++m_packetsSent < m_nPackets)
    {
        ScheduleTx();
    }
}

void MyApp::ScheduleTx(void)
{
    if (m_running)
    {
        Time tNext(Seconds(m_packetSize * 8 / static_cast<double>(m_dataRate.GetBitRate())));
        m_sendEvent = Simulator::Schedule(tNext, &MyApp::SendPacket, this);
    }
}

void MyApp::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time &rtt)
{
    // NS_LOG_FUNCTION(this << tcb << segmentsAcked << rtt);

    if (rtt.IsZero())
    {
        return;
    }

    //m_minRtt = std::min(m_minRtt, rtt);
    NS_LOG_UNCOND("Updated m_minRtt = " << rtt);

    //m_baseRtt = std::min(m_baseRtt, rtt);
    // NS_LOG_DEBUG("Updated m_baseRtt = " << m_baseRtt);

    // Update RTT counter
    // m_cntRtt++;
    // NS_LOG_DEBUG("Updated m_cntRtt = " << m_cntRtt);
}