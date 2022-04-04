#include "base_cache.h"
#include "ccnsim_dsme_core_layer.h"
#include "content_distribution.h"
#include "ccn_interest.h"
#include "ccn_data.h"
#include "strategy_layer.h"

#include "two_lru_policy.h"
#include "two_ttl_policy.h"

// The module class needs to be registered with OMNeT++
Define_Module(ccnsim_dsme_core_layer);

void ccnsim_dsme_core_layer::initialize()
{
    core_layer::initialize();
    string rfd_protocol = par("rfd_protocol");
    timer = NULL;
    /* TODO: Add configurable option here */
    if (rfd_protocol.compare("indication") == 0) {
        timer = new cMessage("timer", TIMER_INDICATION);
    }
    else if (rfd_protocol.compare("push") == 0){
        timer = new cMessage("timer", TIMER_PUSH);
    }
    else if (rfd_protocol.compare("vanilla") != 0) {
        throw cRuntimeError("Unknown RFD protocol");
    }

    timeToNext = SimTime(par("timeToNext"));
    warmUpTime = par("warmUpTime");
    if (timer) {
        scheduleAt(simTime() + warmUpTime, timer);
    }

    int num_repos = getAncestorPar("num_repos");
    for (int i=0; i<num_repos; i++) {
		if (content_distribution::repositories[i] == getIndex())
		{
			EV << "I AM REPO\n";
			// j is counter and content name?
			int j = 0;
			for (const auto &conts : content_distribution::catalog)
			{
				// my repsository in bitmask format
                for (const auto &repos : conts.cont_repos) {
                    if (repos == getIndex())
                    {
                        // append to vector
                        my_contents.push_back(j);
                        // EV << "My contents:  " << j << "\n";
                    }
                }
				j++;
			}
            it_has_a_repo_attached = true;
        }
    }
    if (my_contents.size()) {
            for (std::size_t i = 0; i < my_contents.size(); ++i)
            {
                EV_INFO << "NODE: " << getIndex() << " my_contents[" << i << "]: " << my_contents[i] << "\n";
            }
        }
        else{
            EV_INFO<< "I GOT NO CONTENT :(\n";
        }
}

void ccnsim_dsme_core_layer::send_interest_indication(chunk_t chunk, int ogate_idx)
{

	ccn_interest *interest_indication = new ccn_interest("interest_indication", INDICATION);

	interest_indication->setChunk(chunk);
	// interest_indication->setHops(-1);
	// interest_indication->setTarget(-1);

	send(interest_indication, "face$o", ogate_idx); // 0 is client, 1 is node[0] in star. hard coded!!!
}

void ccnsim_dsme_core_layer::send_push(chunk_t chunk, int ogate_idx)
{
	ccn_data *push = new ccn_data("push", PUSH);
    push->setChunk (chunk);
    push->setHops(0);
    push->setTimestamp(simTime());
    push->setPrice(repo_price); 	// I fix in the data msg the cost of the object
                                        // that is the price of the repository

    push->setHops(1);
    push->setTarget(getIndex());
    /* TODO: What's the right value here? */
    push->setBtw(0);

    push->setCapacity(0);
    push->setTSI(1);
    push->setTSB(1);
    push->setFound(true);
	send(push, "face$o", ogate_idx); // 0 is client, 1 is node[0] in star. hard coded!!!
}

void ccnsim_dsme_core_layer::send_indication_or_push(chunk_t chunk, int type, int ogate_idx)
{
    if (type == TIMER_INDICATION) {
        send_interest_indication(chunk, ogate_idx);
    }
    else {
        send_push(chunk, ogate_idx);
    }
}

