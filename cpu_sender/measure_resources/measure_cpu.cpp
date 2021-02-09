#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <cstring>
#include <iomanip>
#include <time.h>
#include <chrono>
#include <signal.h>
#include <sys/stat.h>


typedef long unsigned int num;

long number_of_processors;

/*
 * Saves the measured utilization (per core) for a certain timestamp
 */
struct cpu_measure {
    double time;
    std::vector<double> cpu_usage_per_core;

    explicit cpu_measure(double t){
        time = t;
        cpu_usage_per_core = std::vector<double>(number_of_processors);
    }

    void add_measure(int core, double measure){
        cpu_usage_per_core[core] = measure;
    }
};

struct proc_info{
    num idle;
    num non_idle;

    proc_info(){}

    proc_info(num i, num ni){
        idle = i;
        non_idle = ni;
    }
};

/*
 * Parses the stat file to obtain the relevant CPU times.
 */
std::vector<proc_info> getCPUTime(FILE* input){
    std::vector<proc_info> times(number_of_processors);

    /* Read total CPU times */
    long unsigned int cpu_time[10];

    bzero(cpu_time, sizeof(cpu_time));
    fscanf(input, "%*s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &cpu_time[0], &cpu_time[1], &cpu_time[2], &cpu_time[3],
           &cpu_time[4], &cpu_time[5], &cpu_time[6], &cpu_time[7], &cpu_time[8], &cpu_time[9]);

    for (int i = 0; i < number_of_processors; ++i) {
        bzero(cpu_time, sizeof(cpu_time));
        fscanf(input, "%*s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &cpu_time[0], &cpu_time[1], &cpu_time[2], &cpu_time[3],
               &cpu_time[4], &cpu_time[5], &cpu_time[6], &cpu_time[7], &cpu_time[8], &cpu_time[9]);


        num idle = cpu_time[3] + cpu_time[4];
        num non_idle = cpu_time[0] + cpu_time[1] + cpu_time[2] + cpu_time[5] + cpu_time[6] + cpu_time[7];

        times[i] = proc_info(idle, non_idle);
    }

    fclose(input);

    return times;
}

/*
 * Read the CPU utilization per core from the system files (/proc/stat). After reading the information, it waits
 * until the next time (user-defined).
 */
void getCPUusage(int sleepTime, std::string& cpu_file){
    std::ofstream outfile;
    outfile.open(cpu_file, std::ios::app | std::ios::out);

    chdir("/proc");

    FILE *system_info = fopen("stat", "r");

    std::vector<proc_info> old_measures = getCPUTime(system_info), new_measures;
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now(), end;

    for(;;){
        std::string use_per_core;
        system_info = fopen("stat", "r");

        new_measures = getCPUTime(system_info);

        for (int i = 0; i < number_of_processors; ++i) {
            proc_info previous_state = old_measures[i];
            proc_info current_state = new_measures[i];

            num old_total = previous_state.idle + previous_state.non_idle;
            num new_total = current_state.idle + current_state.non_idle;

            num total_diff = new_total - old_total;
            num idle_diff = current_state.idle - previous_state.idle;

            double cpu = 0;

            if(total_diff != 0){
                cpu = 100.0 * (total_diff - idle_diff)/(double)total_diff;
            }

            use_per_core += " " + std::to_string(cpu) ;
        }

        old_measures = new_measures;

        end = std::chrono::high_resolution_clock::now();
        long int duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        outfile << duration << use_per_core << std::endl;
        outfile.flush();

        usleep(sleepTime);
    }
}


/*
 * Measures CPU usage per core. The measuring process is always pinned to the last available core, to avoid interfering
 * with the processes it will measure.
 * Arguments:
 *      granularity: how often to take measurements.
 *      cpu_file: file to write the results
 */
int main(int argc, char* argv[]){
    if (argc < 3){
        perror("Not enough arguments: granularity cpu_file");
        exit(EXIT_FAILURE);
    }

    int granularity = atoi(argv[1]);
    std::string filename = argv[2];
    number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);

    cpu_set_t my_set;
    CPU_ZERO(&my_set);
    CPU_SET(number_of_processors-1, &my_set);
    sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

    try {
        getCPUusage(granularity, filename);
    }catch (const std::exception&){
        return 0;
    }

    return 0;
}