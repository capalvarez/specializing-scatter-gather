import os
import re
import csv
import statistics
import sys


def separate_filename(filename):
    file_info = {}

    m = re.search(
        r"workers(?P<workers>\d+)_size(?P<size>\d+)",
        filename)

    if m:
        file_info = {
            'workers': int(m.group('workers')), 'size': int(m.group('size'))
        }

    return file_info


def read_packet_times(packet_file):
    packet_times = []
    total_time = 0
    scatter_time = 0

    with open(packet_file, 'r') as file:
        for line in file:
            if "Total" in line:
                total_time = int(line[line.find(":")+1:].strip())

                packet_times = []
                continue

            pieces = line.strip().split()

            try:
                packet_time = (float(pieces[1]), float(pieces[2]))
            except (ValueError, IndexError):
                continue

            packet_times.append(packet_time)

    return total_time, scatter_time


def read_open_connection_times(packet_file):
    min_first = sys.maxsize
    max_second = 0

    with open(packet_file, 'r') as file:
        for line in file:
            if "Total" in line:
                continue

            pieces = line.strip().split()

            try:
                first = float(pieces[1])
                second = float(pieces[2])

                if first < min_first:
                    min_first = first

                if second > max_second:
                    max_second = second

            except (ValueError, IndexError):
                continue

    return max_second - min_first


def read_scatter_times(packet_file):
    first_request = sys.maxsize
    last_request = 0
    first_byte = sys.maxsize

    with open(packet_file, 'r') as file:
        for line in file:
            if "Total" in line:
                continue

            pieces = line.strip().split()

            try:
                send_req = float(pieces[3])
                get_byte = float(pieces[5])

                if send_req < first_request:
                    first_request = send_req

                if send_req > last_request:
                    last_request = send_req

                if get_byte < first_byte:
                    first_byte = get_byte

            except (ValueError, IndexError):
                continue

    return (first_byte - first_request), (last_request - first_request)


def get_fct_measures(packet_files_path):
    packet_files = [f for f in os.listdir(packet_files_path) if os.path.isfile(os.path.join(packet_files_path, f))]

    fcts = []

    for file in packet_files:
        if 'cpu' in file:
            continue

        info = separate_filename(file)
        fct, _ = read_packet_times('{}/{}'.format(packet_files_path, file))

        info['fct'] = fct

        fcts.append(info)

    return fcts


def get_open_connection(packet_files_path):
    packet_files = [f for f in os.listdir(packet_files_path) if os.path.isfile(os.path.join(packet_files_path, f))]

    open_times = []

    for file in packet_files:
        if 'cpu' in file:
            continue

        info = separate_filename(file)
        open_time = read_open_connection_times('{}/{}'.format(packet_files_path, file))
        info['open_time'] = open_time

        open_times.append(info)

    return open_times


def get_scatter_time(packet_files_path):
    packet_files = [f for f in os.listdir(packet_files_path) if os.path.isfile(os.path.join(packet_files_path, f))]

    scatter_times = []

    for file in packet_files:
        if 'cpu' in file:
            continue

        info = separate_filename(file)
        req_resp, req_req = read_scatter_times('{}/{}'.format(packet_files_path, file))

        info['request_response'] = req_resp
        info['request_request'] = req_req

        scatter_times.append(info)

    return scatter_times


def read_cpu_file(filename, n_threads):
    cpu_usage = []

    with open(filename, 'r') as file:
        for line in file:
            pieces = line.strip().split()

            total_cpu = 0

            try:
                for i in range(1, n_threads + 1):
                    total_cpu = total_cpu + float(pieces[i])
            except (ValueError, IndexError):
                continue

            if total_cpu:
                cpu_usage.append(total_cpu)

    return cpu_usage


def get_cpu_measures(packet_files_path, threads):
    packet_files = [f for f in os.listdir(packet_files_path) if os.path.isfile(os.path.join(packet_files_path, f))]

    cpu_info = []

    for file in packet_files:
        if 'cpu' not in file or 'scatter' in file:
            continue

        info = separate_filename(file)
        cpu_use = read_cpu_file('{}/{}'.format(packet_files_path, file), threads)

        if cpu_use:
            info['max_cpu'] = max(cpu_use)
            info['min_cpu'] = min(cpu_use)
            info['avg'] = statistics.mean(cpu_use)

            cpu_info.append(info)

    return cpu_info


def write_fct_csv(file_dir, results_file):
    fcts = get_fct_measures('{}'.format(file_dir))

    with open(results_file, 'w+', newline='') as f:
        fieldnames = ['workers', 'payload', 'goodput']
        writer = csv.writer(f, delimiter=',')

        writer.writerow(fieldnames)

        for fct in fcts:
            if not fct['fct']:
                continue

            writer.writerow([fct['workers'], fct['size'], (fct['workers'] * 8 * fct['size']) / (fct['fct'])])


def write_cpu_csv(file_dir, results_file):
    cpus = get_cpu_measures('{}'.format(file_dir), 4)

    with open(results_file, 'w+', newline='') as f:
        fieldnames = ['workers', 'payload', 'min_cpu', 'max_cpu', 'avg']
        writer = csv.writer(f, delimiter=',')

        writer.writerow(fieldnames)

        for cpu in cpus:
            writer.writerow([cpu['workers'], cpu['size'], cpu['min_cpu'], cpu['max_cpu'], cpu['avg']])


def write_open_csv():
    file_dir = 'results/incast-sync'

    fcts = get_open_connection('{}'.format(file_dir))

    with open('open_sync.csv', 'w+', newline='') as f:
        fieldnames = ['workers', 'payload', 'open_connections']
        writer = csv.writer(f, delimiter=',')

        writer.writerow(fieldnames)

        for fct in fcts:
            writer.writerow([fct['workers'], fct['open_time']])


def write_scatter_csv():
    file_dir = 'results/incast-cpu-4threads'

    fcts = get_scatter_time('{}'.format(file_dir))

    with open('scatter_cpu-desync.csv', 'w+', newline='') as f:
        fieldnames = ['workers', 'first_request_last_request']
        writer = csv.writer(f, delimiter=',')

        writer.writerow(fieldnames)

        for fct in fcts:
            writer.writerow([fct['workers'], fct['request_request']])


