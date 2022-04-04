#include"DSMELoRaPlatform.h"

#include <iomanip>
#include <stdlib.h>

#include "./StaticSchedule.h"
#include "LoRaAppDummy.h"

#include "ccnsim_dsme_tags_m.h"

#include <inet/common/ModuleAccess.h>
#include <inet/linklayer/common/InterfaceTag_m.h>
#include <inet/linklayer/common/MacAddressTag_m.h>
#include <inet/common/ProtocolTag_m.h>
#include <inet/common/ProtocolGroup.h>
#include <inet/common/packet/chunk/ByteCountChunk.h>
#include <inet/physicallayer/wireless/common/base/packetlevel/FlatRadioBase.h>
#include <inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h>
#include <inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h>

#include "openDSME/dsmeLayer/DSMELayer.h"
#include "openDSME/dsmeLayer/messages/MACCommand.h"
//#include "openDSME/dsmeAdaptionLayer/scheduling/PIDScheduling.h"
//#include "openDSME/dsmeAdaptionLayer/scheduling/TPS.h"
//#include "openDSME/dsmeAdaptionLayer/scheduling/StaticScheduling.h"
#include "openDSME/mac_services/pib/dsme_phy_constants.h"

#include "LoRa/LoRaRadio.h"
#include "LoRa/LoRaTagInfo_m.h"
#include "LoRa/LoRaMacFrame_m.h"

using namespace inet;
using namespace inet::physicallayer;

Define_Module(dsme::DSMELoRaPlatform);

namespace dsme {
simsignal_t DSMELoRaPlatform::broadcastDataSentDown;
simsignal_t DSMELoRaPlatform::unicastDataSentDown;
simsignal_t DSMELoRaPlatform::commandSentDown;
simsignal_t DSMELoRaPlatform::beaconSentDown;
simsignal_t DSMELoRaPlatform::sentL2CCNFrame;
simsignal_t DSMELoRaPlatform::ackSentDown;
simsignal_t DSMELoRaPlatform::uncorruptedFrameReceived;
simsignal_t DSMELoRaPlatform::corruptedFrameReceived;
simsignal_t DSMELoRaPlatform::gtsChange;
simsignal_t DSMELoRaPlatform::queueLength;
simsignal_t DSMELoRaPlatform::packetsTXPerSlot;
simsignal_t DSMELoRaPlatform::packetsRXPerSlot;
simsignal_t DSMELoRaPlatform::commandFrameDwellTime;

static int get_absolute_slot_id(int superframe_id, int rel_slot_id)
{
    if (superframe_id == 0) {
        return rel_slot_id;
    }
    else {
        return rel_slot_id + 8;
    }
}

static int get_absolute_superframe_id(bool cap_reduction, int pos)
{
    if (pos < 7) {
        return 0;
    }

    if (cap_reduction) {
        return 1 + ((pos - 7) / 15);
    }
    else {
        return (pos / 7);
    }
}

static int get_absolute_slot_id(bool cap_reduction, int pos)
{
    if (pos < 7) {
        return pos;
    }

    if (cap_reduction) {
        return (pos - 7) % 15;
    }
    else {
        return (pos % 7) + 8;
    }
}

static int get_num_available_slots(bool cap_reduction, int MO)
{
    if (cap_reduction) {
        return (15 * (1<<(MO-3))) - 8;
    }
    else {
        return 7*(1<<(MO-3));
    }
}

static void translateMacAddress(MacAddress& from, IEEE802154MacAddress& to) {
    // TODO only handles short address
    if(from.isBroadcast()) {
        to = IEEE802154MacAddress(IEEE802154MacAddress::SHORT_BROADCAST_ADDRESS);
    } else {
        to.setShortAddress((from.getAddressByte(4) << 8) | from.getAddressByte(5));
    }
}

static void translateMacAddress(IEEE802154MacAddress& from, MacAddress& to) {
    if (from == IEEE802154MacAddress(IEEE802154MacAddress::SHORT_BROADCAST_ADDRESS)) {
        to.setBroadcast();
    } else {
        to.setAddress("00 00 00 00 00 00");
        to.setAddressByte(4, from.getShortAddress() >> 8);
        to.setAddressByte(5, (from.getShortAddress() & 0xFF));
    }
}

static uint8_t PERtoLQI(double per) {
    // inverse function of the graph given in the ATmega256RFR2 datasheet
    double lqi = -22.2222 * log(0.00360656 * (-1 + (1 / (1 - per))));
    if(lqi > 255) {
        lqi = 255;
    } else if(lqi < 0) {
        lqi = 0;
    }
    return (uint8_t)(lqi + 0.5);
}

static std::string getErrorInfo(inet::Packet* packet) {
    auto errorRateInd = packet->getTag<inet::ErrorRateInd>();

    std::stringstream ss;
    ss << std::setprecision(3) << errorRateInd->getBitErrorRate() * 100.0 << "%, ";
    ss << errorRateInd->getPacketErrorRate() * 100.0 << "%, ";
    ss << "LQI " << (uint16_t) PERtoLQI(errorRateInd->getPacketErrorRate());
    return ss.str();
}

DSMELoRaPlatform::DSMELoRaPlatform()
    : phy_pib(),
      mac_pib(phy_pib),

