/*
 * openDSME
 *
 * Implementation of the Deterministic & Synchronous Multi-channel Extension (DSME)
 * introduced in the IEEE 802.15.4e-2012 standard
 *
 * Authors: Florian Meier <florian.meier@tuhh.de>
 *          Maximilian Koestler <maximilian.koestler@tuhh.de>
 *          Sandrina Backhauss <sandrina.backhauss@tuhh.de>
 *
 * Based on
 *          DSME Implementation for the INET Framework
 *          Tobias Luebkert <tobias.luebkert@tuhh.de>
 *
 * Copyright (c) 2015, Institute of Telematics, Hamburg University of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef DSMELORAPLATFORM_H
#define DSMELORAPLATFORM_H

#include <stdint.h>
#include <stdlib.h>

#include <omnetpp.h>

#include <inet/linklayer/base/MacProtocolBase.h>
#include <inet/linklayer/contract/IMacProtocol.h>
#include <inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h>

#include "DSMELoRaMessage.h"
#include "dsme_settings.h"
#include "openDSME/helper/DSMEDelegate.h"
#include "openDSME/interfaces/IDSMEPlatform.h"
//#include "openDSME/dsmeAdaptionLayer/DSMEAdaptionLayer.h"
#include "openDSME/mac_services/dataStructures/IEEE802154MacAddress.h"
#include "openDSME/mac_services/mcps_sap/MCPS_SAP.h"
#include "openDSME/mac_services/mlme_sap/MLME_SAP.h"
#include "openDSME/mac_services/pib/MAC_PIB.h"
#include "openDSME/mac_services/pib/PHY_PIB.h"

namespace dsme {

class DSMELayer;

class DSMELoRaPlatform : public inet::MacProtocolBase, public inet::IMacProtocol, public IDSMEPlatform {
    using omnetpp::cIListener::finish;
    using omnetpp::cSimpleModule::send;

protected:
    /****** INET ******/

    virtual void configureNetworkInterface() override;
public:
    DSMELoRaPlatform();
    virtual ~DSMELoRaPlatform();

    DSMELoRaPlatform(const DSMELoRaPlatform&) = delete;
    DSMELoRaPlatform& operator=(const DSMELoRaPlatform&) = delete;

    void handleASSOCIATION_indication(mlme_sap::ASSOCIATE_indication_parameters& params);
    void handleASSOCIATION_confirm(mlme_sap::ASSOCIATE_confirm_parameters& params);
    void handleDISASSOCIATION_confirm(mlme_sap::DISASSOCIATE_confirm_parameters& params);
    void handleStartOfCFP();
    void allocateGTS(uint8_t superframeID, uint8_t slotID, uint8_t channelID, Direction direction, uint16_t address);
    void handleCOMM_STATUS_indication(mlme_sap::COMM_STATUS_indication_parameters& params);
    void handleDSME_GTS_indication(mlme_sap::DSME_GTS_indication_parameters& params);
    void handleDSME_GTS_confirm(mlme_sap::DSME_GTS_confirm_parameters& params);
    void handleDataIndication(mcps_sap::DATA_indication_parameters& params);
    void handleDataConfirm(mcps_sap::DATA_confirm_parameters& params);
    void handleSyncLossIndication(mlme_sap::SYNC_LOSS_indication_parameters& params);
    void handleBEACON_NOTIFY_indication(mlme_sap::BEACON_NOTIFY_indication_parameters& params);
    void handleSCAN_confirm(mlme_sap::SCAN_confirm_parameters& params);
    void request_slot();
    void findFreeSlots(DSMESABSpecification& requestSABSpec, DSMESABSpecification& replySABSpec, uint8_t numSlots, uint16_t preferredSuperframe,
                              uint8_t preferredSlot);
    GTS getNextFreeGTS_CAP_CFP(uint16_t initialSuperframeID, uint8_t initialSlotID, const DSMESABSpecification* sabSpec, GTSType typeOfGTS);
    GTS getNextFreeGTS(uint16_t initialSuperframeID, uint8_t initialSlotID, const DSMESABSpecification* sabSpec);

    /****** OMNeT++ ******/

    /** @brief Initialization of the module and some variables*/
    virtual void initialize(int) override;

    /** @brief Delete all dynamically allocated objects of the module*/
    virtual void finish() override;

    /** @brief Handle messages from lower layer */
    virtual void handleLowerPacket(inet::Packet*) override;

    /** @brief Handle messages from upper layer */
    virtual void handleUpperPacket(inet::Packet*) override;

    /** @brief Handle self messages such as timers */
    virtual void handleSelfMessage(omnetpp::cMessage*) override;

    /** @brief Handle control messages from lower layer */
    virtual void receiveSignal(cComponent *source, omnetpp::simsignal_t signalID, inet::intval_t l, cObject *details) override;

    /****** IDSMERadio ******/

    virtual bool setChannelNumber(uint8_t k) override;
    virtual uint8_t getChannelNumber() override;

    virtual bool prepareSendingCopy(IDSMEMessage* msg, Delegate<void(bool)> txEndCallback) override;

    virtual bool sendNow() override;

    virtual void abortPreparedTransmission() override;

    virtual bool sendDelayedAck(IDSMEMessage* ackMsg, IDSMEMessage* receivedMsg, Delegate<void(bool)> txEndCallback) override;

    virtual void setReceiveDelegate(receive_delegate_t receiveDelegate) override;

    virtual bool startCCA() override;

    virtual void turnTransceiverOn() override;

    virtual void turnTransceiverOff() override;

    /****** IDSMEPlatform ******/

    virtual bool isReceptionFromAckLayerPossible() override;

    virtual void handleReceivedMessageFromAckLayer(IDSMEMessage* message) override;

    virtual DSMELoRaMessage* getEmptyMessage() override;

    virtual void releaseMessage(IDSMEMessage* msg) override;

    virtual void startTimer(uint32_t symbolCounterValue) override;

    virtual uint32_t getSymbolCounter() override;

    virtual uint16_t getRandom() override;

    virtual void updateVisual() override;

    virtual void scheduleStartOfCFP() override;

    virtual uint8_t getMinCoordinatorLQI() override;

    virtual void signalGTSChange(bool deallocation, IEEE802154MacAddress counterpart) override;

    virtual void signalQueueLength(uint32_t length) override;

    virtual void signalPacketsTXPerSlot(uint32_t packets);

    virtual void signalPacketsRXPerSlot(uint32_t packets);

    void startAssociation();

    void startScan();

    void associate(uint16_t coordPANId, AddrMode addrMode, IEEE802154MacAddress& coordAddress, uint8_t channel);