void ccnsim_dsme_core_layer::handleMessage(cMessage *in)
{
    int type = in->getKind();
    boost::unordered_map <chunk_t, pit_entry > *PIT = get_PIT();
    switch(type) {
        case TIMER_INDICATION:
        case TIMER_PUSH:

            if (it_has_a_repo_attached)
            {
                    EV_INFO
                << "Node [" << getIndex() << "] TIMER at: " << simTime() << "\n ";

                chunk_t chunk = 0;
                __sid(chunk, my_contents[count]);
                __schunk(chunk, 0);
                send_indication_or_push(chunk, type, 1);
                count++;
                if (count < my_contents.size()) {
                    string dist = par("distribution");
                    if (dist.compare("uniform") == 0) {
                        scheduleAt(simTime() + uniform(timeToNext - 2, timeToNext + 2), timer);
                    }
                    else if (dist.compare("exponential") == 0){
                        scheduleAt(simTime() + exponential(timeToNext), timer);
                    }
                    else if (dist.compare("deterministic") == 0) {
                        scheduleAt(simTime() + timeToNext, timer);
                    }
                    else {
                        throw cRuntimeError("Unknown distribution time");
                    }
                }
            }
            break;

        case INDICATION: {
            ccn_interest *incoming_msg = (ccn_interest *)in;
            EV << "node [" << getIndex() << "]: CORE INDICATION MESSAGE\n";
            EV << " int_message->getChunk(): " << incoming_msg->getChunk() << "\n";
            EV << " int_message->get_name(): " << incoming_msg->get_name() << "\n";

            //if not in CS, send upward (CS not implemented here)
            chunk_t chunk = incoming_msg->getChunk();
            EV << "node[" << getIndex() << "]: Send indication from cache\n ";
            send_interest_indication(chunk, 0); // hard coded 0 sends to client in star topology
            delete in;
            break;
         }
        case PUSH: {
            ccn_data *push = check_and_cast<ccn_data *>(in);
            chunk_t chunk = push->getChunk();
            if(!getContentStore()->lookup(chunk)) {
                EV_INFO << "PUSH: Adding chunk " << chunk << " to the Content Store" << endl;
                getContentStore()->store(push);
            }
            else {
                EV_INFO << "PUSH: chunk " << chunk << " was already in the Content Store" << endl;
            }

            delete in;
            break;
       }
        default:
            core_layer::handleMessage(in);
    }
}

int ccnsim_dsme_core_layer::i_am_src_repo(name_t name)
{
	EV << "i_am_src_repo START\n";
	for (const auto &repos : content_distribution::catalog[name].cont_repos)
	{
		if (repos == getIndex()) {
			EV << "i_am_src_repo return 1\n";
			return 1;
		}
	}
	EV << "i_am_src_repo return 0\n";
	return 0;
}