      dsme(new DSMELayer()),
      mcps_sap(*dsme),
      mlme_sap(*dsme){
    broadcastDataSentDown = registerSignal("broadcastDataSentDown");
    unicastDataSentDown = registerSignal("unicastDataSentDown");
    commandSentDown = registerSignal("commandSentDown");
    beaconSentDown = registerSignal("beaconSentDown");
    ackSentDown = registerSignal("ackSentDown");
    sentL2CCNFrame = registerSignal("sentL2CCNFrame");
    uncorruptedFrameReceived = registerSignal("uncorruptedFrameReceived");
    corruptedFrameReceived = registerSignal("corruptedFrameReceived");
    gtsChange = registerSignal("GTSChange");
    queueLength = registerSignal("queueLength");
    packetsTXPerSlot = registerSignal("packetsTXPerSlot");
    packetsRXPerSlot = registerSignal("packetsRXPerSlot");
    commandFrameDwellTime = registerSignal("commandFrameDwellTime");
}

DSMELoRaPlatform::~DSMELoRaPlatform() {
    delete dsme;
    //delete scheduling;

    cancelAndDelete(ccaTimer);
    cancelAndDelete(cfpTimer);
    cancelAndDelete(timer);
}

/****** INET ******/

void DSMELoRaPlatform::configureNetworkInterface() {
    NetworkInterface *e = getContainingNicModule(this);

    // data rate
    e->setDatarate(bitrate);

    // generate a link-layer address to be used as interface token for IPv6
    e->setMacAddress(addr);
    e->setInterfaceToken(addr.formInterfaceIdentifier());

    // capabilities
    e->setMtu(par("mtu").intValue());
    e->setMulticast(true);
    e->setBroadcast(true);
}

void DSMELoRaPlatform::request_slot()
{
    if (this->mac_pib.macIsPANCoord) {
        throw cRuntimeError("GW should not request slots");
    }
    int host_id = getParentModule()->getParentModule()->getIndex();

    /* Superframe ID 0 has slots [0-6]. All the other Superframes have [0-8] if
     * CAP Reduction is off. Otherwise, these Superframes have [0-14].
     */
    bool cap_reduction = this->mac_pib.macCapReduction;
    int pos = host_id - 1;
    int rx_pos;
    int tx_pos;
    if (par("gtsTx") && par("gtsRx")) {
        rx_pos = 2 * pos;
        tx_pos = rx_pos + 1;
    }
    else {
        rx_pos = pos;
        tx_pos = pos;
    }

    int tx_superframe_id = get_absolute_superframe_id(cap_reduction, tx_pos);
    int tx_slot_id = get_absolute_slot_id(cap_reduction, tx_pos);

    int rx_superframe_id = get_absolute_superframe_id(cap_reduction, rx_pos);
    int rx_slot_id = get_absolute_slot_id(cap_reduction, rx_pos);

    cModule *submod = getSimulation()->getModuleByPath("ccnsim_dsme_network.node[0].wlan[0].mac");
    DSMELoRaPlatform *gw = check_and_cast<DSMELoRaPlatform*>(submod);

    if (par("gtsTx")) {
        EV_INFO << "Host[" << getParentModule()->getParentModule()->getIndex() << "]: GTS: SId: " << tx_superframe_id << " slotID: " << tx_slot_id << " Direction: TX" << endl;
        gw->allocateGTS(tx_superframe_id, tx_slot_id, 0, Direction::RX, this->mac_pib.macExtendedAddress.getShortAddress());
        allocateGTS(tx_superframe_id, tx_slot_id, 0, Direction::TX, this->panDescriptorToSyncTo.coordAddress.getShortAddress());
        recordScalar("txSuperframeID", tx_superframe_id);
        recordScalar("txSlotID", tx_slot_id);
    }

    if (par("gtsRx")) {
        EV_INFO << "Host[" << getParentModule()->getParentModule()->getIndex() << "]: GTS: SId: " << rx_superframe_id << " slotID: " << rx_slot_id << " Direction: RX" << endl;
        gw->allocateGTS(rx_superframe_id, rx_slot_id, 0, Direction::TX, this->mac_pib.macExtendedAddress.getShortAddress());
        allocateGTS(rx_superframe_id, rx_slot_id, 0, Direction::RX, this->panDescriptorToSyncTo.coordAddress.getShortAddress());
        recordScalar("rxSuperframeID", rx_superframe_id);
        recordScalar("rxSlotID", rx_slot_id);
    }
}

/****** IEEE 802.15.4 SAPs ******/
void DSMELoRaPlatform::handleASSOCIATION_indication(mlme_sap::ASSOCIATE_indication_parameters& params) {
    LOG_INFO("Association requested from 0x" << params.deviceAddress.getShortAddress() << ".");

    mlme_sap::ASSOCIATE::response_parameters response_params;
    response_params.deviceAddress = params.deviceAddress;
    response_params.assocShortAddress = params.deviceAddress.a4();
    response_params.status = AssociationStatus::SUCCESS;
    response_params.channelOffset = this->mac_pib.macChannelOffset;
    if(params.hoppingSequenceRequest == true || params.hoppingSequenceId == 1) {
        response_params.hoppingSequence = this->mac_pib.macHoppingSequenceList;
    }

    // TODO update list of associated devices

    this->mlme_sap.getASSOCIATE().response(response_params);
    return;
}

void DSMELoRaPlatform::handleDISASSOCIATION_confirm(mlme_sap::DISASSOCIATE_confirm_parameters& params) {
    //this->disassociationCompleteDelegate(params.status);
    throw cRuntimeError("handleDISASSOCIATION_confirm");
    return;
}

void DSMELoRaPlatform::handleASSOCIATION_confirm(mlme_sap::ASSOCIATE_confirm_parameters& params) {
    //this->associationCompleteDelegate(params.status);
    if(params.status == AssociationStatus::SUCCESS) {
        LOG_INFO("Association completed successfully.");
        recordScalar("associationTimestamp", simTime());
        if (!par("isPANCoordinator")) {
            request_slot();
            EV_INFO << "Node doesn't have slot. Requesting one" << endl;
        }

    } else {
        LOG_ERROR("Association failed.");
        this->scanOrSyncInProgress = false;
        startAssociation();
    }
}

void DSMELoRaPlatform::allocateGTS(uint8_t superframeID, uint8_t slotID, uint8_t channelID, Direction direction, uint16_t address) {
    this->dsme->getMessageDispatcher().addNeighbor(IEEE802154MacAddress(address));
    this->mac_pib.macDSMEACT.add(superframeID, slotID, channelID, direction, address, ACTState::VALID);
}

void DSMELoRaPlatform::handleStartOfCFP() {
    for(DSMEAllocationCounterTable::iterator it = this->mac_pib.macDSMEACT.begin(); it != this->mac_pib.macDSMEACT.end(); ++it) {
        it->resetIdleCounter();
    }

    return;
}

void DSMELoRaPlatform::handleCOMM_STATUS_indication(mlme_sap::COMM_STATUS_indication_parameters& params) {
    LOG_INFO("COMM_STATUS indication handled.");

    // TODO should we do anything here? especially for failures?

    return;
}

GTS DSMELoRaPlatform::getNextFreeGTS_CAP_CFP(uint16_t initialSuperframeID, uint8_t initialSlotID, const DSMESABSpecification* sabSpec, GTSType typeOfGTS) {


    DSMEAllocationCounterTable& macDSMEACT = this->mac_pib.macDSMEACT;
    DSMESlotAllocationBitmap& macDSMESAB = this->mac_pib.macDSMESAB;

    uint8_t numChannels = this->mac_pib.helper.getNumChannels();
    uint8_t numSuperFramesPerMultiSuperframe = this->mac_pib.helper.getNumberSuperframesPerMultiSuperframe();

    uint16_t slotsToCheck = 0;
    uint8_t numGTSlots = 0;

    GTS gts(0, 0, 0);

    BitVector<MAX_CHANNELS> occupied;
    BitVector<MAX_CHANNELS> remoteOccupied; // only used if sabSpec != nullptr
    occupied.setLength(numChannels);

    if(sabSpec != nullptr) {
            DSME_ASSERT(sabSpec->getSubBlockIndex() == initialSuperframeID);
            remoteOccupied.setLength(numChannels);
        }

    if (typeOfGTS == GTSType::GTS_CFP){

        slotsToCheck = this->mac_pib.helper.getNumGTSlots(0) +
                       (this->mac_pib.helper.getNumberSuperframesPerMultiSuperframe() - 1) *
                       this->mac_pib.helper.getNumGTSlots(0);

        numGTSlots = this->mac_pib.helper.getNumGTSlots(0); // numGTS in SF


    }else if (typeOfGTS == GTSType::GTS_CAP){

        gts = GTS(1,0,0);

        slotsToCheck = (this->mac_pib.helper.getNumberSuperframesPerMultiSuperframe() - 1) *
                        (this->mac_pib.helper.getNumGTSlots(0) +1); // 8 slots per SF
        numGTSlots = (this->mac_pib.helper.getNumGTSlots(0)+1); // numGTS available in CAP 8 slots

        if(gts.superframeID ==0){
            LOG_DEBUG("gts undefined because there is no slots for CAP in SF 0");
            return GTS::UNDEFINED;
            }
    }

    LOG_DEBUG(". initialSFid -> " << (uint16_t) initialSuperframeID << " initial slotID -> "<< (uint16_t)initialSlotID);
    for(gts.superframeID = initialSuperframeID; slotsToCheck > 0; gts.superframeID = (gts.superframeID + 1) % numSuperFramesPerMultiSuperframe) {


        if(sabSpec != nullptr && gts.superframeID != initialSuperframeID) {
            // currently per convention a sub block holds exactly one superframe
            return GTS::UNDEFINED;
        }


        //redefinition of superframe id for gts in cfp, when superframeId ==0
        if(typeOfGTS == GTSType::GTS_CAP){
            // proof of concept CAPON OFF. idea is to select only solts number 7 to 15. for selecting only available slots in CAP when CAPreduction is on or on/off
            if(gts.superframeID ==0){
                LOG_DEBUG(". getnextFreeGTS -> superframeID==0 bypass SF = 0");
                gts.superframeID=1;
            }
        }

         LOG_INFO("Checking " <<  (uint16_t) numGTSlots << " in superframe " << gts.superframeID);

         // TODO in case the initial slot id is not in the range (8,14) is it neccesary?
         uint8_t delimiter = numGTSlots;
         if (typeOfGTS == GTSType::GTS_CFP){
             if(gts.superframeID!=0){
                 delimiter = 15; //the limit for slots in CFP for SF!=0
             }
         }

         for(gts.slotID = initialSlotID % delimiter; slotsToCheck > 0; gts.slotID = (gts.slotID + 1) % delimiter) {

             if (typeOfGTS == GTSType::GTS_CFP){
                 if(gts.superframeID!=0){
                     delimiter =15;
                     if (gts.slotID < 8){
                         gts.slotID = 8;
                     }
                 }else if(gts.superframeID==0){
                     delimiter = numGTSlots;
                 }
             }
            if(!macDSMEACT.isAllocated(gts.superframeID, gts.slotID)) {

                uint8_t startChannel = this->dsme->getPlatform().getRandom() % numChannels;
                macDSMESAB.getOccupiedChannels(occupied, gts.superframeID, gts.slotID);
                if(sabSpec != nullptr) {
                    remoteOccupied.copyFrom(sabSpec->getSubBlock(), gts.slotID * numChannels);
                    occupied.setOperationJoin(remoteOccupied);
                }

                gts.channel = startChannel;
                for(uint8_t i = 0; i < numChannels; i++) {
                    if(!occupied.get(gts.channel)) {
                        /* found one */
                        //PRoof of concept CAPon CAPoff. Idea-> after a GTS is found in GTS_CAP. check if the next one is available as well
                        if(typeOfGTS == GTSType::GTS_CAP){
                            // TODO CHECK if slot is odd and THE NEXT SLOT IS AVAILABLE(?)

                            if ((gts.superframeID != 0) && (gts.slotID < 8)){
                                return gts;
                            }

                        }else if (typeOfGTS == GTSType::GTS_CFP){
                            if(((gts.superframeID==0)&&(gts.slotID<7))||((gts.superframeID!=0) && (gts.slotID>7) && (gts.slotID <15))){
                                return gts;
                            }

                        }

                    }

                    gts.channel++;
                    if(gts.channel == numChannels) {
                        gts.channel = 0;
                    }
                }
            }
            slotsToCheck--;

            // calculate the next slot id to check

            //special case is for CFP next slot, in which the next slot is not in range slotId(8,14)
            if(typeOfGTS == GTSType::GTS_CFP){

                if(((gts.slotID+1)% 15 <8) && (gts.superframeID!=0)){
                gts.slotID = 7;
                }
                if((gts.superframeID!=0)) {
                    if ((((gts.slotID+1)%15) == initialSlotID)){
                        LOG_DEBUG(". getnextFreeGTS -> gts.slotsID+ 1 == initialSuperframeID");
                        break;
                    }
                    if ((gts.slotID+1)%this->mac_pib.helper.getNumGTSlots(0) == initialSlotID) {
                        LOG_DEBUG(". getnextFreeGTS -> gts.slotsID+ 1 == initialSuperframeID");
                        break;
                    }
                }

            }else if(typeOfGTS == GTSType::GTS_CAP){
                if(gts.superframeID !=0){
                    if((gts.slotID+1)%((this->mac_pib.helper.getNumGTSlots(0))+1) == initialSlotID) {
                        LOG_DEBUG(". getnextFreeGTS -> gts.slotsID+ 1 == initialSuperframeID");
                        break;
                    }
                }
            }
        }
    }

    return GTS::UNDEFINED;
}
GTS DSMELoRaPlatform::getNextFreeGTS(uint16_t initialSuperframeID, uint8_t initialSlotID, const DSMESABSpecification* sabSpec) {
    GTS gts(0,0,0);

    if (((initialSlotID < 7) && (initialSuperframeID == 0)) || ((initialSlotID > 7) && (initialSlotID < 15) && (initialSuperframeID != 0))){ //SF=0 , slots between 0-6 are part of CFP
        gts = getNextFreeGTS_CAP_CFP(initialSuperframeID, initialSlotID, sabSpec, GTSType::GTS_CFP);
        if (gts == GTS::UNDEFINED){
            initialSlotID = 0;
            if(initialSuperframeID == 0){
                return GTS::UNDEFINED;
            }
            gts = getNextFreeGTS_CAP_CFP(initialSuperframeID, initialSlotID, sabSpec, GTSType::GTS_CAP);
            return gts;
            }
        return gts;

        }else if((initialSuperframeID !=0) && (initialSlotID < 8)){
            gts = getNextFreeGTS_CAP_CFP(initialSuperframeID, initialSlotID, sabSpec, GTSType::GTS_CAP);
            if (gts == GTS::UNDEFINED){
                initialSlotID = 8;
                gts = getNextFreeGTS_CAP_CFP(initialSuperframeID, initialSlotID, sabSpec, GTSType::GTS_CFP);
                return gts;
            }
            return gts;
    }else if (((initialSlotID > 6) && (initialSlotID < 15) && (initialSuperframeID == 0))){
        return GTS::UNDEFINED;
    }
    return GTS::UNDEFINED;
}
void DSMELoRaPlatform::findFreeSlots(DSMESABSpecification& requestSABSpec, DSMESABSpecification& replySABSpec, uint8_t numSlots, uint16_t preferredSuperframe,
                              uint8_t preferredSlot) {
    const uint8_t numChannels = this->mac_pib.helper.getNumChannels();

    GTS gts = GTS(preferredSuperframe,preferredSlot,0);

    for(uint8_t i = 0; i < numSlots; i++) {

        //proof of concept CAPoff capOn
        if (i == 0){
            gts = getNextFreeGTS(preferredSuperframe, preferredSlot, &requestSABSpec);
        }

        if(gts == GTS::UNDEFINED) {
            break;
        }

        /* mark slot as allocated */
        replySABSpec.getSubBlock().set(gts.slotID * numChannels + gts.channel, true);

        if(i < numSlots - 1) {
            /* mark already allocated slots as occupied for next round */
            for(uint8_t channel = 0; channel < numChannels; channel++) {
                requestSABSpec.getSubBlock().set(gts.slotID * numChannels + channel, true);
            }

            gts.slotID = gts.slotID+ 1;
        }
    }
    return;
}
void DSMELoRaPlatform::handleDSME_GTS_indication(mlme_sap::DSME_GTS_indication_parameters& params) {
    if(!this->mac_pib.macAssociatedPANCoord) {
        LOG_INFO("Not associated, discarding incoming GTS request from " << params.deviceAddress << ".");
        return;
    }

    LOG_INFO("GTS request handled.");

    mlme_sap::DSME_GTS::response_parameters responseParams;
    responseParams.deviceAddress = params.deviceAddress;
    responseParams.managementType = params.managementType;
    responseParams.direction = params.direction;
    responseParams.prioritizedChannelAccess = params.prioritizedChannelAccess;

    bool sendReply = true;

    switch(params.managementType) {
        case ALLOCATION: {
            responseParams.dsmeSabSpecification.setSubBlockLengthBytes(params.dsmeSabSpecification.getSubBlockLengthBytes());
            responseParams.dsmeSabSpecification.setSubBlockIndex(params.dsmeSabSpecification.getSubBlockIndex());

            DSME_ASSERT(params.dsmeSabSpecification.getSubBlockIndex() == params.preferredSuperframeId);

            findFreeSlots(params.dsmeSabSpecification, responseParams.dsmeSabSpecification, params.numSlot, params.preferredSuperframeId,
                          params.preferredSlotId);

            responseParams.channelOffset = this->mac_pib.macChannelOffset;
            if(responseParams.dsmeSabSpecification.getSubBlock().isZero()) {
                LOG_ERROR("Unable to allocate GTS.");
                responseParams.status = GTSStatus::DENIED;
            } else {
                responseParams.status = GTSStatus::SUCCESS;
            }
            break;
        }
        case DUPLICATED_ALLOCATION_NOTIFICATION: {
            LOG_ERROR("DUPLICATED_ALLOCATION_NOTIFICATION");
            //ASSERT(FALSE);
            // TODO what shall we do here? With this the deallocation could be to aggressive
            uint16_t address = IEEE802154MacAddress::NO_SHORT_ADDRESS;
            Direction direction;
            throw cRuntimeError("DUPLICATED_ALLOCATION");
            //responseParams.status = verifyDeallocation(params.dsmeSabSpecification, address, direction);

            if(responseParams.status == GTSStatus::SUCCESS) {
                // Now handled by the ACTUpdater this->dsmeAdaptionLayer.getMAC_PIB().macDSMEACT.setACTState(params.dsmeSABSpecification, INVALID);
            } else {
                // the deallocated slot is not allocated, so send back as DENIED
            }

            sendReply = false; // TODO correct?

            break;
        }
        case DEALLOCATION: {
            Direction directionUnused;
            throw cRuntimeError("DEALLOCATION");
            //responseParams.status = verifyDeallocation(params.dsmeSabSpecification, params.deviceAddress, directionUnused);

            if(responseParams.status == GTSStatus::GTS_Status::SUCCESS) {
                // TODO remove
                // only set to INVALID here and remove them not before NOTIFY
                // if anything goes wrong, the slot will be deallocated later again
                // Now handled by the ACTUpdater this->dsmeAdaptionLayer.getMAC_PIB().macDSMEACT.setACTState(params.dsmeSABSpecification, INVALID);
                //this->dsmeAdaptionLayer.getMAC_PIB().macDSMEACT.setACTState(params.dsmeSABSpecification, INVALID);// ADDED to test
            }

            responseParams.dsmeSabSpecification = params.dsmeSabSpecification;
            break;
        }
        case EXPIRATION:
            // In this implementation EXPIRATION is only issued while no confirm is pending
            // DSME_ASSERT(!gtsConfirmPending);

            // TODO is this required?
            // this->dsmeAdaptionLayer.getMAC_PIB().macDSMEACT.setACTState(params.dsmeSABSpecification, DEALLOCATED);

            throw cRuntimeError("EXPIRATION");
            //sendDeallocationRequest(params.deviceAddress, params.direction, params.dsmeSabSpecification);
            sendReply = false;
            break;
        default:
            DSME_ASSERT(false);
            break;
    }

    if(sendReply) {
        this->mlme_sap.getDSME_GTS().response(responseParams);
    }

    return;
}

void DSMELoRaPlatform::handleDSME_GTS_confirm(mlme_sap::DSME_GTS_confirm_parameters& params) {
    //LOG_DEBUG("GTS confirmation handled (Status: " << printStatus(params.status) << ").");

    // TODO handle channel access failure! retransmission?

    if(params.managementType == ManagementType::ALLOCATION) {
        //gtsConfirmPending = false;
        LOG_DEBUG("gtsConfirmPending = false");
        if(params.status == GTSStatus::SUCCESS) {
            LOG_DEBUG(". GTS confirmed");
            //this->dsmeAdaptionLayer.getMessageHelper().sendRetryBuffer();
        }
        if(params.status != GTSStatus::TRANSACTION_OVERFLOW) {
            //performSchedulingAction(this->gtsScheduling->getNextSchedulingAction());//  line that is used to enable if consecutive GTS negotiations are triggered
        }
    }
    return;
}

void DSMELoRaPlatform::handleDataIndication(mcps_sap::DATA_indication_parameters& params) {
    LOG_DEBUG("Received DATA message from MCPS.");
    //this->dsmeAdaptionLayer.getGTSHelper().indicateReceivedMessage(params.msdu->getHeader().getSrcAddr().getShortAddress());
    handleIndicationFromMCPS(params.msdu);
    return;
}

void DSMELoRaPlatform::handleDataConfirm(mcps_sap::DATA_confirm_parameters& params) {
    LOG_DEBUG("Received DATA confirm from MCPS");
    IDSMEMessage* msg = params.msduHandle;

    DSME_ASSERT(msg->getCurrentlySending());
    msg->setCurrentlySending(false);

    if(params.status != DataStatus::SUCCESS) {
        if(params.status == DataStatus::INVALID_GTS) {
            EV_INFO << "INVALID_GTS" << endl;
            /*
            if(queueMessageIfPossible(msg)) {
                return;
            }
            */

            // GTS slot not yet allocated
            LOG_DEBUG("DROPPED->" << params.msduHandle->getHeader().getDestAddr().getShortAddress() << ": No GTS allocated + queue full");
        } else if(params.status == DataStatus::NO_ACK) {
            // This should not happen, but might be the case for temporary inconsistent slots
            LOG_DEBUG("DROPPED->" << params.msduHandle->getHeader().getDestAddr().getShortAddress() << ": No ACK");
        } else if(params.status == DataStatus::CHANNEL_ACCESS_FAILURE) {
            // This should not happen, but might be the case if a foreign packet is received and the phy is therefore busy
            //DSME_SIM_ASSERT(false);
            LOG_DEBUG("DROPPED->" << params.msduHandle->getHeader().getDestAddr().getShortAddress() << ": Channel Access Failure");
        } else if(params.status == DataStatus::TRANSACTION_OVERFLOW) {
            // Queue is full
            LOG_DEBUG("DROPPED->" << params.msduHandle->getHeader().getDestAddr().getShortAddress() << ": Queue full");
        } else if(params.status == DataStatus::TRANSACTION_EXPIRED) {
            // Transaction expired, e.g. for RESET
            LOG_DEBUG("DROPPED->" << params.msduHandle->getHeader().getDestAddr().getShortAddress() << ": Expired");
        } else {
            // Should not occur
            DSME_ASSERT(false);
        }
        // TODO specialize!
        // TODO verify that Ack in GTS is always successful for simulation
    }

    if(params.gtsTX) {
        EV_INFO << "TODO: params.gtsTX" << endl;
#if 0
        int32_t serviceTime =
            (int32_t) this->dsmeAdaptionLayer.getDSME().getPlatform().getSymbolCounter() - (int32_t)msg->getStartOfFrameDelimiterSymbolCounter();
        this->dsmeAdaptionLayer.getGTSHelper().indicateOutgoingMessage(params.msduHandle->getHeader().getDestAddr().getShortAddress(),
                                                                       params.status == DataStatus::SUCCESS, serviceTime, params.msduHandle->queueAtCreation);
#endif
    }

    handleConfirmFromMCPS(params.msduHandle, params.status);
}

void DSMELoRaPlatform::handleSyncLossIndication(mlme_sap::SYNC_LOSS_indication_parameters& params) {
    throw cRuntimeError("handleSyncLossIndication");
#if 0
    if(params.lossReason == LossReason::BEACON_LOST) {
        LOG_ERROR("Beacon tracking lost.");
        // DSME_SIM_ASSERT(false); TODO
    } else {
        LOG_ERROR("Tracking lost for unsupported reason: " << (uint16_t)params.lossReason);
        DSME_ASSERT(false);
    }

    if(this->syncActive) {
        this->syncActive = false;
        this->scanAndSyncCompleteDelegate(nullptr);
    } else {
        this->syncLossAfterSyncedDelegate();
    }
    return;
#endif
}

void DSMELoRaPlatform::associate(uint16_t coordPANId, AddrMode addrMode, IEEE802154MacAddress& coordAddress, uint8_t channel)
{
    CapabilityInformation capabilityInformation;
    capabilityInformation.alternatePANCoordinator = false;
    capabilityInformation.deviceType = 1;
    capabilityInformation.powerSource = 0;
    capabilityInformation.receiverOnWhenIdle = 1;
    capabilityInformation.associationType = 1; // TODO: FastA? 1 -> yes, 0 -> no
    capabilityInformation.reserved = 0;
    capabilityInformation.securityCapability = 0;
    capabilityInformation.allocateAddress = 1;

    mlme_sap::ASSOCIATE::request_parameters params;
    params.channelNumber = channel;
    params.channelPage = this->phy_pib.phyCurrentPage;
    params.coordAddrMode = SHORT_ADDRESS;
    params.coordPanId = coordPANId;
    params.coordAddress = coordAddress;
    params.capabilityInformation = capabilityInformation;
    params.channelOffset = this->mac_pib.macChannelOffset;
    params.hoppingSequenceId = 1;
    params.hoppingSequenceRequest = true;

    // TODO start timer for macMaxFrameTotalWaitTime, report NO_DATA on timeout
    this->mlme_sap.getASSOCIATE().request(params);
}

void DSMELoRaPlatform::handleBEACON_NOTIFY_indication(mlme_sap::BEACON_NOTIFY_indication_parameters& params) {
    if(!this->mac_pib.macAutoRequest) {
        LOG_INFO("Beacon registered in upper layer.");
        EV_INFO << "TODO: record beacon" << endl;
        //this->recordedPanDescriptors.add(params.panDescriptor);
    }

    uint16_t shortAddress = params.panDescriptor.coordAddress.getShortAddress();
#if 0
    if(!heardCoordinators.contains(shortAddress)) {
        heardCoordinators.add(shortAddress);
    }
#endif

    if(this->syncActive) {
        // TODO check if the beacon is actually from the PAN described in the activePanDescriptor
        this->syncActive = false;
        /* Associate */
        associate(this->panDescriptorToSyncTo.coordPANId, this->panDescriptorToSyncTo.coordAddrMode, this->panDescriptorToSyncTo.coordAddress,
                                                                 this->panDescriptorToSyncTo.channelNumber);
        //handleSCAN_confirm(&this->panDescriptorToSyncTo);
    }

    // TODO CROSS-LAYER-CALLS, no interface for this information
#if 0
    LOG_INFO("Checking whether to become a coordinator: "
             << "isAssociated:" << this->mac_pib.macAssociatedPANCoord << ", isCoordinator:"
             << this->mac_pib.macIsCoord << ", numHeardCoordinators:" << ((uint16_t)heardCoordinators.getLength()) << ".");
    if(this->mac_pib.macAssociatedPANCoord && !this->mac_pib.macIsCoord && heardCoordinators.getLength() < 2) {
        uint16_t random_value = this->dsmeAdaptionLayer.getDSME().getPlatform().getRandom() % 3;
        if(random_value < 1) {
            mlme_sap::START::request_parameters request_params;
            request_params.panCoordinator = false;
            // TODO: fill rest;

            LOG_INFO("Turning into a coordinator now.");
            this->dsmeAdaptionLayer.getMLME_SAP().getSTART().request(request_params);

            mlme_sap::START_confirm_parameters confirm_params;
            bool confirmed = this->dsmeAdaptionLayer.getMLME_SAP().getSTART().confirm(&confirm_params);
            DSME_ASSERT(confirmed);
            DSME_ASSERT(confirm_params.status == StartStatus::SUCCESS);
        }
    }

    return;
#endif
}

void DSMELoRaPlatform::handleSCAN_confirm(mlme_sap::SCAN_confirm_parameters& params) {
    DSME_ASSERT(!this->syncActive);

    PanDescriptorList* list;

    if(!this->mac_pib.macAutoRequest) {
        list = &this->recordedPanDescriptors;
    } else {
        list = &params.panDescriptorList;
    }

    if(list->size() == 0) {
        EV_INFO << "SCAN: Found no candidates" << endl;
        this->scanOrSyncInProgress = false;
        startScan();
        return;
    }

    // Find the coordinator with the best LQI,
    // then find the coordinator with this LQI,
    // but the best RSSI.

    uint8_t bestLQI = 0;
    int8_t bestRSSI = INT8_MIN;
    uint8_t bestIdx = 0xFF;

    for(uint8_t i = 0; i < list->size(); i++) {
        if((*list)[i].linkQuality > bestLQI) {
            bestLQI = (*list)[i].linkQuality;
        }
    }

    if(bestLQI < this->dsme->getPlatform().getMinCoordinatorLQI()) {
        //this->scanAndSyncCompleteDelegate(nullptr);
        EV_INFO << "SCAN: No candidate with LQI above threshold" << endl;
        startScan();
        return;
    }

    for(uint8_t i = 0; i < list->size(); i++) {
        if((*list)[i].linkQuality == bestLQI) {
            if((*list)[i].rssi > bestRSSI) {
                bestRSSI = (*list)[i].rssi;
                bestIdx = i;
            }
        }
    }

    this->panDescriptorToSyncTo = (*list)[bestIdx];

    mlme_sap::SYNC::request_parameters syncParams;
    syncParams.channelNumber = this->panDescriptorToSyncTo.channelNumber;
    syncParams.channelPage = this->panDescriptorToSyncTo.channelPage;
    syncParams.trackBeacon = true;
    syncParams.syncParentShortAddress = this->panDescriptorToSyncTo.coordAddress.getShortAddress();
    syncParams.syncParentSdIndex = this->panDescriptorToSyncTo.dsmePANDescriptor.getBeaconBitmap().getSDIndex();
    this->syncActive = true;
    EV_INFO << "SCAN: complete. Requesting to sync" << endl;
    this->mlme_sap.getSYNC().request(syncParams);

    return;
}

/****** OMNeT++ ******/

void DSMELoRaPlatform::startScan()
{
    channelList_t scanChannels;
    scanChannels.add(par("commonChannel"));
    uint8_t scanDuration = par("scanDuration").intValue();
    if(!this->scanOrSyncInProgress) {
        this->scanOrSyncInProgress = true;
        DSME_ASSERT(!this->syncActive);

        //this->recordedPanDescriptors.clear();

        mlme_sap::SCAN::request_parameters params;

        LOG_INFO("Initiating passive scan");
        params.scanType = ScanType::PASSIVE;

        params.scanChannels = scanChannels;
        params.scanDuration = scanDuration;
        params.channelPage = this->phy_pib.phyCurrentPage;
        params.linkQualityScan = false;

        this->mlme_sap.getSCAN().request(params);
    } else {
        LOG_INFO("Scan already in progress.");
    }
}

void DSMELoRaPlatform::startAssociation()
{
    if(!this->associationInProgress) {
        startScan();
    } else {
        LOG_INFO("Association already in progress.");
    }
}

void DSMELoRaPlatform::initialize(int stage) {
    MacProtocolBase::initialize(stage);

    if(stage == INITSTAGE_LOCAL) {
        /* 11 <= phyCurrentChannel <= 26 for 2450 MHz band DSSS */
        channelList_t DSSS2450_channels(16);
        for(uint8_t i = 0; i < 16; i++) {
            DSSS2450_channels[i] = 11 + i;
        }
        this->phy_pib.setDSSS2450ChannelPage(DSSS2450_channels);

        this->dsme->setPHY_PIB(&(this->phy_pib));
        this->dsme->setMAC_PIB(&(this->mac_pib));
        this->dsme->setMCPS(&(this->mcps_sap));
        this->dsme->setMLME(&(this->mlme_sap));

        const char* schedulingSelection = par("scheduling");

        /* NEW */
        this->mlme_sap.getASSOCIATE().indication(DELEGATE(&DSMELoRaPlatform::handleASSOCIATION_indication, *this));
        this->mlme_sap.getASSOCIATE().confirm(DELEGATE(&DSMELoRaPlatform::handleASSOCIATION_confirm, *this));

        this->mlme_sap.getDISASSOCIATE().confirm(DELEGATE(&DSMELoRaPlatform::handleDISASSOCIATION_confirm, *this));

        this->dsme->setStartOfCFPDelegate(DELEGATE(&DSMELoRaPlatform::handleStartOfCFP, *this)); /* BAD cross-layer hack */

        this->mlme_sap.getDSME_GTS().indication(DELEGATE(&DSMELoRaPlatform::handleDSME_GTS_indication, *this));
        this->mlme_sap.getDSME_GTS().confirm(DELEGATE(&DSMELoRaPlatform::handleDSME_GTS_confirm, *this));
        this->mlme_sap.getCOMM_STATUS().indication(DELEGATE(&DSMELoRaPlatform::handleCOMM_STATUS_indication, *this));

        this->mcps_sap.getDATA().indication(DELEGATE(&DSMELoRaPlatform::handleDataIndication, *this));
        this->mcps_sap.getDATA().confirm(DELEGATE(&DSMELoRaPlatform::handleDataConfirm, *this));

        this->mlme_sap.getBEACON_NOTIFY().indication(DELEGATE(&DSMELoRaPlatform::handleBEACON_NOTIFY_indication, *this));
        this->mlme_sap.getSCAN().confirm(DELEGATE(&DSMELoRaPlatform::handleSCAN_confirm, *this));
        this->mlme_sap.getSYNC_LOSS().indication(DELEGATE(&DSMELoRaPlatform::handleSyncLossIndication, *this));

        /* END */
#if 0
        if(!strcmp(schedulingSelection, "PID")) {
            scheduling = new PIDScheduling(this->dsmeAdaptionLayer);
        }
        else if(!strcmp(schedulingSelection, "TPS")) {
            TPS* tps = new TPS(this->dsmeAdaptionLayer);
            tps->setAlpha(par("TPSalpha").doubleValue());
            tps->setMinFreshness(this->mac_pib.macDSMEGTSExpirationTime);
            tps->setUseHysteresis(par("useHysteresis").boolValue());
            tps->setUseMultiplePacketsPerGTS(par("sendMultiplePacketsPerGTS").boolValue());
            scheduling = tps;
        }
        else if(!strcmp(schedulingSelection, "STATIC")) {
            scheduling = new StaticScheduling(this->dsmeAdaptionLayer);
        }
        else {
            ASSERT(false);
        }

        uint8_t scanDuration = par("scanDuration").intValue();

        this->dsmeAdaptionLayer.initialize(scanChannels,scanDuration,scheduling);
#endif

        /* Initialize Address */
        IEEE802154MacAddress address;
        const char* addrstr = par("address");

        if(!strcmp(addrstr, "auto")) {
            // assign automatic address
            addr = MacAddress::generateAutoAddress();

            // change module parameter from "auto" to concrete address
            par("address").setStringValue(addr.str().c_str());
        } else {
            addr.setAddress(addrstr);
        }

        translateMacAddress(addr, this->mac_pib.macExtendedAddress);
        //registerInterface();

        /* Find Radio Module */
        cModule* radioModule = getModuleFromPar<cModule>(par("radioModule"), this);
        radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radioModule->subscribe(IRadio::receptionStateChangedSignal, this);
        radio = check_and_cast<IRadio*>(radioModule);

        symbolDuration = SimTime(1024, SIMTIME_US);
        timer = new cMessage();
        cfpTimer = new cMessage();
        ccaTimer = new cMessage();

        // check parameters for consistency
        // aTurnaroundTimeSymbols should match (be equal or bigger) the RX to TX
        // switching time of the radio
        if(radioModule->hasPar("timeRXToTX")) {
            simtime_t rxToTx = radioModule->par("timeRXToTX").doubleValue();
            if(rxToTx > aTurnaroundTime) {
                throw cRuntimeError(
                    "Parameter \"aTurnaroundTimeSymbols\" (%f) does not match"
                    " the radios RX to TX switching time (%f)! It"
                    " should be equal or bigger",
                    SIMTIME_DBL(aTurnaroundTime * symbolDuration), SIMTIME_DBL(rxToTx));
            }
        }

        this->mac_pib.macShortAddress = this->mac_pib.macExtendedAddress.getShortAddress();

        if(par("isPANCoordinator")) {
            this->mac_pib.macPANId = par("macPANId");
        }

        this->mac_pib.macCapReduction = par("macCapReduction");

        this->mac_pib.macAssociatedPANCoord = par("isPANCoordinator");
        this->mac_pib.macBeaconOrder = par("beaconOrder");
        this->mac_pib.macSuperframeOrder = par("superframeOrder");
        this->mac_pib.macMultiSuperframeOrder = par("multiSuperframeOrder");

        this->mac_pib.macMinBE = par("macMinBE");
        this->mac_pib.macMaxBE = par("macMaxBE");
        this->mac_pib.macMaxCSMABackoffs = par("macMaxCSMABackoffs");
        this->mac_pib.macMaxFrameRetries = par("macMaxFrameRetries");

        this->mac_pib.macDSMEGTSExpirationTime = par("macDSMEGTSExpirationTime");
        this->mac_pib.macResponseWaitTime = par("macResponseWaitTime");

        this->mac_pib.macIsPANCoord = par("isPANCoordinator");

        this->mac_pib.macIsCoord = (par("isCoordinator") || this->mac_pib.macIsPANCoord);

        this->phy_pib.phyCurrentChannel = par("commonChannel");

#if 0
        this->dsmeAdaptionLayer.setIndicationCallback(DELEGATE(&DSMELoRaPlatform::handleIndicationFromMCPS, *this));
        this->dsmeAdaptionLayer.setConfirmCallback(DELEGATE(&DSMELoRaPlatform::handleConfirmFromMCPS, *this));
#endif

        this->minBroadcastLQI = par("minBroadcastLQI");
        this->minCoordinatorLQI = par("minCoordinatorLQI");

        int gts_val = (par("gtsTx") && par("gtsRx")) ? 2 : 1;
        if (gts_val) {
            int n = getAncestorPar("n");
            int num_nodes = n - 1;
            int num_req_slots = num_nodes * gts_val;
            int MO = 3;
            while ( num_req_slots > get_num_available_slots(this->mac_pib.macCapReduction, MO)) {
                MO++; 
                if (MO > this->mac_pib.macBeaconOrder) {
                    throw cRuntimeError("Cannot allocate enough slots with current configuration");
                }
            }
            this->mac_pib.macMultiSuperframeOrder = MO;
            EV_INFO << "MO: " << MO << inet::endl;
        }

        recordScalar("requiredMultiSuperframeOrder", this->mac_pib.macMultiSuperframeOrder);
        this->dsme->initialize(this);

        this->dsme->getMessageDispatcher().setSendMultiplePacketsPerGTS(par("sendMultiplePacketsPerGTS").boolValue());

#if 0
        // static schedules need to be initialized after dsmeLayer
        if(!strcmp(schedulingSelection, "STATIC")) {
            cXMLElement *xmlFile = par("staticSchedule");
            std::vector<StaticSlot> slots = StaticSchedule::loadSchedule(xmlFile, this->mac_pib.macShortAddress);
            for(auto &slot : slots) {
                static_cast<StaticScheduling*>(scheduling)->allocateGTS(slot.superframeID, slot.slotID, slot.channelID, (Direction)slot.direction, slot.address);
            }
        }
#endif
    } else if(stage == INITSTAGE_LINK_LAYER) {
        radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
        dsme->start();
        if(!this->mac_pib.macAssociatedPANCoord) {
            LOG_DEBUG("Device is not associated with PAN.");
            startAssociation();
        }

        updateVisual();
        cModule* mobilityModule = this->getParentModule()->getParentModule()->getSubmodule("mobility");
        IMobility* mobility = dynamic_cast<IMobility*>(mobilityModule);
        if(mobility) {
            Coord currentPosition = mobility->getCurrentPosition();
            LOG_INFO("POSITION: x=" << currentPosition.x << ", y=" << currentPosition.y);
        }
    }
}

void DSMELoRaPlatform::finish() {
    recordScalar("numUpperPacketsForCAP", dsme->getMessageDispatcher().getNumUpperPacketsForCAP());
    recordScalar("numUpperPacketsForGTS", dsme->getMessageDispatcher().getNumUpperPacketsForGTS());
    recordScalar("numUpperPacketsDroppedFullQueue", dsme->getMessageDispatcher().getNumUpperPacketsDroppedFullQueue());
    recordScalar("macChannelOffset", dsme->getMAC_PIB().macChannelOffset);
    recordScalar("numUnusedTxGTS", dsme->getMessageDispatcher().getNumUnusedTxGTS());
}

void DSMELoRaPlatform::handleUpperPacket(inet::Packet* packet) {
    if (auto tag = packet->findTag<inet::PacketProtocolTag>()) {
        /* 'Smuggle' protocol information across lower layers via par() */
        auto protocol = tag->getProtocol();
        auto protocolId = inet::ProtocolGroup::ethertype.getProtocolNumber(protocol);
        packet->addPar("networkProtocol").setLongValue(protocolId);
    }

    LOG_INFO_PREFIX;
    LOG_INFO_PURE("Upper layer requests to send a message to ");

    auto message = getLoadedMessage(packet);

    auto& header = message->getHeader();
    header.setFrameType(IEEE802154eMACHeader::DATA);
    header.setSrcAddr(this->mac_pib.macExtendedAddress);

    auto destinationAddress = packet->getTag<inet::MacAddressReq>()->getDestAddress();
    if(destinationAddress.isMulticast()) {
        // handle multicast as broadcast (TODO ok?)
        destinationAddress = MacAddress::BROADCAST_ADDRESS;
        LOG_INFO_PURE("Broadcast");
    } else {
        LOG_INFO_PURE((destinationAddress.getInt() & 0xFFFF));
    }
    LOG_INFO_PURE("." << std::endl);

    translateMacAddress(destinationAddress, message->getHeader().getDestAddr());

    message->firstTry = true;
    DSME_ASSERT(message);
    DSME_ASSERT(!message->getCurrentlySending());
    message->setCurrentlySending(true);

    message->getHeader().setSrcAddr(this->mac_pib.macExtendedAddress);
    IEEE802154MacAddress& dst = message->getHeader().getDestAddr();

    if(!this->mac_pib.macAssociatedPANCoord) {
        startAssociation();
        LOG_INFO("Discarding message for " << dst.getShortAddress() << ".");
        releaseMessage(message);
        return;
    }

    LOG_DEBUG("Sending DATA message to " << dst.getShortAddress() << " via MCPS.");

    if(dst.getShortAddress() == this->mac_pib.macShortAddress) {
        /* '-> loopback */
        LOG_INFO("Loopback message received.");
        handleIndicationFromMCPS(message);
    } else {
        mcps_sap::DATA::request_parameters params;

        message->getHeader().setSrcAddrMode(SHORT_ADDRESS);
        message->getHeader().setDstAddrMode(SHORT_ADDRESS);
        message->getHeader().setDstAddr(dst);

        message->getHeader().setSrcPANId(this->mac_pib.macPANId);
        message->getHeader().setDstPANId(this->mac_pib.macPANId);

        // Both PAN IDs are equal and we are using short addresses
        // so suppress the PAN ID -> This should not be the task
        // for the user of the MCPS, but it is specified like this... TODO
        params.panIdSuppressed = true;

        params.msdu = message;
        params.msduHandle = 0; // TODO
        //params.gtsTx = !dst.isBroadcast();
        //params.gtsTx = par("gtsTx");
        auto ccnsim_tag = packet->getTag<ccnSimDSMEtags>();
        chunk = ccnsim_tag->getChunk();
        params.gtsTx = ccnsim_tag->getInfo() == CFP ? true : false;
        params.ackTx = (ccnsim_tag->getInfo() == CAP) && par("CAPAckRequest");
        EV_INFO << "gtsTx: " << (int) params.gtsTx << ", ackTx: " << (int) params.ackTx << endl;
        params.indirectTx = false;
        params.ranging = NON_RANGING;
        params.uwbPreambleSymbolRepetitions = 0;
        params.dataRate = 0; // DSSS -> 0
        params.seqNumSuppressed = false;
        params.sendMultipurpose = false;

        if(params.gtsTx) {
            uint16_t srcAddr = this->mac_pib.macShortAddress;
            if(srcAddr == 0xfffe) {
                LOG_ERROR("No short address allocated -> cannot request GTS!");
            } else if(srcAddr == 0xffff) {
                LOG_INFO("Association required before slot allocation.");
                DSME_ASSERT(false);
            }

            if(!this->dsme->getMessageDispatcher().neighborExists(dst)) {
                // TODO implement neighbor management / routing
                this->dsme->getMessageDispatcher().addNeighbor(dst);
            }

            //if(newMessage) {
                message->setStartOfFrameDelimiterSymbolCounter(this->dsme->getPlatform().getSymbolCounter()); // MISSUSE for statistics
                //message->queueAtCreation = this->dsmeAdaptionLayer.getGTSHelper().indicateIncomingMessage(dst.getShortAddress());
                //this->dsmeAdaptionLayer.getGTSHelper().checkAllocationForPacket(dst.getShortAddress());
            //}

            LOG_DEBUG("Preparing transmission in CFP.");
        } else {
            LOG_DEBUG("Preparing transmission in CAP.");
        }

        this->mcps_sap.getDATA().request(params);
    }
}

void DSMELoRaPlatform::receiveSignal(cComponent *source, simsignal_t signalID, intval_t l, cObject *details) {
    Enter_Method_Silent();
    if(signalID == IRadio::transmissionStateChangedSignal) {
        IRadio::TransmissionState newRadioTransmissionState = static_cast<IRadio::TransmissionState>(l);
        if(transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE) {
            // LOG_INFO("Transmission ready")
            txEndCallback(true); // TODO could it be false?
            scheduleAt(simTime(), new cMessage("receive"));
        }
        transmissionState = newRadioTransmissionState;
    } else if(signalID == IRadio::radioModeChangedSignal) {
        IRadio::RadioMode newRadioMode = static_cast<IRadio::RadioMode>(l);
        if(newRadioMode == IRadio::RADIO_MODE_TRANSMITTER) {
            // LOG_INFO("switched to transmit")
            if(pendingSendRequest) {
                // LOG_INFO("sendDown after tx switch")

                sendDown(pendingTxPacket);
                pendingTxPacket = nullptr; // now owned by lower layer
                pendingSendRequest = false;
            }
        } else if(newRadioMode == IRadio::RADIO_MODE_RECEIVER) {
            // LOG_INFO("switched to receive")
        }
    } else if(signalID == IRadio::receptionStateChangedSignal) {
        // LOG_INFO("receptionStateChanged to " << (uint16_t)value);
        channelInactive = false;
    }
}

/****** IDSMERadio ******/

uint8_t DSMELoRaPlatform::getChannelNumber() {
    DSME_ASSERT(currentChannel >= 11 && currentChannel <= 26);
    return currentChannel;
}

static std::string extract_type(std::string s) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, '|')) {
        out.push_back(item);
    }
    return out[3];
}

