import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

cplusplus {{
#include "ccnsim.h"
}}

namespace inet;

class inet_ccn_interest extends FieldsChunk{
	@fieldNameSuffix("_var");
	int path[];//for source routed path (@deprecated)

	uint64_t chunk; //Actual downloading chunk (name+chunk number=64 bit)
	int hops = 0; //Hop counter


	int target = -1; //Generic target of the interest (can be a repository or a generic node)
	int rep_target = -1; //Repository target (it MUST be a repository)

	double btw = 0; //Maximum betweenness centrality along the traveled path (used by the btw DS).
	int TTL = 10000; //Maximum number of hops (after the packet is discarded)

	bool nfound = false;	//Set by a client once the timer for a given object is expired, and used
							// by the core in order to invalidate PIT's entries)

	int capacity = 0;
	int origin = -1; //Origin of the interest (rarely used)
	double Delay = 0; //Delay used by nodes for delay-sending the given packet (useful for simulate any sort of delays)

	//<aa>
	int serialNumber; //Used for debug purposes. It identifies each single interest issued by the client

	bool aggregate = true;	// When true, the interest will not be forwarded by a node, if a PIT entry
									// is already present (we say that the interest is aggregated to the
									// previous one). When an interest is involved in a loop, it will
									// disappear. In this case, to avoid this, set aggregate to false
	//</aa>
    int dest_face;
    int dest_node;
}