protected:
    PANDescriptor panDescriptorToSyncTo;
    DSMELoRaMessage* getLoadedMessage(inet::Packet*);

    void handleIndicationFromMCPS(IDSMEMessage* msg);
    void handleConfirmFromMCPS(IDSMEMessage* msg, DataStatus::Data_Status status);

    bool send(inet::Packet*);

    void signalNewMsg(DSMELoRaMessage* msg);

    std::string getSequenceChartInfo(IDSMEMessage* msg, bool outgoing);
    std::string getDSMEManagement(uint8_t management, DSMESABSpecification& sabSpec, CommandFrameIdentifier cmd);

    PHY_PIB phy_pib;
    MAC_PIB mac_pib;
    DSMELayer* dsme;
    mcps_sap::MCPS_SAP mcps_sap;
    mlme_sap::MLME_SAP mlme_sap;
    //DSMEAdaptionLayer dsmeAdaptionLayer;

    uint16_t messagesInUse{0};
    uint16_t msgId{0};
    receive_delegate_t receiveFromAckLayerDelegate{};

    std::map<DSMELoRaMessage*, uint16_t> msgMap{};
    std::set<uint16_t> msgsActive{};

    omnetpp::cMessage* timer{nullptr};
    omnetpp::cMessage* ccaTimer{nullptr};
    omnetpp::cMessage* cfpTimer{nullptr};
    Delegate<void(bool)> txEndCallback{};
    inet::Packet* pendingTxPacket{nullptr};
    bool pendingSendRequest{false};

    /** @brief The radio. */
    inet::physicallayer::IRadio* radio{nullptr};
    inet::physicallayer::IRadio::TransmissionState transmissionState{inet::physicallayer::IRadio::TRANSMISSION_STATE_UNDEFINED};

    /** @brief the bit rate at which we transmit */
    double bitrate{0};

    inet::MacAddress addr{};
    bool channelInactive{true};

    bool transceiverIsOn{false};
    uint8_t minBroadcastLQI{0};
    uint8_t minCoordinatorLQI{0};
    uint8_t currentChannel{0};
    bool associationInProgress{false};
    bool scanOrSyncInProgress{false};
    bool syncActive{false};

    int slots{0};
    PanDescriptorList recordedPanDescriptors;

    unsigned long chunk;

public:
    omnetpp::SimTime symbolDuration;

    static omnetpp::simsignal_t broadcastDataSentDown;
    static omnetpp::simsignal_t sentL2CCNFrame;
    static omnetpp::simsignal_t unicastDataSentDown;
    static omnetpp::simsignal_t ackSentDown;
    static omnetpp::simsignal_t beaconSentDown;
    static omnetpp::simsignal_t commandSentDown;
    static omnetpp::simsignal_t uncorruptedFrameReceived;
    static omnetpp::simsignal_t corruptedFrameReceived;
    static omnetpp::simsignal_t gtsChange;
    static omnetpp::simsignal_t queueLength;
    static omnetpp::simsignal_t packetsTXPerSlot;
    static omnetpp::simsignal_t packetsRXPerSlot;
    static omnetpp::simsignal_t commandFrameDwellTime;

public:
    IEEE802154MacAddress& getAddress() {
        return this->mac_pib.macExtendedAddress;
    }

    bool getTransceiverIsOn() const { return transceiverIsOn; }

private:
    //GTSScheduling* scheduling = nullptr;
};

} /* namespace dsme */

#endif /* DSMELORAPLATFORM_H */
