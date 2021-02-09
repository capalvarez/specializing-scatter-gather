import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt


def create_compare_open_graphs(file_name, plot_name):
    open = pd.read_csv(file_name)

    flatui = ["#3498db", "#e74c3c"]

    filled_markers = ('o', 'v')
    sns.set_style("whitegrid")
    sns.set_context(rc={'lines.markersize': 15, 'lines.linewidth': 3.3, "font.size": 14,
                        "axes.labelsize": 14})

    dash_styles = ["",
                   (1, 1)]

    g = sns.lineplot(x="workers", y="open_time", dashes=dash_styles, ci=None,  markers=filled_markers,
                     hue="type", style="type", data=open, palette=sns.color_palette(flatui), legend=False)

    plt.ylabel("Connection establishment time [us]")
    plt.xlabel("Workers")

    h, l = g.get_legend_handles_labels()

    box = g.get_position()
    g.set_position([box.x0, box.y0, box.width, box.height])  # resize position

    # Put a legend to the right side
    legend = g.legend(h[1:], l[1:], loc='upper center', bbox_to_anchor=(0.5, 1.15), ncol=4, fancybox=False, shadow=False,
                      frameon=False)
    legend.set_title(None)

    new_labels = ['FPGA', 'CPU']
    for t, l in zip(legend.texts, new_labels): t.set_text(l)

    sns.despine()

    plt.xlim(-0.2, None)

    plt.gcf().set_size_inches(7.68, 5.76)
    plt.savefig('{}.png'.format(plot_name))


def create_compare_completion_time(file_to_open, plot_name):
    open = pd.read_csv(file_to_open)

    flatui = ["#9b59b6", "#3498db"]

    filled_markers = ('o', 'v')
    sns.set_style("whitegrid")
    sns.set_context(rc={'lines.markersize': 15, 'lines.linewidth': 3.3, "font.size": 14,
                        "axes.labelsize": 14})

    dash_styles = ["",
                   (1, 1)]

    g = sns.lineplot(x="workers", y="fct", dashes=dash_styles, ci=None, markers=filled_markers,
                     hue="type", style="type", data=open, palette=sns.color_palette(flatui), legend="brief")

    #
    plt.ylabel("Job Completion Time [ms]")
    plt.xlabel("Workers")

    h, l = g.get_legend_handles_labels()

    box = g.get_position()
    g.set_position([box.x0, box.y0, box.width, box.height])  # resize position

    # Put a legend to the right side
    legend = g.legend(h[1:], l[1:], loc='upper center', bbox_to_anchor=(0.5, 1.15), ncol=4, fancybox=False,
                      shadow=False,
                      frameon=False)
    legend.set_title(None)

    new_labels = ['Theory', 'Ns3']
    for t, l in zip(legend.texts, new_labels): t.set_text(l)

    sns.despine()

    plt.xlim(0, None)
    plt.ylim(0, None)

    plt.gcf().subplots_adjust(left=0.15)
    plt.savefig('{}.png'.format(plot_name))


def compare_2_graphs(fpga_file, cpu_file, plot_name):
    goodputs_fpga = pd.read_csv(fpga_file)
    goodputs_cpu = pd.read_csv(cpu_file)

    flatui = ["#3498db", "#e74c3c", "#34495e", "#2ecc71"]

    filled_markers = ('o', 'P', 's', 'd', 'P', 'X')
    sns.set_style("whitegrid")
    sns.set_context(rc={'lines.markersize': 18, 'lines.linewidth': 3.5, "font.size": 20,
                        "axes.labelsize": 20})

    dash_styles = [(),
                   (4, 1.5),
                   (1, 1),
                   (3, 1, 1.5, 1),
                   (5, 1, 1, 1),
                   (5, 1, 2, 1, 2, 1),
                   (2, 2, 3, 1.5),
                   (1, 2.5, 3, 1.2)]

    fig, ax = plt.subplots(1, 2, figsize=(12, 7), sharex=True, sharey=True)

    g = sns.lineplot(x="workers", y="goodput", dashes=False, ci=None, markers=filled_markers,
                     palette=sns.color_palette(flatui), hue="payload", data=goodputs_fpga,  legend=False,
                     ax=ax[0])

    g1 = sns.lineplot(x="workers", y="goodput", dashes=False, ci=None, markers=filled_markers,
                     palette=sns.color_palette(flatui), hue="payload", data=goodputs_cpu, legend="brief",
                     ax=ax[1])

    g.lines[0].set_linestyle((0, dash_styles[0]))
    g1.lines[0].set_linestyle((0, dash_styles[0]))
    g.lines[0].set_marker(filled_markers[0])
    g1.lines[0].set_marker(filled_markers[0])

    g.lines[1].set_linestyle((0, dash_styles[1]))
    g1.lines[1].set_linestyle((0, dash_styles[1]))
    g.lines[1].set_marker(filled_markers[1])
    g1.lines[1].set_marker(filled_markers[1])

    g.lines[2].set_linestyle((0, dash_styles[2]))
    g1.lines[2].set_linestyle((0, dash_styles[2]))
    g.lines[2].set_marker(filled_markers[2])
    g1.lines[2].set_marker(filled_markers[2])

    g.lines[3].set_linestyle((0, dash_styles[3]))
    g1.lines[3].set_linestyle((0, dash_styles[3]))
    g.lines[3].set_marker(filled_markers[3])
    g1.lines[3].set_marker(filled_markers[3])

    g1.lines[5].set_linestyle((0, dash_styles[0]))
    g1.lines[5].set_marker(filled_markers[0])

    g1.lines[6].set_linestyle((0, dash_styles[1]))
    g1.lines[6].set_marker(filled_markers[1])

    g1.lines[7].set_linestyle((0, dash_styles[2]))
    g1.lines[7].set_marker(filled_markers[2])

    g1.lines[8].set_linestyle((0, dash_styles[3]))
    g1.lines[8].set_marker(filled_markers[3])

    ax[0].set_ylabel("Goodput [Gbps]")
    ax[1].set_ylabel("Goodput [Gbps]")
    ax[0].set_xlabel("Workers")
    ax[1].set_xlabel("Workers")

    ax[0].set_title("FPGA")
    ax[1].set_title("CPU")

    h, l = ax[1].get_legend_handles_labels()
    indices = [1, 2, 3, 4]

    box = ax[1].get_position()
    ax[1].set_position([box.x0, box.y0, box.width, box.height])  # resize position

    legend = ax[1].legend([h[i] for i in indices], [l[i] for i in indices], bbox_to_anchor=(0.7, 1.18),
                      ncol=4, fancybox=False,
                      shadow=False,
                      frameon=False)
    new_labels = ['1kB', '16kB', '64kB', '256kB']
    for t, l in zip(legend.texts, new_labels): t.set_text(l)

    sns.despine()
    plt.xlim(-0.2, 105)
    plt.ylim(-0.5, 9.75)

    plt.savefig('{}.png'.format(plot_name))


