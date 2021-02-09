import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt


def create_size_graphs(variable_name, file_to_read, plot_name):
    fcts = pd.read_csv('cpu/{}.csv'.format(file_to_read))
    flatui = ["#3498db", "#e74c3c", "#34495e", "#2ecc71"]

    filled_markers = ('o', 'v', '^', '<', '>', '8', 's', 'p', '*', 'h', 'H', 'D', 'd', 'P', 'X')
    sns.set_style("whitegrid")
    sns.set_context(rc={'lines.markersize': 18, 'lines.linewidth': 3.8, "font.size": 20,
                        "axes.labelsize": 20})

    dash_styles = [
                   (4, 1.5),
                   (1, 1),
                   (3, 1, 1.5, 1),
                   (5, 1, 1, 1),
                   (5, 1, 2, 1, 2, 1),
                   (2, 2, 3, 1.5),
                   (1, 2.5, 3, 1.2)]

    g = sns.lineplot(x="workers", y=variable_name, dashes=dash_styles, ci=None,  markers=filled_markers,
                     hue="payload", style="payload", data=fcts, palette=sns.color_palette(flatui), legend=False)
    ax1 = g.axes
    ax1.axhline(8.98, ls='-', color="#2ecc71")

    plt.ylabel("Goodput [Gbps]")
    plt.xlabel("Workers")

    h, l = g.get_legend_handles_labels()

    box = g.get_position()
    g.set_position([box.x0, box.y0, box.width, box.height])  # resize position

    # Put a legend to the right side
    legend = g.legend(h[1:], l[1:], loc='upper center', bbox_to_anchor=(0.5, 1.15), ncol=4, fancybox=False,
                      shadow=False,
                      frameon=False)
    new_labels = ['1kB', '16kB', '64kB', '256kB']
    for t, l in zip(legend.texts, new_labels): t.set_text(l)

    sns.despine()
    plt.xlim(-0.2, None)
    plt.gcf().set_size_inches(8.3, 8)
    plt.tight_layout(pad=0.3, w_pad=0)

    plt.savefig('{}.png'.format(plot_name))


def create_cpu_graphs(file_to_read, plot_name):
    fcts = pd.read_csv('cpu/{}.csv'.format(file_to_read))

    flatui = ["#3498db", "#e74c3c" "#34495e", "#2ecc71"]

    filled_markers = ('o', 'v', '^', '<', '>', '8', 's', 'p', '*', 'h', 'H', 'D', 'd', 'P', 'X')
    sns.set_style("whitegrid")
    sns.set_context(rc={'lines.markersize': 18, 'lines.linewidth': 3.8, "font.size": 20,
                        "axes.labelsize": 20})

    dash_styles = ["",
                   (4, 1.5),
                   (1, 1),
                   (3, 1, 1.5, 1),
                   (5, 1, 1, 1),
                   (5, 1, 2, 1, 2, 1),
                   (2, 2, 3, 1.5),
                   (1, 2.5, 3, 1.2)]

    g = sns.lineplot(x="workers", y="avg", dashes=dash_styles, ci=None,  markers=filled_markers,
                     hue="payload", style="payload", data=fcts, palette=sns.color_palette(flatui), legend=False)

    plt.ylabel("CPU usage [%]")
    plt.xlabel("Workers")

    h, l = g.get_legend_handles_labels()

    box = g.get_position()
    g.set_position([box.x0, box.y0, box.width, box.height])

    # Put a legend to the right side
    legend = g.legend(h[1:], l[1:], loc='upper center', bbox_to_anchor=(0.5, 1.15), ncol=4, fancybox=False, shadow=False,
                      frameon=False)
    legend.set_title(None)

    new_labels = ['1kB', '16kB', '64kB', '256kB']
    for t, l in zip(legend.texts, new_labels): t.set_text(l)

    sns.despine()

    plt.xlim(-0.2, None)
    plt.ylim(0, None)
    plt.gcf().set_size_inches(8.3, 8)
    plt.tight_layout(pad=0.3, w_pad=0)

    plt.savefig('{}.png'.format(plot_name))

