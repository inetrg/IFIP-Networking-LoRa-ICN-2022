#include <string.h>
#include <omnetpp.h>

#include "ccnsim_dsme.h"
#include "packets/inet_ccn_interest_m.h"
#include "packets/inet_ccn_data_m.h"
#include "packets/ccnsim_inet_info_m.h"
#include "packets/ccnsim_inet_info_m.h"
#include "ccnsim_dsme_tags_m.h"

#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/networklayer/common/NetworkInterface.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/common/ProtocolTag_m.h"

#include "ccn_data.h"
#include "ccn_interest.h"

#define FACE_DSME_TIMER 123

struct ccn_desc
{
    cMessage *msg;
    int gate;
    int dest_node;
};


    using namespace omnetpp;

    /**
     * Derive the face_dsme class from cSimpleModule.
     */
    class face_dsme : public cSimpleModule
    {
      protected:
        // The following redefined virtual function holds the algorithm.
        virtual void initialize() override;
        virtual void handleMessage(cMessage *msg) override;
        void handleLowerMessage(cMessage *msg);
        void handleUpperMessage(cMessage *msg);
        void sendDSMEMessage(cMessage *msg, int index);
        inet::Packet *ccn_msg_to_inet(cMessage *msg, int dest_face);
        void handle_inet_packet(inet::Packet *packet);
        template <typename T, typename S> void copy_ccn_data(T origin, S dest);
        template <typename T, typename S> void copy_ccn_interest(T origin, S dest);
        void queueCCNData(cMessage *msg);

        std::vector<struct ccn_desc*> data_queue;
        cMessage *timer;
        simsignal_t SentCCNData;
        simsignal_t SentCCNIndication;
        simsignal_t SentCCNInterest;
        simsignal_t SentCCNPush;
        simsignal_t ReceivedCCNData;
        simsignal_t ReceivedCCNIndication;
        simsignal_t ReceivedCCNInterest;
        simsignal_t ReceivedCCNPush;

        bool aggregateCCNData;
    };

        // The module class needs to be registered with OMNeT++
        Define_Module(face_dsme);

