# Scalability tests: ns3

This project is based on an old version of [basic-sim](https://github.com/snkas/basic-sim). The instructions provided here were adapted from the ones originally included in the project, and no longer correspond to the basic-sim ones provided by the latest versions of the basic-sim project. All the code provided in this repository was developed as a branching version of the basic-sim project, and it is no longer compatible with it. To run the experiments you should follow the instructions provided here, and use only code provided, as this repository is designed to be self-contained.

## Code structure
For simplicity, the code provided in this repository is divided in two parts that need to be merged to run the simulations. The following folders are provided:
1. *basic-sim*: Contains all code provided as a base by the basic-sim project.
2. *runs:* Includes the configuration files used to run the experiments.
3. *scatter-gather-sim:* Contains all helpers and models related to the scatter-gather experiments.
4. *scratch:* Contains the scratch file used as a entry point.
5. *internet:* Contains extra modules required to run the project.

As the project was only tested with a particular version of ns3, the right version is provided in this repository: `ns-3.30.1.zip`.

## Running the experiments
The experiments were developed and tested using ns3.30.1.

1. Extract ns3.30.1.zip: ``unzip ns-3.30.1.zip``
2. Remove the scratch folder from the ns-3.30.1 folder.
3. Move the contents to the simulator directory and remove the ns-3.30.1 folder.
4. Replace the content of `src/internet/model/ipv4-global-routing.cc / .h` with that of `src/basic-sim/model/replace-ipv4-global-routing.cc / .h`:
   ```
   cp src/basic-sim/model/replace-ipv4-global-routing.cc.txt src/internet/model/ipv4-global-routing.cc
   cp src/basic-sim/model/replace-ipv4-global-routing.h.txt src/internet/model/ipv4-global-routing.h
   ```
5. Merge the contents of the provided `internet`folder inside `simulator/src`. Two new files will be added (`tcp-cubic.h` and `tcp-cubic.c`) and one file will be replaced (`wscript`).

6. Move the contents both `basic-sim` and `scatter-gather-sim` inside `simulator/src`.

7. Configure `./waf configure`

8. Build `./waf`

9. To run a particular experiment two parameters are required: type of experiment and experiment configuration file; both will be explained in detail in the following sections. For example, to run the basic incast simulation, one needs to run the following command inside the `simulator` folder:
   ```
   ./waf --run="main --run_dir='../runs/example-incast' --sim_type='incast'"
   ```

## Type of experiments 
Here we list the different types of experiments available to run:

### One switch experiments
All the following experiments use the same topology: [one switch topology](https://github.com/capalvarez/specializing-scatter-gather/blob/master/docs/img/One-switch.png)

1. **Incast:** Simple incast experiment, the workers send the required payload as soon as they are contacted.
2. **Batching:** Static batching experiment. The workers are contacted in groups of fixed size, preventing incast in absense of background traffic.
3. **Scheduling:** Implements the rate algorithm in the simplest case: only one work piece is required (one request + N responses from the workers).
4. **Poisson:** Implements the rate algorithm, but X work pieces are required, arriving following a Poisson model.
5. **TCP background:** Implements the rate algorithm, but with a TCP flow running in the background.

### Bottleneck experiments
For different topologies, we test the following: X work pieces arrive following a Poisson model, and they can be served by one of two frontend servers, chosen at random. 

The topologies tested define two other types of experiments:

1. **1 bottleneck**: [1 bottleneck topology](https://github.com/capalvarez/specializing-scatter-gather/blob/master/docs/img/Bottleneck1.png)
2. **2 bottleneck:** [2 bottleneck topology](https://github.com/capalvarez/specializing-scatter-gather/blob/master/docs/img/Bottleneck2.png)

## Experiment configuration files
General properties of the simulation. It includes parameters related to basic-sim, and scatter-gather specific ones.

#### Basic-sim parameters
- *simulation_start_time_ns:*  When to start the simulation in simulation time (ns)
- *simulation_seed:* If there is randomness present in the simulation, this guarantees reproducibility (exactly the same outcome) if the seed is the same
- *link_data_rate_megabit_per_s:* Data rate set for all links (Mbit/s)
- *link_delay_ns:* Propagation delay set for all links (ns)

#### Scatter-gather parameters
Some of the parameters are common to all experiments, while others are specific:

##### Common parameters
See the example in `runs/example-incast/config.properties`:
- *sending_start_time_ns:* When the sender should start sending requests to the workers, in simulation time (ns).
- *min_rto_us:* Min RTO used for bother sender and worker. The standard is 200 ms, the default Linux value.
- *buffer_size_num_pkts:* Buffer size for all the switches, in number of packets.
- *payload_size*: Number of bytes that should be returned by the workers.
- *request_spacing_us:* Time in microseconds that the sender should wait after sending a request. Emulates sender-side processing time.
- *worker_processing_us:* Time in microseconds that it takes a worker to prepare a response. Emulates worker-side processing time.
- *number_workers_init, workers_step, number_workers_end:* Allow running the experiment for different number of workers.

#### Batching parameters
See an example in `runs/example-batching/config.properties`:
- *concurrent_workers:* Batch size.

#### Rate algorithm parameters
See an example in `runs/example-scheduled/config.properties`:
- *safety_factor:* Proportion of the sending time that should be left as "safety" to account for variations.
- *reference_window:* Number of microseconds we wait to measure rate and issue cancellations.
- *rate_tolerance:* Tolerance for rate measurement, how much do we allow the rate to diverge from the expected one before we consider it a relevant drop.
- *preemptive_cancel_count:* Left for testing purposes. It allows to control the number of events that should be cancelled every time a drop in rate is detected. To emulate the CPU sender behaviour it should be set as 1.

#### Poisson arrival parameters
See an example in `runs/example-poisson/config.properties`:
- *rate:* Poisson rate of arrival.
- *number_requests:* Number of requests that should arrive.
- *start_time:* When should request start arriving, in simulation time (ns).

#### TCP Background flow parameters
See an example in `runs/example-tcp-background/config.properties`:
- *background_data_rate_mbps:* Rate to send the data in the background, in Mbps.
- *background_flow_duration_ns:* How long should the flow last.

## Output
All experiments output is the same: 
- For each number of workers a text file named `worker<number>_size<payload_size>.txt` will be generated.
- Inside each text file the results are listed, one line per worker, detailing the following events from the perspective of the sender:
`<worker_index> <request_sent> <first_byte_response_received> <last_byte_response_received>`.
- At the end of the file a summary of the complete process is given:
	- *Starting time:* First request sent.
	- *Finishing time:* Last byte of last response was received.
	- *FCT:* Flow Completion Time.
- In the case of Poisson experiments, all the requests are listed following the same format described previously, separated by spaces.

## Acknowledgements
Based on code written by Hussain (master student in the NDAL group) and Simon Kassing. 