void ccnsim_dsme_core_layer::handle_interest(ccn_interest *int_msg)
{
	#ifdef SEVERE_DEBUG
		client* cli = __get_attached_client( int_msg->getArrivalGate()->getIndex() );
		if (cli && !cli->is_active() ) {
			std::stringstream msg; 
			msg<<"I am node "<< getIndex()<<" and I received an interest from interface "<<
				int_msg->getArrivalGate()->getIndex()<<". This is an error since there is "<<
				"a deactivated client attached there";
			debug_message(__FILE__, __LINE__, msg.str().c_str() );
		}
	#endif

	chunk_t chunk = int_msg->getChunk();
    double int_btw = int_msg->getBtw();

    bool cacheable = true;  // This value indicates whether the retrieved content will be cached.
    						// Usually it is always true, and it can be changed only with 2-LRU meta-caching.

    // Check if the meta-caching is 2-LRU. In this case, we need to lookup for the content ID inside the Name Cache.
    string decision_policy = ContentStore->par("DS");

    if (decision_policy.compare("two_lru")==0)		// 2-LRU
    {
    	Two_Lru* tLruPointer = dynamic_cast<Two_Lru *> (ContentStore->get_decisor());
    	if (!(tLruPointer->name_to_cache(chunk)))	// The ID is not present inside the Name Cache, so the
    												// cacheable flag inside the PIT will be set to '0'.
    			cacheable = false;
    }

    if (decision_policy.compare("two_ttl")==0)		// 2-TTL
    {
    	Two_TTL* tTTLPointer = dynamic_cast<Two_TTL *> (ContentStore->get_decisor());
    	if (!(tTTLPointer->name_to_cache(chunk)))	// The ID is not present inside the Name Cache, so the
    		// cacheable flag inside the PIT will be set to '0'.
    		cacheable = false;
    }

    //cout << "** Receiver INTEREST for content: " << int_msg->get_name() << " **" << endl;

    if (ContentStore->lookup(chunk))	// a) Lookup inside the local Content Store.
    {
    	// *** Logging HIT EVENT with timestamp
    	//

    	//cout << SIMTIME_DBL(simTime()) << "\tNODE\t" << getIndex() << "\t_HIT_\t" << __id(chunk) << endl;

    	// The received Interest is satisfied locally.
        ccn_data* data_msg = compose_data(chunk);

        data_msg->setHops(0);
        data_msg->setBtw(int_btw);
        data_msg->setTarget(getIndex());
        data_msg->setFound(true);

        data_msg->setCapacity(int_msg->getCapacity());
        data_msg->setTSI(int_msg->getHops());
        data_msg->setTSB(1);

        send_data(data_msg,"face$o", int_msg->getArrivalGate()->getIndex(), __LINE__);

        
        // *** Link Load Evaluation ***
		if(llEval && stable)
		{
			if(!(ContentStore->__check_client(int_msg->getArrivalGate()->getIndex())))
			{
				int outIndex = int_msg->getArrivalGate()->getIndex() - 1; // Because we downsized the vectors for the load evaluation
																		  // by excluding the face towards the client.
				// With percentiles
				//for(int i=1; i <= numPercentiles+1; i++)
				//{
					// Complete (put attention on the first interval)
				//	if(__id(chunk) > percID[i-1] && __id(chunk)<= percID[i])
				//	{
						// cout << "NODE # " << getIndex() << " - TX - Content # " << __id(chunk) << " from Interface N " << outIndex << " between percentiles " << percID[i-1] << " and " << percID[i] << endl;
						// Only sent DATA packets are considered (supposed having a size of 1536 Bytes)
						// With percentiles
						//numPackets[outIndex][i-1]++;
						//numBits += ((cPacket*)msg)->getBitLength();
						//numBits[outIndex][i-1] += 1536*8;
						// Without percentiles
						//numPackets[outIndex]++;
						numBits[outIndex][__id(chunk)-1] += 1536*8;

						// LOG Link Load in time
						if(getIndex() == 0)
						{
							double nextNode = getParentModule()->gate("face$o",outIndex+1)->getNextGate()->getOwnerModule()->getIndex();
							if(nextNode == 1)
							{
								//cout << SIMTIME_DBL(simTime()) << "\t LL-T1 - 1" << "\tContent # " << __id(chunk)-1 << endl;
							}
						}
						else if(getIndex() == 1)
						{
							double nextNode = getParentModule()->gate("face$o",outIndex+1)->getNextGate()->getOwnerModule()->getIndex();
							if(nextNode == 3)
							{
								//cout << SIMTIME_DBL(simTime()) << "\t LL-T2 - 1" << "\tContent # " << __id(chunk)-1 << endl;
							}
						}
						else if(getIndex() == 3)
						{
							double nextNode = getParentModule()->gate("face$o",outIndex+1)->getNextGate()->getOwnerModule()->getIndex();
							if(nextNode == 7)
							{
								//cout << SIMTIME_DBL(simTime()) << "\t LL-T3 - 1" << "\tContent # " << __id(chunk)-1 << endl;
							}
						}
						// With percentiles
						//intvlNumPackets[outIndex][i-1]++;
						//intvlNumBits += ((cPacket*)msg)->getBitLength();
						//intvlNumBits[outIndex][i-1] += 1536*8;
						//break;

						// Without percentiles
						//intvlNumPackets[outIndex]++;
						//intvlNumBits[outIndex] += 1536*8;
				//	}
				//}
			}
		}

        #ifdef SEVERE_DEBUG
        interests_satisfied_by_cache++;
		check_if_correct(__LINE__);
        #endif
    }
    else if (i_am_src_repo(int_msg->get_name()))  // b) Lookup inside the attached repo if I am supposed
    													 //	   to be the source for the requested content.
    {
    	// *** Logging MISS EVENT with timestamp
    	//cout << SIMTIME_DBL(simTime()) << "\tNODE\t" << getIndex() << "\t_MISS_\t" << __id(chunk) << endl;

    	// We are mimicking an Interest sent to the repository.
        ccn_data* data_msg = compose_data(chunk);
	
		data_msg->setPrice(repo_price); 	// I fix in the data msg the cost of the object
											// that is the price of the repository
		repo_interest++;
		repo_load++;

        data_msg->setHops(1);
        data_msg->setTarget(getIndex());
		data_msg->setBtw(std::max(my_btw,int_btw));

		data_msg->setCapacity(int_msg->getCapacity());
		data_msg->setTSI(int_msg->getHops() + 1);
		data_msg->setTSB(1);
		data_msg->setFound(true);

		// Since the PIT entry has not been created yet, the cacheability control
		// is done locally, i.e., the generated content is cached only if cacheable = true.
		// So far, this control makes sense only if a 2-LRU meta-caching is simulated.
        if (cacheable)
        	ContentStore->store(data_msg);

		send_data(data_msg,"face$o",int_msg->getArrivalGate()->getIndex(),__LINE__);

        // *** Link Load Evaluation ***
		if(llEval && stable)
		{
			if(!(ContentStore->__check_client(int_msg->getArrivalGate()->getIndex())))
			{
				int outIndex = int_msg->getArrivalGate()->getIndex() - 1; // Because we downsized the vectors for the load evaluation
																		  // by excluding the face towards the client.
				// With percentiles
				//for(int i=1; i <= numPercentiles+1; i++)
				//{
					// Complete (put attention on the first interval)
					//if(__id(chunk) > percID[i-1] && __id(chunk)<= percID[i])
					//{
						//cout << "NODE # " << getIndex() << " - TX - Content # " << __id(chunk) << " from Interface N " << outIndex << " between percentiles " << percID[i-1] << " and " << percID[i] << endl;
						// Only sent DATA packets are considered (supposed having a size of 1536 Bytes)
						// With percentiles
//						numPackets[outIndex][i-1]++;
//						//numBits += ((cPacket*)msg)->getBitLength();
//						numBits[outIndex][i-1] += 1536*8;
//
//						intvlNumPackets[outIndex][i-1]++;
//						//intvlNumBits += ((cPacket*)msg)->getBitLength();
//						intvlNumBits[outIndex][i-1] += 1536*8;
//						break;

						// Without percentiles
						//numPackets[outIndex]++;
						//numBits += ((cPacket*)msg)->getBitLength();
						numBits[outIndex][__id(chunk)-1] += 1536*8;

						// LOG Link Load in time
						if(getIndex() == 0)
						{
							double nextNode = getParentModule()->gate("face$o",outIndex+1)->getNextGate()->getOwnerModule()->getIndex();
							if(nextNode == 1)
							{
								//cout << SIMTIME_DBL(simTime()) << "\t LL-T1 - 1" << "\tContent # " << __id(chunk)-1 << endl;
							}
						}
						else if(getIndex() == 1)
						{
							double nextNode = getParentModule()->gate("face$o",outIndex+1)->getNextGate()->getOwnerModule()->getIndex();
							if(nextNode == 3)
							{
								//cout << SIMTIME_DBL(simTime()) << "\t LL-T2 - 1" << "\tContent # " << __id(chunk)-1 << endl;
							}
						}
						else if(getIndex() == 3)
						{
							double nextNode = getParentModule()->gate("face$o",outIndex+1)->getNextGate()->getOwnerModule()->getIndex();
							if(nextNode == 7)
							{
								//cout << SIMTIME_DBL(simTime()) << "\t LL-T3 - 1" << "\tContent # " << __id(chunk)-1 << endl;
							}
						}

						//intvlNumPackets[outIndex]++;
						//intvlNumBits[outIndex] += 1536*8;
					//}
				//}
			}
		}


		#ifdef SEVERE_DEBUG
		check_if_correct(__LINE__);
		#endif
	}
    else 	// c) PIT lookup.
    {
		#ifdef SEVERE_DEBUG
		unsatisfied_interests++;
		check_if_correct(__LINE__);
		#endif

		// *** Logging MISS EVENT with timestamp
		//cout << SIMTIME_DBL(simTime()) << "\tNODE\t" << getIndex() << "\t_MISS_\t" << __id(chunk) << endl;

		unordered_map < chunk_t , pit_entry >::iterator pitIt = PIT.find(chunk);

		bool i_will_forward_interest = false;

		//<aa> Insert a new PIT entry for this object, if not present. If present and invalid, reset the
		// old entry. If present and valid, do nothing </aa>
        if (	
			// There is no PIT entry for the received Interest, which, as a consequence, should be forwarded.
			pitIt==PIT.end()

			// There is a PIT entry but it is invalid (the PIT entry has been invalidated by client through a retransmission
			// because a timer expired and the object has not been found)
			|| (pitIt != PIT.end() && int_msg->getNfound() ) 

			// Too much time has been passed since the PIT entry was added
			|| simTime() - PIT[chunk].time > 2*RTT
        )
        {
			i_will_forward_interest = true;
			if (pitIt!=PIT.end())				// Invalidate and re-create a new PIT entry.
				PIT.erase(chunk);

			PIT[chunk].time = simTime();

	    	if(!cacheable)						// Set the cacheable flag inside the PIT entry.
	    		PIT[chunk].cacheable.reset();
	    	else
	    		PIT[chunk].cacheable.set();
		}

		if (int_msg->getTarget() == getIndex() )
		{	// I am the target of this interest but I have no more the object
			// Therefore, this interest cannot be aggregated with the others
			int_msg->setAggregate(false);
		}

		if ( !interest_aggregation || int_msg->getAggregate()==false )
			i_will_forward_interest = true;

		if (i_will_forward_interest)
		{  	bool * decision = strategy->get_decision(int_msg);
	    	handle_decision(decision,int_msg);
	    	delete [] decision;//free memory for the decision array
		}

		#ifdef SEVERE_DEBUG
		interface_t old_PIT_string = PIT[chunk].interfaces;
		check_if_correct(__LINE__);

		client*  c = __get_attached_client( int_msg->getArrivalGate()->getIndex() );
		if (c && !c->is_active() ){
			std::stringstream ermsg; 
			ermsg<<"Trying to add to the PIT an interface where a deactivated client is attached";
			severe_error(__FILE__,__LINE__,ermsg.str().c_str() );
		}
		#endif

		// Add the incoming interface to the PIT entry.
		add_to_pit( chunk, int_msg->getArrivalGate()->getIndex() );

		#ifdef SEVERE_DEBUG
		check_if_correct(__LINE__);
		#endif
    }
    
    #ifdef SEVERE_DEBUG
    check_if_correct(__LINE__);
    #endif

}
void ccnsim_dsme_core_layer::handle_data(ccn_data *data_msg)
{
    core_layer::handle_data(data_msg);
}

void ccnsim_dsme_core_layer::finish()
{
    core_layer::finish();
    cancelAndDelete(timer);
}
