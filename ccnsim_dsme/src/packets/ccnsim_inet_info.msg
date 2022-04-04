import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

cplusplus {{
#include "ccnsim.h"
}}

namespace inet;

class ccnsim_inet_info extends FieldsChunk{
    int type;
    int num;
}