bool DSMELoRaPlatform::prepareSendingCopy(IDSMEMessage* msg, Delegate<void(bool)> txEndCallback) {
    if(msg == nullptr) {
        return false;
    }

    DSMELoRaMessage* message = check_and_cast<DSMELoRaMessage*>(msg);

    std::string printable_info = getSequenceChartInfo(msg, true);
    LOG_DEBUG(printable_info);

    LOG_INFO("sendCopyNow");

    this->txEndCallback = txEndCallback;
    auto packet = message->getSendableCopy();

    const auto& fcs = makeShared<ByteCountChunk>(B(2));
    packet->insertAtBack(fcs);

    packet->addTagIfAbsent<inet::PacketProtocolTag>()->setProtocol(&Protocol::ieee802154);
    if(!msg->getReceivedViaMCPS()) { // do not rewrite upper layer packet names
        DSME_ASSERT(strlen(packet->getName()) == 0);
        packet->setName(extract_type(printable_info).c_str());
    }

    switch(msg->getHeader().getFrameType()) {
        case IEEE802154eMACHeader::BEACON:
            emit(beaconSentDown, packet);
            break;
        case IEEE802154eMACHeader::DATA:
            if(msg->getHeader().getDestAddr().isBroadcast()) {
                emit(broadcastDataSentDown, packet);
            } else {
                emit(unicastDataSentDown, packet);
                emit(sentL2CCNFrame, chunk);
            }
            break;
        case IEEE802154eMACHeader::ACKNOWLEDGEMENT:
            emit(ackSentDown, packet);
            break;
        case IEEE802154eMACHeader::COMMAND: {
            CommandFrameIdentifier cmd = (CommandFrameIdentifier)message->packet->peekDataAsBytes()->getByte(0);
            if(cmd == CommandFrameIdentifier::DSME_GTS_REQUEST || cmd == CommandFrameIdentifier::DSME_GTS_REPLY || cmd == CommandFrameIdentifier::DSME_GTS_NOTIFY) {
                LOG_INFO("Command frame transmitted with creation time " << (long)msg->getHeader().getCreationTime() << " and dwell time " << (long)(getSymbolCounter() - msg->getHeader().getCreationTime()));
                emit(commandFrameDwellTime, (int) (getSymbolCounter() - msg->getHeader().getCreationTime()));
                DSME_ASSERT(msg->getHeader().getCreationTime() > 0);
            }
            emit(commandSentDown, packet);
            break; }
        default:
            DSME_ASSERT(false);
    }

    DSME_ASSERT(pendingTxPacket == nullptr);

    pendingTxPacket = packet;

    if(radio->getRadioMode() != IRadio::RADIO_MODE_TRANSMITTER) {
        DSME_ASSERT(msg->getHeader().getFrameType() != IEEE802154eMACHeader::ACKNOWLEDGEMENT); // switching is handled by ACK routine
        radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
    }

    return true;
}

