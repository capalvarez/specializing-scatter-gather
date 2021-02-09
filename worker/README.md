# Worker

3 different workers are provided:

### Incast worker:

Sends the required data as soon as the request is received.
- Configuration: See example in *config/examples/base_example.config*.
    - *test_file_name:* file used as a source of data. An example is provided in *test.txt*.
    - *max_payload:* Maximum number of bytes that can be queried. Used to fill the buffer before the queries arrive,
    preventing I/O time from interfering.
    - *cpu_index:* optional. Index of the core to pin the worker to.
    - *ip:* IP address to bind to.
    - *port:* port to bind to.
    
### Rate worker: 

Computes its local schedule and only sends at the right times. Before sending, it checks if a 
cancellation packet has been received and, if that's the case, it reschedules the event using the information included
in the cancellation packet. For details about the schedule computation refer back to the paper.
- Configuration: See example in *config/examples/rate_example.config*. This configuration builds on top on the 
incast worker one, adding extra parameters:
    - *bandwidth:* link bandwidth in Mbps.
    - *init_cwnd:* TCP initial CWND for schedule computation.
    - *MSS:* Maximum Segment Size, for schedule computation.
    - *MTU:* Maximum Transfer Unit.
    - *safety_factor:* Proportion of the sending time that should be left as "safety" to account for variations. 
    This value is static and should have the same value for all workers and associated sender so that the schedules
    match.
    - *inter_request:* Artificial processing time added to the response.
    - *chunk_size_round:* How much data should the worker send per round. 
        
### Background worker:

Mimicks the behaviour of the incast worker but ignoring the parameters sent by the sender. Instead, it sends a fixed
number of bytes as soon as the reply is received, with no rate limiting. This worker is used to simulate background 
traffic in the rate experiments.
- Configuration: See example in *config/examples/background_example.config*. This configuration file is identical to the 
incast worker one, with one extra parameter:
    - *bytes_to_send:* Number of bytes that should be sent. 

# Requirements
  - C++ 11 
  - CMake 3.7 or higher
  
The scripts were tested and developed in Ubuntu, using:
  - CMake 3.16.3
  - GCC 9.3.0
  