void face_dsme::queueCCNData(cMessage *msg)
{
    /* TODO:Don't queue them if data is already there... */
    ccn_data *data = check_and_cast<ccn_data*>(msg);

    for (auto e : data_queue) {
        ccn_data *item = check_and_cast<ccn_data*>(e->msg);
        if (data->getChunk() == item->getChunk()) {
            EV_INFO << "Data was already queued. Drop" << endl;
            /* TODO: Signal event */
            return;
        }
    }

    EV_INFO << "Adding new data to the overload queue" << endl;
    struct ccn_desc *e = new struct ccn_desc;
    e->msg = msg;
    /* TODO: Gate?? */
    //e->gate = getParentModule()->gate("face$o",gate + 1)->getNextGate()->getIndex();
    /* TODO: dest_node?? */
    //e->dest_node = getParentModule()->gate("face$o",gate+1)->getNextGate()->getOwnerModule()->getIndex();
    const_cast<face_dsme*>(this)->emit(SentCCNData, data->getChunk());
    
    data_queue.push_back(e);
}
    void face_dsme::initialize()
    {
        // Initialize is called at the beginning of the simulation.
        EV << "INIT face_dsme" << "\n";

        SentCCNData = registerSignal("SentCCNData");
        SentCCNIndication = registerSignal("SentCCNIndication");
        SentCCNInterest = registerSignal("SentCCNInterest");
        SentCCNPush = registerSignal("SentCCNPush");
        ReceivedCCNData = registerSignal("ReceivedCCNData");
        ReceivedCCNIndication = registerSignal("ReceivedCCNIndication");
        ReceivedCCNInterest = registerSignal("ReceivedCCNInterest");
        ReceivedCCNPush = registerSignal("ReceivedCCNPush");

        /* TODO: Set warmup time */
        timer = new cMessage("timer", FACE_DSME_TIMER);
        /* TODO: Adjust case if beacon interval changes */
        scheduleAt(simTime() + SimTime(960*1.024*8*16, SIMTIME_MS), timer);

        aggregateCCNData = par("aggregateCCNData");
    }

    template <typename T, typename S> void face_dsme::copy_ccn_data(T origin, S dest)
    {
        dest->setChunk(origin->getChunk());
        dest->setPrice(origin->getPrice());
        dest->setTarget(origin->getTarget());
        dest->setOrigin(origin->getOrigin());
        dest->setHops(origin->getHops());
        dest->setTSB(origin->getTSB());
        dest->setTSI(origin->getTSI());
        dest->setCapacity(origin->getCapacity());
        dest->setBtw(origin->getBtw());
        dest->setFound(origin->getFound());
    }

    template <typename T, typename S> void face_dsme::copy_ccn_interest(T origin, S dest)
    {
        dest->setPathArraySize(origin->getPathArraySize());
        for (int k=0; k < origin->getPathArraySize(); k++) {
            dest->setPath(k, origin->getPath(k));
        }
        dest->setChunk(origin->getChunk());
        dest->setHops(origin->getHops());
        dest->setTarget(origin->getTarget());
        dest->setRep_target(origin->getRep_target());
        dest->setBtw(origin->getBtw());
        dest->setTTL(origin->getTTL());
        dest->setNfound(origin->getNfound());
        dest->setCapacity(origin->getCapacity());
        dest->setOrigin(origin->getOrigin());
        dest->setDelay(origin->getDelay());
        dest->setSerialNumber(origin->getSerialNumber());
        dest->setAggregate(origin->getAggregate());
    }

    void face_dsme::handle_inet_packet(inet::Packet *packet)
    {
        cMessage *msg;
        auto info = packet->removeAtFront<inet::ccnsim_inet_info>();
        int dest_face;
        switch(info->getType()) {
            case CCN_D: {
                for (int i=0; i<info->getNum(); i++) {
                    auto payload = packet->removeAtFront<inet::inet_ccn_data>();
                    /* Create and populate CCN interest */
                    msg = new ccn_data("data",CCN_D);

                    /* Create and populate CCN data */
                    ccn_data *data = check_and_cast<ccn_data*>(msg);
                    copy_ccn_data<inet::IntrusivePtr<inet::inet_ccn_data>, ccn_data*>(payload, data);
                    const_cast<face_dsme*>(this)->emit(ReceivedCCNData, payload->getChunk());
                    dest_face = payload->getDest_face();
                    send(msg, "upper_layer$o", dest_face >= 0 ? dest_face - 1 : 0);
                }
                break;
            }
            case CCN_I: {
                auto payload = packet->removeAtFront<inet::inet_ccn_interest>();
                /* Create and populate CCN interest */
                msg = new ccn_interest("interest",CCN_I);
                ccn_interest *interest = check_and_cast<ccn_interest*>(msg);
                copy_ccn_interest<inet::IntrusivePtr<inet::inet_ccn_interest>, ccn_interest*>(payload, interest);
                const_cast<face_dsme*>(this)->emit(ReceivedCCNInterest, payload->getChunk());
                dest_face = payload->getDest_face();
                send(msg, "upper_layer$o", dest_face - 1);
                break;
            }
            case INDICATION: {
                auto payload = packet->removeAtFront<inet::inet_ccn_interest>();
                /* Create and populate CCN interest */
                msg = new ccn_interest("indication",INDICATION);
                ccn_interest *interest = check_and_cast<ccn_interest*>(msg);
                copy_ccn_interest<inet::IntrusivePtr<inet::inet_ccn_interest>, ccn_interest*>(payload, interest);
                const_cast<face_dsme*>(this)->emit(ReceivedCCNIndication, payload->getChunk());
                dest_face = payload->getDest_face();
                send(msg, "upper_layer$o", dest_face - 1);
                break;
            }
            case PUSH: {
                auto payload = packet->removeAtFront<inet::inet_ccn_data>();
                /* Create and p opulate CCN interest */
                msg = new ccn_data("push",PUSH);

                /* Create and populate CCN data */
                ccn_data *data = check_and_cast<ccn_data*>(msg);
                copy_ccn_data<inet::IntrusivePtr<inet::inet_ccn_data>, ccn_data*>(payload, data);

                const_cast<face_dsme*>(this)->emit(ReceivedCCNPush, payload->getChunk());
                dest_face = payload->getDest_face();
                send(msg, "upper_layer$o", dest_face - 1);

                break;

           }
            default:
                throw cRuntimeError("handle_inet_packet: Unknown CCN packet type!") ;
        }

    }

    inet::Packet *face_dsme::ccn_msg_to_inet(cMessage *msg, int dest_face)
    {
        inet::Packet *packet;
        int face = getParentModule()->gate("face$o",dest_face + 1)->getNextGate()->getIndex();

        char ccn_str[64];
        switch (msg->getKind())
        {
            case CCN_D:
            {
                ccn_data *tmp_data = (ccn_data *)msg;
                sprintf(ccn_str, "ccn_data-%llu", tmp_data->getChunk());
                packet = new inet::Packet(ccn_str);
                auto payload = inet::makeShared<inet::inet_ccn_data>();

                copy_ccn_data<ccn_data*, inet::IntrusivePtr<inet::inet_ccn_data>>(tmp_data, payload);

                /* TODO: Chunk shouldn't be zero, otherwise debug mode fails */
                payload->setChunkLength(inet::B(16));
                payload->setDest_face(face);
                packet->insertAtFront(payload);
                const_cast<face_dsme*>(this)->emit(SentCCNData, tmp_data->getChunk());

                auto tag = packet->addTag<inet::ccnSimDSMEtags>();
                if (par("dataCFP"))
                {
                    tag->setInfo(inet::CFP);
                }
                else
                {
                    tag->setInfo(inet::CAP);
                }
                tag->setChunk(payload->getChunk());

                break;
            }

            case CCN_I:
            {
                ccn_interest *tmp_int = (ccn_interest *)msg;
                sprintf(ccn_str, "ccn_interest-%llu", tmp_int->getChunk());
                packet = new inet::Packet(ccn_str);
                auto payload = inet::makeShared<inet::inet_ccn_interest>();
                copy_ccn_interest<ccn_interest*, inet::IntrusivePtr<inet::inet_ccn_interest>>(tmp_int, payload);


                /* TODO: Chunk shouldn't be zero, otherwise debug mode fails */
                payload->setChunkLength(inet::B(20));
                payload->setDest_face(face);
                packet->insertAtFront(payload);
                const_cast<face_dsme*>(this)->emit(SentCCNInterest, tmp_int->getChunk());

                auto tag = packet->addTag<inet::ccnSimDSMEtags>();
                if (par("interestCFP"))
                {
                    tag->setInfo(inet::CFP);
                }
                else
                {
                    tag->setInfo(inet::CAP);
                }
                tag->setChunk(payload->getChunk());

                break;
            }
            case INDICATION:
            {
                ccn_interest *tmp_int = (ccn_interest *)msg;
                sprintf(ccn_str, "ccn_interest_indication-%llu", tmp_int->getChunk());
                packet = new inet::Packet(ccn_str);
                auto payload = inet::makeShared<inet::inet_ccn_interest>();
                copy_ccn_interest<ccn_interest*, inet::IntrusivePtr<inet::inet_ccn_interest>>(tmp_int, payload);


                /* TODO: Chunk shouldn't be zero, otherwise debug mode fails */
                payload->setChunkLength(inet::B(20));
                payload->setDest_face(face);
                packet->insertAtFront(payload);
                const_cast<face_dsme*>(this)->emit(SentCCNIndication, tmp_int->getChunk());

                auto tag = packet->addTag<inet::ccnSimDSMEtags>();
                if (par("indicationCFP"))
                {
                    tag->setInfo(inet::CFP);
                }
                else
                {
                    tag->setInfo(inet::CAP);
                }
                tag->setChunk(payload->getChunk());

                break;
            }
            case PUSH:
                {
                ccn_data *tmp_data = (ccn_data *)msg;
                sprintf(ccn_str, "ccn_push-%llu", tmp_data->getChunk());
                packet = new inet::Packet(ccn_str);
                auto payload = inet::makeShared<inet::inet_ccn_data>();

                copy_ccn_data<ccn_data*, inet::IntrusivePtr<inet::inet_ccn_data>>(tmp_data, payload);

                /* TODO: Chunk shouldn't be zero, otherwise debug mode fails */
                payload->setChunkLength(inet::B(16));
                payload->setDest_face(face);
                packet->insertAtFront(payload);
                const_cast<face_dsme*>(this)->emit(SentCCNPush, tmp_data->getChunk());

                auto tag = packet->addTag<inet::ccnSimDSMEtags>();
                if (par("pushCFP"))
                {
                    tag->setInfo(inet::CFP);
                }
                else
                {
                    tag->setInfo(inet::CAP);
                }
                tag->setChunk(payload->getChunk());

                break;

                }
            default:
                throw cRuntimeError("ccn_msg_to_inet: Unknown CCN packet type!") ;
        }


            auto info = inet::makeShared<inet::ccnsim_inet_info>();
            info->setType(msg->getKind());
            /* Aggregated packets don't get to this point */
            info->setNum(1);
            info->setChunkLength(inet::B(1));
            packet->insertAtFront(info);

            return packet;
    }