void DSMELoRaPlatform::abortPreparedTransmission() {
    DSME_ASSERT(!pendingSendRequest);
    DSME_ASSERT(pendingTxPacket);
    delete pendingTxPacket;
    pendingTxPacket = nullptr;
    scheduleAt(simTime(), new cMessage("receive"));
}

bool DSMELoRaPlatform::sendDelayedAck(IDSMEMessage* ackMsg, IDSMEMessage* receivedMsg, Delegate<void(bool)> txEndCallback) {
    DSME_ASSERT(this->transceiverIsOn);
    DSMELoRaMessage* dsmeAckMsg = dynamic_cast<DSMELoRaMessage*>(ackMsg);
    DSME_ASSERT(dsmeAckMsg != nullptr);

    cMessage* acktimer = new cMessage("acktimer");
    acktimer->getParList().setTakeOwnership(false); // ackMsg is still owned by the AckLayer
    acktimer->getParList().addAt(0, dsmeAckMsg);

    this->txEndCallback = txEndCallback;

    // Preamble (4) | SFD (1) | PHY Hdr (1) | MAC Payload | FCS (2)
    uint32_t endOfReception = receivedMsg->getStartOfFrameDelimiterSymbolCounter() + receivedMsg->getTotalSymbols() - 2 * 4 // Preamble
                              - 2 * 1;                                                                                      // SFD
    uint32_t ackTime = endOfReception + aTurnaroundTime;
    uint32_t now = getSymbolCounter();
    uint32_t diff = ackTime - now;

    radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);

    scheduleAt(simTime() + diff * symbolDuration, acktimer);
    return true;
}

