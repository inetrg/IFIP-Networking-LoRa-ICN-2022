/*
 * ccnSim is a scalable chunk-level simulator for Content Centric
 * Networks (CCN), that we developed in the context of ANR Connect
 * (http://www.anr-connect.org/)
 *
 * People:
 *    Michele Tortelli (Principal suspect 1.0, mailto michele.tortelli@telecom-paristech.fr)
 *    Andrea Araldo (Principal suspect 1.1, mailto araldo@lri.fr)
 *    Dario Rossi (Occasional debugger, mailto dario.rossi@enst.fr)
 *    Emilio Leonardi (Well informed outsider, mailto emilio.leonardi@tlc.polito.it)
 *    Giuseppe Rossini (Former lead developer, mailto giuseppe.rossini@enst.fr)
 *    Raffaele Chiocchetti (Former developer, mailto raffaele.chiocchetti@gmail.com)
 *
 * Mailing list: 
 *	  ccnsim@listes.telecom-paristech.fr
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef CLIENT_IRM_H_
#define CLIENT_IRM_H_

#include <omnetpp.h>
#include <random>
#include "ccnsim.h"
#include "client.h"


using namespace std;

class client_vanilla : public client {
	public:
		void extend_sim();

    protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *);
		virtual void finish();

		virtual void request_file(unsigned long);		// For IRM clients the class_num will be always '0' by default.
        void handle_timers(cMessage *timer);

    private:
		cMessage *arrival;			// Message to trigger content requests in ED scenario.
		cMessage *arrival_ttl;		// Message to trigger content requests in ModelGraft (TTL) scenario.
		cMessage *timer;			// Message to trigger timers.

		unsigned long newCard;
		double normConstant;
		unsigned long down;
		double alphaVal;
        name_t chunk_counter{1};

		bool onlyModel = false;		// Avoids the initialization of clients requests in case of model execution.
};
#endif
