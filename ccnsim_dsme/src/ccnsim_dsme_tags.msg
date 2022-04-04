import inet.common.INETDefs;
import inet.common.TagBase;

namespace inet;

enum Direction
{
    CAP = 0;
    CFP = 1;
}

class ccnSimDSMEtags extends TagBase
{
    int info;
    int chunk;
}