void DSMELoRaPlatform::setReceiveDelegate(receive_delegate_t receiveDelegate) {
    this->receiveFromAckLayerDelegate = receiveDelegate;
}

bool DSMELoRaPlatform::startCCA() {
    DSME_ASSERT(this->transceiverIsOn);

    this->channelInactive = true;
    scheduleAt(simTime() + 8 * this->symbolDuration, this->ccaTimer);
    return true;
}

void DSMELoRaPlatform::turnTransceiverOn() {
    if(!this->transceiverIsOn) {
	this->transceiverIsOn = true;
    	this->radio->setRadioMode(inet::physicallayer::IRadio::RADIO_MODE_RECEIVER);
    }
}

void DSMELoRaPlatform::turnTransceiverOff(){
    this->transceiverIsOn = false;
    this->radio->setRadioMode(inet::physicallayer::IRadio::RADIO_MODE_OFF);
}

/****** IDSMELoRaPlatform ******/

bool DSMELoRaPlatform::isReceptionFromAckLayerPossible() {
    return true;
}

void DSMELoRaPlatform::handleReceivedMessageFromAckLayer(IDSMEMessage* message) {
    DSME_ASSERT(receiveFromAckLayerDelegate);
    receiveFromAckLayerDelegate(message);
}

DSMELoRaMessage* DSMELoRaPlatform::getEmptyMessage() {
    messagesInUse++;
    DSME_ASSERT(messagesInUse <= MSG_POOL_SIZE); // TODO should return nullptr (and check everywhere!!)
    auto msg = new DSMELoRaMessage();
    msg->receivedViaMCPS = false;
    signalNewMsg(msg);
    return msg;
}

