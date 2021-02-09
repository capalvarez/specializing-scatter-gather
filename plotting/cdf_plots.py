import csv
import os
import re

import seaborn as sns
from matplotlib import pyplot as plt
import numpy as np


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


def read_packet_time(packet_file):
    with open(packet_file, 'r') as file:
        for line in file:
            if "FCT" in line or "Total" in line:
                return int(line[line.find(":")+1:].strip()) / 1000


def get_all_times(packet_files_path):
    packet_files = [f for f in os.listdir(packet_files_path) if os.path.isfile(os.path.join(packet_files_path, f))]

    fcts = {1000: [], 16000: [], 64000: [], 256000: []}

    for file in packet_files:
        if 'cpu' in file:
            continue

        info = separate_filename(file)
        fct = read_packet_time('{}/{}'.format(packet_files_path, file))

        if fct and fct > 0:
            fcts[info['size']].append(fct)

    return fcts


def create_graph(batch_directory, rate_directory, plot_name):
    sns.set_style("whitegrid")
    sns.set_context(rc={'lines.markersize': 12, 'lines.linewidth': 3.3, "font.size": 15,
                        "axes.labelsize": 18})

    filled_markers = ('o', 'P', 's', 'd', 'P', 'X')

    dash_styles = [(),
                       (4, 1.5),
                       (1, 1),
                       (3, 1, 1.5, 1),
                       (5, 1, 1, 1),
                       (5, 1, 2, 1, 2, 1),
                       (2, 2, 3, 1.5),
                       (1, 2.5, 3, 1.2)]

    fcts_batch = get_all_times(batch_directory)
    fcts_rate = get_all_times(rate_directory)

    fig, ax = plt.subplots(sharex=True, sharey=True)

    sns.kdeplot(fcts_batch[64000], cumulative=True, label="64kB")
    sns.kdeplot(fcts_rate[64000], cumulative=True, label="256kB")

    sns.kdeplot(fcts_batch[256000], cumulative=True, label="Batching")
    sns.kdeplot(fcts_rate[256000], cumulative=True, label="Rate Algorithm")

    h, l = ax.get_legend_handles_labels()

    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width, box.height])  # resize position

    # Put a legend to the right side
    legend = ax.legend(h, l, loc='upper center', bbox_to_anchor=(0.5, 1.15), ncol=3, fancybox=False, shadow=False,
                       frameon=False, numpoints=1)
    legend.set_title(None)

    ax.lines[0].set_linestyle((0, dash_styles[2]))
    ax.lines[1].set_linestyle((0, dash_styles[0]))

    ax.lines[0].set_color("#9b59b6")
    ax.lines[1].set_color("#9b59b6")

    ax.lines[0].set_linewidth(2.5)
    ax.lines[1].set_linewidth(5.5)

    ax.lines[0].set_marker(filled_markers[0])
    ax.lines[0].set_markerfacecolor("#ffffff")
    ax.lines[1].set_marker(filled_markers[0])
    ax.lines[1].set_markerfacecolor("#ffffff")
    ax.lines[0].set_markevery(0.04)
    ax.lines[1].set_markevery(0.03)

    ax.lines[2].set_linestyle((0, dash_styles[2]))
    ax.lines[3].set_linestyle((0, dash_styles[0]))

    ax.lines[2].set_color("#3498db")
    ax.lines[3].set_color("#3498db")

    ax.lines[2].set_marker(filled_markers[2])
    ax.lines[3].set_marker(filled_markers[2])
    ax.lines[2].set_markevery(0.05)
    ax.lines[3].set_markevery(0.05)

    ax.lines[2].set_linewidth(2.5)
    ax.lines[3].set_linewidth(5.5)

    array = np.append(legend.legendHandles[0].get_ydata(), 4.9)

    legend.legendHandles[0].set_marker(filled_markers[0])
    legend.legendHandles[0].set_markerfacecolor("#ffffff")
    legend.legendHandles[0].set_color("#9b59b6")
    legend.legendHandles[0].set_xdata([0.0, 14.0, 28.0])
    legend.legendHandles[0].set_ydata(array)
    legend.legendHandles[0].set_markevery([1])

    legend.legendHandles[1].set_marker(filled_markers[2])
    legend.legendHandles[1].set_color("#3498db")
    legend.legendHandles[1].set_xdata([0.0, 14.0, 28.0])
    legend.legendHandles[1].set_ydata(array)
    legend.legendHandles[1].set_markevery([1])

    legend.legendHandles[2].set_linestyle((0, dash_styles[2]))

    legend.legendHandles[2].set_color("#000000")
    legend.legendHandles[2].set_linewidth(2.5)

    legend.legendHandles[3].set_linestyle((0, dash_styles[0]))
    legend.legendHandles[3].set_color("#000000")
    legend.legendHandles[3].set_linewidth(5.5)

    plt.xlabel("Job Completion Time [ms]")
    plt.ylabel("Cumulative Fraction")

    plt.xlim(0, None)

    fig.canvas.draw()

    ax.set_xscale('log')

    sns.despine()
    plt.gcf().set_size_inches(7.68, 5.76)
    plt.savefig('{}.png'.format(plot_name))
    plt.show()