def compare_1_graph(file_to_read, plot_name):
    goodputs = pd.read_csv(file_to_read)

    flatui = ["#3498db", "#e74c3c", "#34495e", "#2ecc71"]

    filled_markers = ('o', 'P', 's', 'd', 'P', 'X')
    sns.set_style("whitegrid")
    sns.set_context(rc={'lines.markersize': 18, 'lines.linewidth': 3.8, "font.size": 20,
                        "axes.labelsize": 20})

    dash_styles = [(),
                   (4, 1.5),
                   (1, 1),
                   (3, 1, 1.5, 1),
                   (5, 1, 1, 1),
                   (5, 1, 2, 1, 2, 1),
                   (2, 2, 3, 1.5),
                   (1, 2.5, 3, 1.2)]

    g = sns.lineplot(x="workers", y="goodput_adjusted", dashes=False, ci=None, markers=filled_markers,
                     palette=sns.color_palette(flatui), hue="type", style="type", data=goodputs, legend=False)

    g.lines[0].set_linestyle((0, dash_styles[2]))
    g.lines[1].set_linestyle((0, dash_styles[0]))
    g.lines[0].set_marker(filled_markers[0])
    g.lines[1].set_marker(filled_markers[0])

    g.lines[2].set_linestyle((0, dash_styles[2]))
    g.lines[3].set_linestyle((0, dash_styles[0]))
    g.lines[2].set_marker(filled_markers[1])
    g.lines[3].set_marker(filled_markers[1])

    g.lines[4].set_linestyle((0, dash_styles[2]))
    g.lines[5].set_linestyle((0, dash_styles[0]))
    g.lines[4].set_marker(filled_markers[2])
    g.lines[5].set_marker(filled_markers[2])

    g.lines[6].set_linestyle((0, dash_styles[2]))
    g.lines[7].set_linestyle((0, dash_styles[0]))
    g.lines[6].set_marker(filled_markers[3])
    g.lines[7].set_marker(filled_markers[3])

    g.lines[9].set_marker(filled_markers[0])
    g.lines[10].set_marker(filled_markers[1])
    g.lines[11].set_marker(filled_markers[2])
    g.lines[12].set_marker(filled_markers[3])

    g.lines[14].set_marker(None)
    g.lines[15].set_marker(None)
    g.lines[14].set_linestyle((0, dash_styles[2]))
    g.lines[15].set_linestyle((0, dash_styles[0]))

    plt.ylabel("Goodput [Gbps]")
    plt.xlabel("Workers")

    h, l = g.get_legend_handles_labels()
    indices = [1, 2, 3, 4, 6, 7]

    box = g.get_position()
    g.set_position([box.x0, box.y0, box.width, box.height])  # resize position

    legend = g.legend([h[i] for i in indices], [l[i] for i in indices], loc="upper center", bbox_to_anchor=(0.5, 1.17),
                      ncol=3, fancybox=False,
                      shadow=False,
                      frameon=False)
    new_labels = ['1kB', '16kB', '64kB', '256kB', 'Theory', 'CPU']
    for t, l in zip(legend.texts, new_labels): t.set_text(l)

    sns.despine()
    plt.xlim(-0.2, None)
    plt.ylim(-0.5, None)
    plt.gcf().set_size_inches(8.3, 8)
    plt.tight_layout(pad=0.3, w_pad=0)

    plt.savefig('{}.png'.format(plot_name))