DSMELoRaMessage* DSMELoRaPlatform::getLoadedMessage(inet::Packet* packet) {
    messagesInUse++;
    DSME_ASSERT(messagesInUse <= MSG_POOL_SIZE); // TODO
    auto msg = new DSMELoRaMessage(packet);
    msg->receivedViaMCPS = false;
    signalNewMsg(msg);
    return msg;
}

void DSMELoRaPlatform::releaseMessage(IDSMEMessage* msg) {
    DSME_ASSERT(messagesInUse > 0);
    DSME_ASSERT(msg != nullptr);
    messagesInUse--;

#if 1
    DSMELoRaMessage* dsmeMsg = dynamic_cast<DSMELoRaMessage*>(msg);
    DSME_ASSERT(dsmeMsg != nullptr);
    msgsActive.erase(msgMap[dsmeMsg]);
#endif

    delete msg;
}

void DSMELoRaPlatform::startTimer(uint32_t symbolCounterValue) {
    SimTime time = symbolCounterValue * symbolDuration;
    if(timer->isScheduled()) {
        cancelEvent(timer);
    }
    if(simTime() <= time) {
        scheduleAt(time, timer);
    }
}

uint32_t DSMELoRaPlatform::getSymbolCounter() {
    return simTime() / symbolDuration;
}

