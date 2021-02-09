#ifndef INCAST_TIMESTAMP_H
#define INCAST_TIMESTAMP_H

namespace ns3 {


/**
 * Contains all the timestamps of the events associated to a particular worker.
 */
class IncastTimestamp {
public:
    uint32_t bytes;

    int64_t firstByte;
    int64_t lastByte;
    int64_t sentRequest;

    int worker_index;

    IncastTimestamp (){
        bytes = 0;
        firstByte = 0;
        lastByte = 0;
        sentRequest = 0;
        worker_index = 0;
    }
};

}


#endif
