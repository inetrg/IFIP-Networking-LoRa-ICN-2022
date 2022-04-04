import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

cplusplus {{
#include "ccnsim.h"
}}

namespace inet;

class inet_ccn_data extends FieldsChunk{
	@fieldNameSuffix("_var");

//Chunk identifier (Name+Chunk Number = 64 bit)
	uint64_t chunk;

//<aa> The price of the external link this data msg passes through
	double price = 0;
//</aa>


//Target of the interest file
	int target = -1;

//Origin of the interest file
	int origin = -1;
	int hops = 0;

//Prob-Cache decision strategy
	int TSB = 0; //Time Since Birth
	int TSI = 0; //Time Since Injection
	double capacity = 0; //Path capacity

//Betweenness decision strategy
	double btw = 0;// carries the higher betweenness identified by the interest packet

	bool found = false; 
    int dest_face;
    int dest_node;
}