uint16_t DSMELoRaPlatform::getRandom() {
    return intrand(UINT16_MAX);
}

void DSMELoRaPlatform::updateVisual() {
    std::stringstream s;
    s << this->mac_pib.macShortAddress;

    if(this->mac_pib.macIsCoord) {
        s << " C";
    }
    if(this->mac_pib.macAssociatedPANCoord) {
        s << " A";
    }

    cModule* host = findContainingNode(this);
    while(host->getParentModule() && host->getParentModule()->getId() != 1) {
        host = host->getParentModule();
    }
    cDisplayString& displayString = host->getDisplayString();
    displayString.setTagArg("t", 0, s.str().c_str());
}

void DSMELoRaPlatform::scheduleStartOfCFP() {
    scheduleAt(simTime(), cfpTimer);
}

uint8_t DSMELoRaPlatform::getMinCoordinatorLQI() {
    return minCoordinatorLQI;
}

void DSMELoRaPlatform::handleIndicationFromMCPS(IDSMEMessage* msg) {
    if(msg->getHeader().getDestAddr().isBroadcast() && msg->getLQI() < minBroadcastLQI) {
        releaseMessage(msg);
        return;
    }

    DSMELoRaMessage* dsmeMessage = check_and_cast<DSMELoRaMessage*>(msg);

    auto packet = dsmeMessage->decapsulatePacket();

    inet::MacAddress address;
    translateMacAddress(dsmeMessage->getHeader().getSrcAddr(), address);
    packet->addTagIfAbsent<MacAddressInd>()->setSrcAddress(address);
    packet->addTagIfAbsent<InterfaceInd>()->setInterfaceId(networkInterface->getInterfaceId());

    releaseMessage(msg);

    if (packet->hasPar("networkProtocol")) {
        /* Reattach protocol information from par() */
        auto protocolId = packet->par("networkProtocol").longValue();
        auto protocol = inet::ProtocolGroup::ethertype.getProtocol(protocolId);
        packet->addTagIfAbsent<inet::PacketProtocolTag>()->setProtocol(protocol);
        packet->addTagIfAbsent<inet::DispatchProtocolReq>()->setProtocol(protocol);
    }

    sendUp(packet);
}

void DSMELoRaPlatform::handleConfirmFromMCPS(IDSMEMessage* msg, DataStatus::Data_Status status) {
    releaseMessage(msg);
}

void DSMELoRaPlatform::signalNewMsg(DSMELoRaMessage* msg) {
#if 0
    LOG_INFO_PREFIX;
    LOG_INFO_PURE(msgId << " - ");
    for(auto i : msgsActive) {
        LOG_INFO_PURE(i << " ");
    }
    LOG_INFO_PURE(cometos::endl);
#endif

    msgMap[msg] = msgId;
    msgsActive.insert(msgId);
    msgId++;
}

std::string DSMELoRaPlatform::getDSMEManagement(uint8_t management, DSMESABSpecification& subBlock, CommandFrameIdentifier cmd) {
    std::stringstream ss;



    uint8_t numChannels = this->mac_pib.helper.getNumChannels();

    ss << " ";
    uint8_t type = management & 0x7;
    switch((ManagementType)type) {
        case DEALLOCATION:
            ss << "DEALLOCATION";
            break;
        case ALLOCATION:
            ss << "ALLOCATION";
            break;
        case DUPLICATED_ALLOCATION_NOTIFICATION:
            ss << "DUPLICATED-ALLOCATION-NOTIFICATION";
            break;
        case REDUCE:
            ss << "REDUCE";
            break;
        case RESTART:
            ss << "RESTART";
            break;
        case EXPIRATION:
            ss << "EXPIRATION";
            break;
        default:
            ss << (uint16_t)management;
    }

    if(subBlock.getSubBlock().count(true) == 1) {
        for(DSMESABSpecification::SABSubBlock::iterator it = subBlock.getSubBlock().beginSetBits(); it != subBlock.getSubBlock().endSetBits(); it++) {
            // this calculation assumes there is always exactly one superframe in the subblock
            GTS gts(subBlock.getSubBlockIndex(), (*it) / numChannels, (*it) % numChannels);

            ss << " " << gts.slotID << " " << gts.superframeID << " " << (uint16_t)gts.channel;
        }
    }

    return ss.str();
}

