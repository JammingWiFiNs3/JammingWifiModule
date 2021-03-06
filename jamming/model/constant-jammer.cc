/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Network Security Lab, University of Washington, Seattle.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>
 */
 
#include "constant-jammer.h"
#include "ns3/simulator.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"

NS_LOG_COMPONENT_DEFINE ("ConstantJammer");

/*
 * Constant Jammer
 */
 namespace ns3 {
 	
NS_OBJECT_ENSURE_REGISTERED (ConstantJammer);
 	
TypeId
ConstantJammer::GetTypeId (void)
{

  static TypeId tid = TypeId ("ns3::ConstantJammer")
    .SetParent<Jammer> ()
    .AddConstructor<ConstantJammer> ()
    .AddAttribute ("ConstantJammerTxPower",
                   "Power to send jamming signal for constant jammer, in Watts.",
                   DoubleValue (0.001), // 0dBm
                   MakeDoubleAccessor (&ConstantJammer::SetTxPower,
                                       &ConstantJammer::GetTxPower),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ConstantJammerJammingDuration",
                   "Jamming duration for constant jammer.",
                   TimeValue (MilliSeconds (4)),
                   MakeTimeAccessor (&ConstantJammer::SetJammingDuration,
                                     &ConstantJammer::GetJammingDuration),
                   MakeTimeChecker ())
    .AddAttribute ("ConstantJammerConstantInterval",
                   "Constant jammer jamming interval.",
                   TimeValue (MilliSeconds (0.1)),  // Set to 0 for continuous jamming
                   MakeTimeAccessor (&ConstantJammer::SetConstantJammingInterval,
                                     &ConstantJammer::GetConstantJammingInterval),
                   MakeTimeChecker ())
    .AddAttribute ("ConstantJammerRxTimeout",
                   "Constant jammer RX timeout.",
                   TimeValue (Seconds (0.2)),
                   MakeTimeAccessor (&ConstantJammer::SetRxTimeout,
                                     &ConstantJammer::GetRxTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("ConstantJammerReactToMitigationFlag",
                   "Constant jammer react to mitigation flag, set to enable chasing.",
                   UintegerValue (false), // default to no chasing
                   MakeUintegerAccessor (&ConstantJammer::SetReactToMitigation,
                                         &ConstantJammer::GetReactToMitigation),
                   MakeUintegerChecker<bool> ())
    .AddAttribute ("ConstantJammerMaxChannel",
                   "Constant jammer MaxChannel.",
                   UintegerValue (10),
                   MakeUintegerAccessor (&ConstantJammer::SetMaxChannel,
                                     &ConstantJammer::GetMaxChannel),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("ReactByChannelHopMethod",
                     "Mitigation method to use.",
                     UintegerValue (1), // default to FREQUENCY
                     MakeUintegerAccessor (&ConstantJammer::SetJammingReactMethod,
                                           &ConstantJammer::GetJammingReactMethod),
                      MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

ConstantJammer::ConstantJammer ()
  :  m_reactToMitigation (false),
     m_reacting (false),
     premier(0),
     max_channel(11),
     m_jamming(false),
     m_channelOptimal(0)
{
}

ConstantJammer::~ConstantJammer ()
{
}

void
ConstantJammer::SetUtility (Ptr<WirelessModuleUtility> utility)
{
  NS_LOG_FUNCTION (this << utility);
  NS_ASSERT (utility != NULL);
  m_utility = utility;
}

void
ConstantJammer::SetEnergySource (Ptr<EnergySource> source)
{
  NS_LOG_FUNCTION (this << source);
  NS_ASSERT (source != NULL);
  m_source = source;
}

void
ConstantJammer::SetTxPower (double power)
{
  NS_LOG_FUNCTION (this << power);
  m_txPower = power;
}

void
ConstantJammer::SetMaxChannel(uint16_t max){
max_channel=max;
}

uint16_t 
ConstantJammer::GetMaxChannel(void) const{
  return max_channel;
}
double
ConstantJammer::GetTxPower (void) const
{
  NS_LOG_FUNCTION (this);
  return m_txPower;
}

void
ConstantJammer::SetJammingDuration (Time duration)
{
  NS_LOG_FUNCTION (this << duration);
  m_jammingDuration = duration;
}

Time
ConstantJammer::GetJammingDuration (void) const
{
  NS_LOG_FUNCTION (this);
  return m_jammingDuration;
}

void
ConstantJammer::SetConstantJammingInterval (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  NS_ASSERT (interval.GetSeconds () >= 0);
  m_constantJammingInterval = interval;
}

Time
ConstantJammer::GetConstantJammingInterval (void) const
{
  NS_LOG_FUNCTION (this);
  return m_constantJammingInterval;
}

void
ConstantJammer::SetRxTimeout (Time rxTimeout)
{
  NS_LOG_FUNCTION (this << rxTimeout);
  m_rxTimeout = rxTimeout;
}

Time
ConstantJammer::GetRxTimeout (void) const
{
  NS_LOG_FUNCTION (this);
  return m_rxTimeout;
}

void
ConstantJammer::SetReactToMitigation (const bool flag)
{
  NS_LOG_FUNCTION (this << flag);
  m_reactToMitigation = flag;
}

bool
ConstantJammer::GetReactToMitigation (void) const
{
  NS_LOG_FUNCTION (this);
  return m_reactToMitigation;
}

void 
ConstantJammer::SetJammingReactMethod(JammingReactMethod method){
    m_jammingReactMethod = method; 
}

uint32_t 
ConstantJammer::GetJammingReactMethod (void) const {
  return m_jammingReactMethod;
}
  

/*
 * Private functions start here.
 */

void
ConstantJammer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_jammingEvent.Cancel ();
}

void
ConstantJammer::DoStopJammer (void)
{
  NS_LOG_FUNCTION (this);
  m_jammingEvent.Cancel ();
}

void
ConstantJammer::DoJamming (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_utility != NULL);


  if (!IsJammerOn ()) // check if jammer is on
    {
      NS_LOG_DEBUG ("ConstantJammer:At Node #" << GetId () << ", Jammer is OFF!");
      return;
    }

  NS_LOG_DEBUG ("ConstantJammer:At Node #" << GetId () <<
                ", Sending jamming signal with TX power = " << m_txPower <<
                " W" << ", At " << Simulator::Now ().GetSeconds () << "s" << "channel" << m_utility->GetPhyLayerInfo ().currentChannel << "total" << m_utility->GetPhyLayerInfo ().numOfChannels);

  //open RL port is selctionner 

if(m_jamming == false){
    switch (m_jammingReactMethod)
    {
    case RL:
       NS_LOG_DEBUG ("MitigateMethod:At Node #" << GetId () << "RL");
       BeginRLMitigation();
       m_jamming=true;
       break;
    default:
      break;
    }
}
 
  // send jamming signal

  double actualPower = m_utility->SendJammingSignal (m_txPower, m_jammingDuration);
  if (actualPower != 0.0)
    {
      NS_LOG_DEBUG ("ConstantJammer:At Node #" << GetId () <<
                    ", Jamming signal sent with power = " << actualPower << " W");
    }
  else
    {
      NS_LOG_ERROR ("ConstantJammer:At Node #" << GetId () <<
                    ", Failed to send jamming signal!");
    }

  /*
   * Schedule *first* RX timeout if react to mitigation is enabled. We know if
   * react to jamming mitigation is enabled, there should always be a RX timeout
   * event scheduled. The RX timeout event can only "expire" if it's never been
   * scheduled.
   */

  if (m_reactToMitigation)
    {
      NS_LOG_DEBUG ("ConstantJammer:At Node #" << GetId () <<
                    ", After jammer starts, scheduling RX timeout!");
      m_rxTimeoutEvent = Simulator::Schedule (m_rxTimeout,
                                              &ConstantJammer::RxTimeoutHandler,
                                              this);
    }

  m_reacting = false; // always reset reacting flag
   
  
  
  //premier =premier+1;
}

bool
ConstantJammer::DoStartRxHandler (Ptr<Packet> packet, double startRss)
{
  NS_LOG_FUNCTION (this << packet << startRss);

  m_jammingEvent.Cancel (); // cancel previously scheduled event
      // react to packet
      m_jammingEvent = Simulator::Schedule (m_constantJammingInterval,
                                            &ConstantJammer::DoJamming,
                                            this);

  if (m_reactToMitigation )  // check if react to mitigation is enabled
    {
      NS_LOG_DEBUG ("ConstantJammer:At Node #" << GetId () <<
                    ", React to mitigation enabled! Rescheduling RX Timeout");
      // cancel RX timeout event
      m_rxTimeoutEvent.Cancel ();
      // reschedule next RX timeout

      m_rxTimeoutEvent = Simulator::Schedule (m_rxTimeout,
                                              &ConstantJammer::RxTimeoutHandler,
                                              this);                                  
    }
  else
    {
      NS_LOG_DEBUG ("ConstantJammer:At Node #" << GetId () <<
                    ", React to mitigation disabled!");
    }
  return false;
}

bool
ConstantJammer::DoEndRxHandler (Ptr<Packet> packet, double averageRss)
{
  NS_LOG_FUNCTION (this << packet);
  NS_LOG_DEBUG ("ConstantJammer:At Node #" << GetId () <<
                ", Ignoring incoming packet!");
                
  return false;
}

void
ConstantJammer::DoEndTxHandler (Ptr<Packet> packet, double txPower)
{
  NS_LOG_FUNCTION (this << packet << txPower);
  NS_LOG_DEBUG ("ConstantJammer:At Node #" << GetId () <<
                ". Sent jamming burst with power = " << txPower);
  NS_LOG_DEBUG (Simulator::Now ().GetSeconds () <<"test");
  

  m_jammingEvent.Cancel (); // cancel previously scheduled event

  // check to see if we are waiting the jammer to finish reacting to mitigation
  /*if (m_reacting)
    {
      NS_LOG_DEBUG ("ConstantJammer:At Node #" << GetId () <<
                    ", Not sending jamming signal, jammer reacting to mitigation!");
      // get channel switch delay*/
      //Time delay = m_utility->GetPhyLayerInfo().channelSwitchDelay;
       
      
      // schedule next jamming burst after channel switch delay
     
       
      /*  if(m_reacting){
           NS_LOG_DEBUG ("ConstantJammer:At Node #" << GetId () <<
                    ", Not sending jamming signal, jammer reacting to mitigation!");
      // get channel switch delay
      Time delay = m_utility->GetPhyLayerInfo().channelSwitchDelay;
      //Time delay = Seconds(10);
      

      NS_LOG_DEBUG ("ConstantJammer:At Node #" << delay);
       
      m_jammingEvent = Simulator::Schedule (delay,
                                            &ConstantJammer::DoJamming,
                                            this);
                                    
          return;
        }
                
          
          m_jammingEvent = Simulator::Schedule (m_constantJammingInterval,
                                            &ConstantJammer::DoJamming,
                                            this);
        
        NS_LOG_DEBUG (" time after "<<Simulator::GetDelayLeft( m_jammingEvent).GetSeconds());
      */
     
    //}

  // schedule next jamming burst after jamming interval.
  m_jammingEvent = Simulator::Schedule (Seconds(1),
                                        &ConstantJammer::DoJamming, this);
                                        ;

}

void
ConstantJammer::RxTimeoutHandler (void)
{

  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("ConstantJammer:At Node #" << GetId () << ", RX timeout at " <<
                Simulator::Now ().GetSeconds () << "s");
  NS_ASSERT (m_utility != NULL);

  if (!m_reactToMitigation) // check react to mitigation flag
    {
      NS_LOG_DEBUG ("ConstantJammer:At Node #" << GetId () <<
                    ", React to mitigation is turned OFF!");
      return;
    }

  // Calculate channel number.
  uint16_t currentChannel = m_utility->GetPhyLayerInfo ().currentChannel;

  uint16_t nextChannel = currentChannel;
 /* uint16_t nextChannel = currentChannel + 1;
  if (nextChannel >= m_utility->GetPhyLayerInfo ().numOfChannels)
    {
      nextChannel = 1;  // wrap around and start form 1
    }*/

 switch (m_jammingReactMethod)
    {
    case FREQUENCY:
       NS_LOG_DEBUG ("FREQUENCY");
       nextChannel = Increment();
       break;
    case RANDOM:
      NS_LOG_DEBUG ("Random");
       nextChannel =  RandomChannel();
      break;
    case RL:
      NS_LOG_DEBUG ("RL");
       nextChannel =  GetObservationsRl();
      break;
    case OPTIMAL:
      NS_LOG_DEBUG("optimal");
      nextChannel = m_channelOptimal;
      break;
    default:
       NS_LOG_DEBUG("default");
       nextChannel = currentChannel;
      break;

    }

   m_reacting = true;

  NS_LOG_DEBUG ("ConstantJammer:At Node #" << GetId () <<
                ", Switching from channel " << currentChannel << " >-> " <<
                nextChannel << ", At " << Simulator::Now ().GetSeconds () << "s");

  // hop to next channel
 
  m_utility->SwitchChannel (nextChannel);
  

  /*
   * Set reacting flag to indicate jammer is reacting. When the flag is set, no
   * scheduling of new jamming event is allowed.
   */
  
  m_reacting = true;

  // cancel previously scheduled RX timeout
  m_rxTimeoutEvent.Cancel ();

  // schedule next RX timeout
  m_rxTimeoutEvent = Simulator::Schedule (m_rxTimeout,
                                          &ConstantJammer::RxTimeoutHandler,
                                          this);
}


uint16_t
ConstantJammer::RandomChannel (void){
  NS_LOG_INFO("Random" << max_channel);
  uint16_t channelNumber =rand()%(max_channel -  1) + 1;

return channelNumber;
}


uint16_t
ConstantJammer::Increment (void){
  uint16_t currentChannel = m_utility->GetPhyLayerInfo ().currentChannel;
  uint16_t nextChannel = currentChannel + 1;
 
  if (nextChannel >= max_channel)
    {
      nextChannel = 1;  // wrap around and start form 
    }
  /*Time t1= Seconds(2.100);
  NS_LOG_INFO(t1.GetSeconds());
  NS_LOG_INFO(Simulator::Now().GetSeconds());
  if(t1.Compare(Simulator::Now())>=0){
    nextChannel=1;
  }
  else{
    nextChannel=2;
  }*/
  double pdr =   m_utility->GetRss ();
   NS_LOG_INFO(pdr);
  return nextChannel;
}

void
ConstantJammer::BeginRLMitigation(){
   NS_LOG_FUNCTION (this);
   uint32_t openGymPort = 5557;
   Ptr<OpenGymInterface> openGymInterface = CreateObject<OpenGymInterface> (openGymPort);
   m_myGymEnv= CreateObject<MyGymEnvJammer>(m_utility->GetPhyLayerInfo ().currentChannel,12);
   m_myGymEnv->SetOpenGymInterface(openGymInterface);
  }

uint32_t
ConstantJammer::GetObservationsRl(){
     std::ostringstream oss;
     oss.str ("");
     oss << "/NodeList/";
     double rss =   m_utility->GetRssinDbm ();
     NS_LOG_FUNCTION (rss);
     NS_LOG_FUNCTION (m_utility->GetPdr (););
     m_myGymEnv->PerformJamming(m_myGymEnv,m_utility->GetPhyLayerInfo ().currentChannel,rss);
     uint32_t channel = m_myGymEnv->GetChoiseChannel();
     NS_LOG_FUNCTION ("Ok choise"<<channel );

    return channel;
}


void 
ConstantJammer::Optimal(uint32_t optimal){
  NS_LOG_FUNCTION ("Ok choise"<<optimal );
  m_channelOptimal = optimal;
   NS_LOG_FUNCTION ("Ok choise test "<<m_channelOptimal );
  
  //return  m_channelOptimal;

}





} // namespace ns3
  
