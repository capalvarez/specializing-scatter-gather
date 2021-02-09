#ifndef BATCH_INFO_H
#define BATCH_INFO_H

#include <stdint.h>

namespace ns3 {

/**
 * Information used to organize the sending of batches.
 *
*/
class BatchInfo {
public:
    double corrected_rtt;
    int64_t batch_size;
    int64_t number_batches;

    double end_previous_round;
    double transmission_time;

    int64_t to_send_round;

    BatchInfo(){}
};

}

#endif
