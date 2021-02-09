# Specializing the network for scatter-gather workloads
In this repo, you'll find all the data to recreate the results shown in the SoCC 2020 paper "Specializing the network for scatter-gather workloads"

## Repository structure
- *cpu_sender*: All scripts representing the different CPU baselines: incast tests, static batching to solve incast, and the rate algorithm implementation.
- *worker:* All workers used for the experiments. They are common for both the FPGA and CPU experiments.
- *ns3:* Simulator used for the scalability tests.
- *plotting:* plotting scripts.
- *docs:* Documentation. 

The FPGA sender implementation can be found on its own repository.

## Authors
- Catalina Alvarez - ETH Zürich
- Zhenhao He - ETH Zürich
- Gustavo Alonso - ETH Zürich
- Ankit Singla - ETH Zürich