std::string DSMELoRaPlatform::getSequenceChartInfo(IDSMEMessage* msg, bool outgoing) {
    DSMELoRaMessage* dsmeMsg = dynamic_cast<DSMELoRaMessage*>(msg);
    DSME_ASSERT(dsmeMsg != nullptr);

    std::stringstream ss;

    IEEE802154eMACHeader& header = msg->getHeader();

    if(outgoing) {
        ss << (uint16_t)header.getDestAddr().getShortAddress() << "|";
    } else {
        ss << (uint16_t)header.getSrcAddr().getShortAddress() << "|";
    }

    ss << (uint16_t)header.hasSequenceNumber() << "|";

    ss << (uint16_t)header.getSequenceNumber() << "|";

    switch(header.getFrameType()) {
        case IEEE802154eMACHeader::BEACON:
            ss << "BEACON";
            break;
        case IEEE802154eMACHeader::DATA:
            ss << "DATA";
            break;
        case IEEE802154eMACHeader::ACKNOWLEDGEMENT:
            ss << "ACK";
            break;
        case IEEE802154eMACHeader::COMMAND: {
            //uint8_t cmd = dsmeMsg->frame->getData()[0];
            uint8_t cmd = dsmeMsg->packet->peekDataAsBytes()->getByte(0);

            switch((CommandFrameIdentifier)cmd) {
                case ASSOCIATION_REQUEST:
                    ss << "ASSOCIATION-REQUEST";
                    break;
                case ASSOCIATION_RESPONSE:
                    ss << "ASSOCIATION-RESPONSE";
                    break;
                case DISASSOCIATION_NOTIFICATION:
                    ss << "DISASSOCIATION-NOTIFICATION";
                    break;
                case DATA_REQUEST:
                    ss << "DATA-REQUEST";
                    break;
                case BEACON_REQUEST:
                    ss << "BEACON-REQUEST";
                    break;
                case DSME_ASSOCIATION_REQUEST:
                    ss << "DSME-ASSOCIATION-REQUEST";
                    break;
                case DSME_ASSOCIATION_RESPONSE:
                    ss << "DSME-ASSOCIATION-RESPONSE";
                    break;
                case DSME_BEACON_ALLOCATION_NOTIFICATION:
                    ss << "DSME-BEACON-ALLOCATION-NOTIFICATION";
                    break;
                case DSME_BEACON_COLLISION_NOTIFICATION:
                    ss << "DSME-BEACON-COLLISION-NOTIFICATION";
                    break;
                case DSME_GTS_REQUEST:
                case DSME_GTS_REPLY:
                case DSME_GTS_NOTIFY: {
                    DSMELoRaMessage* m = getLoadedMessage(dsmeMsg->getSendableCopy());
                    m->getHeader().decapsulateFrom(m);

                    MACCommand cmdd;
                    cmdd.decapsulateFrom(m);
                    GTSManagement man;
                    man.decapsulateFrom(m);

                    switch(cmdd.getCmdId()) {
                        case DSME_GTS_REQUEST: {
                            ss << "DSME-GTS-REQUEST";
                            GTSRequestCmd req;
                            req.decapsulateFrom(m);
                            //ss << getDSMEManagement(dsmeMsg->frame->getData()[1], req.getSABSpec(), cmdd.getCmdId());
                            ss << getDSMEManagement(dsmeMsg->packet->peekDataAsBytes()->getByte(1), req.getSABSpec(), cmdd.getCmdId());
                            break;
                        }
                        case DSME_GTS_REPLY: {
                            ss << "DSME-GTS-REPLY";
                            GTSReplyNotifyCmd reply;
                            reply.decapsulateFrom(m);
                            //ss << getDSMEManagement(dsmeMsg->frame->getData()[1], reply.getSABSpec(), cmdd.getCmdId());
                            ss << getDSMEManagement(dsmeMsg->packet->peekDataAsBytes()->getByte(1), reply.getSABSpec(), cmdd.getCmdId());
                            break;
                        }
                        case DSME_GTS_NOTIFY: {
                            ss << "DSME-GTS-NOTIFY";
                            GTSReplyNotifyCmd notify;
                            notify.decapsulateFrom(m);
                            //ss << getDSMEManagement(dsmeMsg->frame->getData()[1], notify.getSABSpec(), cmdd.getCmdId());
                            ss << getDSMEManagement(dsmeMsg->packet->peekDataAsBytes()->getByte(1), notify.getSABSpec(), cmdd.getCmdId());
                            break;
                        }
                        default:
                            break;
                    }

                    this->releaseMessage(m);

                    break;
                }
                default:
                    break;
            }
            break;
        }
        default:
            ss << "UNKNOWN";
            break;
    }

    ss << "|" << msg->getTotalSymbols();

    return ss.str();
}

void DSMELoRaPlatform::signalGTSChange(bool deallocation, IEEE802154MacAddress counterpart) {
    if(deallocation) slots--;
    else slots++;
    emit(gtsChange, slots);
}

void DSMELoRaPlatform::signalQueueLength(uint32_t length) {
    emit(queueLength, (int) length);
}


void DSMELoRaPlatform::signalPacketsTXPerSlot(uint32_t packets) {
    emit(packetsTXPerSlot, (int) packets);
}

void DSMELoRaPlatform::signalPacketsRXPerSlot(uint32_t packets) {
    emit(packetsRXPerSlot, (int) packets);
}

/****** OMNeT++ ******/

void DSMELoRaPlatform::handleLowerPacket(inet::Packet* packet) {
    auto frame = packet->removeAtFront<LoRaMacFrame>();
    auto fcs = packet->removeAtBack(B(2)); // FCS is not explicitly handled -> hasBitError is used instead
    if(!getTransceiverIsOn()) {
        DSMELoRaMessage* message = getLoadedMessage(packet);
        message->getHeader().decapsulateFrom(message);

        LOG_DEBUG("Missed frame " << packet->str() << "(" << getSequenceChartInfo(message, false) << ") [" << getErrorInfo(packet) << "]");

        releaseMessage(message);
        return;
    }

    if(packet->hasBitError()) {
        DSMELoRaMessage* message = getLoadedMessage(packet);
        message->getHeader().decapsulateFrom(message);

        LOG_DEBUG("Received corrupted frame " << packet->str() << "(" << getSequenceChartInfo(message, false) << ")");
        emit(corruptedFrameReceived, packet);

        releaseMessage(message);
        return;
    }

    emit(uncorruptedFrameReceived, packet);

    DSMELoRaMessage* message = getLoadedMessage(packet);
    message->getHeader().decapsulateFrom(message);

    // Get LQI
    auto errorRateInd = packet->getTag<inet::ErrorRateInd>();
    message->setLQI(PERtoLQI(errorRateInd->getPacketErrorRate()));

    LOG_DEBUG("Received valid frame     " << packet->str() << "(" << getSequenceChartInfo(message, false) << ") [" << getErrorInfo(packet) << "]");

    // Preamble (4) | SFD (1) | PHY Hdr (1) | MAC Payload | FCS (2)
    message->setStartOfFrameDelimiterSymbolCounter(getSymbolCounter() - message->getTotalSymbols() + 2 * 4 // Preamble
                                                  + 2 * 1 - (frame->getChunkLength().get() / 8));                                                // SFD

    dsme->getAckLayer().receive(message);
}

void DSMELoRaPlatform::handleSelfMessage(cMessage* msg) {
    if(msg == timer) {
        dsme->getEventDispatcher().timerInterrupt();
    } else if(msg == ccaTimer) {
        /* HACK: The LoRa Radio uses BUSY instead of IDLE */
        bool isIdle = (radio->getReceptionState() == IRadio::RECEPTION_STATE_BUSY) && channelInactive;
        LOG_DEBUG("CCA isIdle " << isIdle);
        dsme->dispatchCCAResult(isIdle);
    } else if(msg == cfpTimer) {
        dsme->handleStartOfCFP();
    } else if(strcmp(msg->getName(), "acktimer") == 0) {
        // LOG_INFO("send ACK")
        bool result = prepareSendingCopy((DSMELoRaMessage*)msg->getParList().get(0), txEndCallback);
        ASSERT(result);
        result = sendNow();
        ASSERT(result);
        // the ACK Message itself will be deleted inside the AckLayer
        delete msg;
    } else if(strcmp(msg->getName(), "receive") == 0) {
        // LOG_INFO("switch to receive")
        radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
        delete msg;
    } else {
        MacProtocolBase::handleSelfMessage(msg);
    }
}

/****** IDSMERadio ******/

bool DSMELoRaPlatform::setChannelNumber(uint8_t k) {
    DSME_ASSERT(this->transceiverIsOn);
    DSME_ASSERT(k >= 11 && k <= 26);

    //auto r = check_and_cast<LoRaRadio*>(radio);
    /* TODO */
    //r->setCenterFrequency(MHz(868 + 3 * (k-11)));
    currentChannel = k;

    /* The LoRaReceiver reads the channel from the LoRaApp aplication in Flora.
     * Therefore, it has to be set there */
    LoRaAppDummy *dummy = check_and_cast<LoRaAppDummy*>(getParentModule()->getParentModule()->getSubmodule("SimpleLoRaApp"));
    dummy->setCF(MHz(868 + 3 * (k-11)));
    return true;
}

bool DSMELoRaPlatform::sendNow() {
    DSME_ASSERT(transceiverIsOn);
    DSME_ASSERT(pendingTxPacket);
    DSME_ASSERT(!pendingSendRequest);

    auto r = check_and_cast<LoRaRadio*>(radio);
    if(this->radio->getRadioMode() == IRadio::RADIO_MODE_TRANSMITTER) {
        // can be sent direct
        auto loraTag = pendingTxPacket->addTagIfAbsent<LoRaTag>();
        loraTag->setBandwidth(Hz(125000));
        loraTag->setCenterFrequency(MHz(868 + 3 * (currentChannel - 11)));
        loraTag->setSpreadFactor(7);
        loraTag->setCodeRendundance(1);
        loraTag->setPower(W(math::dBmW2mW(14)));

        auto frame = makeShared<LoRaMacFrame>();
        /* HACK: This is required to avoid "empty chunk" errors when running
         * on debug mode
         */
        frame->setChunkLength(B(1));
        frame->setTransmitterAddress(MacAddress::BROADCAST_ADDRESS);
        frame->setLoRaTP(loraTag->getPower().get());
        frame->setLoRaCF(loraTag->getCenterFrequency());
        frame->setLoRaSF(loraTag->getSpreadFactor());
        frame->setLoRaBW(loraTag->getBandwidth());
        frame->setLoRaCR(loraTag->getCodeRendundance());
        frame->setSequenceNumber(0);
        frame->setReceiverAddress(MacAddress::BROADCAST_ADDRESS);

        //++sequenceNumber;
        //frame->setLoRaUseHeader(cInfo->getLoRaUseHeader());
        frame->setLoRaUseHeader(loraTag->getUseHeader());

        pendingTxPacket->insertAtFront(frame);

        sendDown(pendingTxPacket);
        pendingTxPacket = nullptr;
    } else {
        pendingSendRequest = true;
    }
    // otherwise receiveSignal will be called eventually
    return true;
}

}