void face_dsme::handleLowerMessage(cMessage *msg)
{
    // find out dst face and send msg there
    inet::Packet *packet = check_and_cast<inet::Packet*>(msg);
    handle_inet_packet(packet);
    delete msg;
}

void face_dsme::sendDSMEMessage(cMessage *msg, int index)
{
    /* Queue messages if sendBroadcastData is set */
    if (aggregateCCNData && msg->getKind() == CCN_D) {
        queueCCNData(msg);
        return;
    }

    auto packet = ccn_msg_to_inet(msg, index);
    // get network interface of the host assiciated with this node
    inet::NetworkInterface *netif = check_and_cast<inet::NetworkInterface*>(getParentModule()->getSubmodule("wlan", 0));

    // get network interface of the host associated to the node we want to reach (dest)
    inet::NetworkInterface *dest_netif = check_and_cast<inet::NetworkInterface*>(getParentModule()->gate("face$o",index+1)->getNextGate()->getOwnerModule()->getSubmodule("wlan", 0));

    // tag with associated host interface
    packet->addTag<inet::InterfaceReq>()->setInterfaceId(netif->getInterfaceId());

    // tag with remote host (dest) MAC address
    packet->addTag<inet::MacAddressReq>()->setDestAddress(dest_netif->getMacAddress());

    // tag with local host source address
    packet->addTagIfAbsent<inet::MacAddressReq>()->setSrcAddress(netif->getMacAddress());

    packet->addTag<inet::PacketProtocolTag>()->setProtocol(&inet::Protocol::arp);

    send(packet, "lower_layer$o");
    // send(msg, "lower_layer$o", msg->getArrivalGate()->getIndex());
}