def read_csv(csv_file):
    my_dict = {64000: [], 256000: []}

    with open(csv_file, mode='r') as infile:
        reader = csv.reader(infile)
        for rows in reader:
            try:
                my_dict[int(rows[0])].append(float(rows[1]) / 1000)
            except ValueError:
                continue

    return my_dict


def fpga_cpu_graph(cpu_rate_directory, fpga_results, plot_name):
    sns.set_style("whitegrid")
    sns.set_context(rc={'lines.markersize': 12, 'lines.linewidth': 3.3, "font.size": 18,
                        "axes.labelsize": 18})

    filled_markers = ('o', 'P', 's', 'd', 'P', 'X')

    dash_styles = [(),
                       (4, 1.5),
                       (1, 1),
                       (3, 1, 1.5, 1),
                       (5, 1, 1, 1),
                       (5, 1, 2, 1, 2, 1),
                       (2, 2, 3, 1.5),
                       (1, 2.5, 3, 1.2)]

    fcts_rate = get_all_times(cpu_rate_directory)
    fcts_hw = read_csv(fpga_results)

    fcts_rate = {256000: fcts_rate[256000]}
    fcts_hw = {256000: fcts_hw[256000]}

    fig, ax = plt.subplots(sharex=True, sharey=True)

    sns.kdeplot(fcts_rate[64000], cumulative=True, label="64kB")
    sns.kdeplot(fcts_hw[64000], cumulative=True, label="256kB")

    sns.kdeplot(fcts_rate[256000], cumulative=True, label="CPU", legend=None)
    sns.kdeplot(fcts_hw[256000], cumulative=True, label="FPGA", legend=None)

    h, l = ax.get_legend_handles_labels()

    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width, box.height])  # resize position

    # Put a legend to the right side
    legend = ax.legend(h, l, loc='upper center', bbox_to_anchor=(0.5, 1.15), ncol=4, fancybox=False, shadow=False,
                       frameon=False, numpoints=1)
    legend.set_title(None)

    ax.lines[0].set_linestyle((0, dash_styles[2]))
    ax.lines[1].set_linestyle((0, dash_styles[0]))

    ax.lines[0].set_color("#3498db")
    ax.lines[1].set_color("#e74c3c")

    ax.lines[0].set_linewidth(2.5)
    ax.lines[1].set_linewidth(5.5)

    ax.lines[0].set_marker(filled_markers[0])
    ax.lines[0].set_markerfacecolor("#ffffff")

    ax.lines[1].set_marker(filled_markers[0])
    ax.lines[1].set_markerfacecolor("#ffffff")

    ax.lines[0].set_markevery(0.04)
    ax.lines[1].set_markevery(0.03)

    ax.lines[2].set_linestyle((0, dash_styles[2]))
    ax.lines[3].set_linestyle((0, dash_styles[0]))

    ax.lines[2].set_color("#3498db")
    ax.lines[3].set_color("#3498db")

    ax.lines[2].set_marker(filled_markers[2])
    ax.lines[3].set_marker(filled_markers[2])

    ax.lines[2].set_markevery(0.05)

    ax.lines[3].set_markevery(0.05)

    ax.lines[2].set_linewidth(2.5)
    ax.lines[3].set_linewidth(5.5)

    array = np.append(legend.legendHandles[0].get_ydata(), 6.3)

    legend.legendHandles[0].set_marker(filled_markers[0])
    legend.legendHandles[0].set_color("#9b59b6")
    legend.legendHandles[0].set_markerfacecolor("#ffffff")
    legend.legendHandles[0].set_xdata([0.0, 14.0, 28.0])
    legend.legendHandles[0].set_ydata(array)
    legend.legendHandles[0].set_markevery([1])

    legend.legendHandles[1].set_marker(filled_markers[2])
    legend.legendHandles[1].set_color("#3498db")
    legend.legendHandles[1].set_xdata([0.0, 14.0, 28.0])
    legend.legendHandles[1].set_ydata(array)
    legend.legendHandles[1].set_markevery([1])

    legend.legendHandles[2].set_linestyle((0, dash_styles[2]))

    legend.legendHandles[2].set_color("#000000")
    legend.legendHandles[2].set_linewidth(2.5)

    legend.legendHandles[3].set_linestyle((0, dash_styles[0]))
    legend.legendHandles[3].set_color("#000000")
    legend.legendHandles[3].set_linewidth(5.5)

    plt.xlabel("Job Completion Time [ms]")
    plt.ylabel("Cumulative Fraction")

    plt.xlim(1, None)

    sns.despine()
    plt.gcf().set_size_inches(7.68, 5.76)

    fig.canvas.draw()

    ax.set_xscale('log')
    locs, labels = plt.xticks()
    ax.set(xticklabels=[10 ** int(float(i.get_text().replace(u'\u2212', '-')))
                        for i in labels])
    plt.savefig('{}.png'.format(plot_name))
    plt.show()
