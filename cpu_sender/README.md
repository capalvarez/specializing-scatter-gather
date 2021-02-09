# CPU Sender

## Listing the workers
The workers are specified in a configuration file (see *examples/workers_example.txt*), in the following format:
    
    <ip-address>:<port>
    <ip-address>:<port>

The order in which the workers are defined is later used to define the *worker-index*.

## Running the senders
3 different senders are provided:
- **Incast sender**: Includes desynchronization procedure as mentioned in the paper.
    - ./SenderIncast <payload-size> <number-of-threads> <path-to-workers-file>

- **Batching sender:** Batch size is given as an input parameter.
    - ./SenderBatching <payload-size> <number-of-threads> <path-to-workers-file> <batch-size>

- **Rate sender:** It follows the algorithm explained in the paper, offloading most of the schedule computation to
the workers (they must be rate workers).
    - ./SenderRate <payload-size> <number-of-threads> <path-to-workers-file> <path-to-rate-config-file> 
    - It can also be run including background traffic, defined by background workers. To use background traffic, two extra
    parameters:
        - ./SenderRate <payload-size> <number-of-threads> <path-to-workers-file> <path-to-rate-config-file> <path-to-background-workers-file> <total-background-bytes>
        - The file listing the background workers follows the same format as the normal worker file.
        
### Rate algorithm configuration
An example config file is provided in *examples/rate_config_example.txt*. The configuration file includes the following parameters, one per line:   
- *safety_factor:* Proportion of the sending time that should be left as "safety" to account for variations. 
This value is static and should have the same value for the sender and all associated workers so that the schedules
match.
- *bandwidth:* link bandwidth in Mbps.
- *init_cwnd:* TCP initial CWND for schedule computation.
- *MSS:* Maximum Segment Size, for schedule computation.
- *MTU:* Maximum Transfer Unit.
- *size_per_round:* How much data each worker should send per round. 
- *reference_window:* Number of microseconds we wait to measure rate and issue cancellations.
- *rate_tolerance:* Tolerance for rate measurement, how much do we allow the rate to diverge from the expected one 
before we consider it a relevant drop.
- *cancel_distance:* Number of events in the future we consider for the cancelling.


## Output
The results of all senders have the same format. An example of the output for 6 workers is provided in 
*examples/output_example.txt*. 

The output is printed in the standard output, with each line representing a worker:

``
<worker-index> <connection-start> <connection-established> <query-sent> <query-acked> <first-byte-reply> <last-byte-reply>
``

The last line shows the total simulation time, only considering queries and responses. The total time is computed as:
```
<total_time> = <last-byte-last-request-received> - <first-query-sent>
```

## Requirements
- C++ 11
- CMake 3.7 or higher
- libevent 2.1.11 

The system was developed and tested in Ubuntu 18.02, using GCC 9.3.0.

### Linking with Libevent
- Depending on the local configurations, libevent may not be available as a package. In that case, downloading and
compiling from source is recommended.
- To simplify compilation, the file __FindLibEvent.cmake__ is provided. In case of difficulties finding the correct 
paths, the file can be customised to the system by changing the paths used as hints (by default, it is assumed that the 
library is located in */home/user/local/lib* with the headers in */home/user/local/include*).

## Acknowledgements
- The ThreadPool implementation was adapted from [ThreadPool](https://github.com/progschj/ThreadPool). 