void face_dsme::handleUpperMessage(cMessage *msg)
{
    /* Check if packet should be sent to the DSME mac (if any) or using the
     * face */
    int index = msg->getArrivalGate()->getIndex();

    int dest_wlans = getParentModule()->gate("face$o",index+1)->getNextGate()->getOwnerModule()->par("numWlanInterfaces");
    int wlans = getAncestorPar("numWlanInterfaces");

    bool send_with_dsme = dest_wlans && wlans;
    if (send_with_dsme) {
        sendDSMEMessage(msg, index);
    }
    else {
        /* face_dsme.upper_layer[n] maps to core_layer.face[n+1].
         * At the same time, core_layer.face[n] maps to node.face[n] in the
         * original implementation.
         * Since face_dsme.face[n] maps to node.face[n+1], the index of
         * face_dsme.upper_layer and face_dsme.face are the same
         */
        send(msg, "face$o", msg->getArrivalGate()->getIndex());
    }
}

    void face_dsme::handleMessage(cMessage *msg)
    {
        // EV << msg << "\n";
        // EV << msg->getArrivalGate() << "\n";
        // EV << msg->getArrivalGate()->getIndex() << "\n";
        // EV << msg->getArrivalGate()->getFullName() << "\n";
        // EV << msg->getArrivalGate()->str() << "\n";

        EV << "node["<<getIndex()<<"]: face_dsme::handleMessage\n";
        if (msg->isSelfMessage()) {
            /* Self message */
            if (msg->getKind() == FACE_DSME_TIMER && data_queue.size() > 0) {
                inet::Packet *packet = new inet::Packet("ccn_agg_data", CCN_D);

                int num = 0;
                /* TODO */
                int max_data = 6;
                bool drop = true;
                for (auto e : data_queue) {
                    auto payload = inet::makeShared<inet::inet_ccn_data>();
                    copy_ccn_data<ccn_data*, inet::IntrusivePtr<inet::inet_ccn_data>>(check_and_cast<ccn_data*>(e->msg), payload);
                    payload->setChunkLength(inet::B(16));
                    /* This one doesn't require destination */
                    payload->setDest_face(-1);
                    packet->insertAtFront(payload);
                    num++;
                    delete e->msg;
                    delete e;
                    if (max_data-- == 0) {
                        break;
                    }
                }
                if (drop) {
                    data_queue.clear();
                }
                //auto r1 = packet->removeAtFront<inet::inet_ccn_interest>();
                ////auto r2 = packet->removeAtFront<inet::inet_ccn_interest>();
                inet::NetworkInterface *netif = check_and_cast<inet::NetworkInterface*>(getParentModule()->gate("data_face$o")->getNextGate()->getOwnerModule()->getSubmodule("wlan", 0));

                // tag with associated host interface
                packet->addTag<inet::InterfaceReq>()->setInterfaceId(netif->getInterfaceId());

                // tag with remote host (dest) MAC address
                inet::MacAddress to;
                to.setBroadcast();
                packet->addTag<inet::MacAddressReq>()->setDestAddress(to);
                auto tag = packet->addTag<inet::ccnSimDSMEtags>();
                tag->setInfo(inet::CAP);

                // tag with local host source address
                packet->addTagIfAbsent<inet::MacAddressReq>()->setSrcAddress(netif->getMacAddress());

                packet->addTag<inet::PacketProtocolTag>()->setProtocol(&inet::Protocol::arp);
                auto info = inet::makeShared<inet::ccnsim_inet_info>();
                info->setNum(num);
                info->setType(CCN_D);
                //auto creationTimeTag = info->addTag<inet::CreationTimeTag>();
                //creationTimeTag->setCreationTime(simTime());
                info->setChunkLength(inet::B(1));
                packet->insertAtFront(info);

                EV_INFO << "Sending aggregated CCN Data" << endl;
                send(packet, "lower_layer$o");
                
            }
            /* TODO */
            scheduleAt(simTime() + SimTime(960*1.024*8*16, SIMTIME_MS), timer);
            return;
        }
        else {
            // simply forward message to the same gate number
            std::string str = msg->getArrivalGate()->getFullName();
            if (str.find("lower") != std::string::npos)
            {
                EV << "RX from lower, send up;" << str  << "\n";
                handleLowerMessage(msg);
            }
            else if(str.find("upper") != std::string::npos) {
                EV << "RX from upper, send down;" << str << "\n";
                handleUpperMessage(msg);
            }
            else if(str.find("face") != std::string::npos) {
                int index = msg->getArrivalGate()->getIndex();

                /* face_dsme.upper_layer[n] maps to core_layer.face[n+1].
                 * At the same time, core_layer.face[n] maps to node.face[n] in the
                 * original implementation.
                 * Since face_dsme.face[n] maps to node.face[n+1], the index of
                 * face_dsme.upper_layer and face_dsme.face are the same
                 */
                send(msg, "upper_layer$o", msg->getArrivalGate()->getIndex());
            }
        }
    }
