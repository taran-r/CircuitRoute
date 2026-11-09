import argparse
import csv
import sys
from pathlib import Path
from statistics import mean

SURFACE = "#fcfcfb"
INK = "#0b0b0b"
MUTED = "#52514e"
GRID = "#e6e5e0"
DIJKSTRA = "#2a78d6"
ASTAR = "#eb6834"
UNIFORM = "#52514e"
WEIGHTED = "#b9b8b0"

NUMBER_FIELDS = ["board_size", "max_cost", "seed", "pair_id", "manhattan_distance", "found",
                 "time_ms", "nodes_expanded", "max_queue_size", "memory_bytes", "path_cost"]
COLUMNS = ["experiment", "board_size", "max_cost", "distance_range", "seed", "pair_id",
           "manhattan_distance", "algorithm", "found", "time_ms", "nodes_expanded",
           "max_queue_size", "memory_bytes", "path_cost"]


def load_rows(filename):
    with open(filename, newline="", encoding="utf-8-sig") as file:
        reader = csv.DictReader(file)
        for column in COLUMNS:
            if column not in (reader.fieldnames or []):
                raise ValueError(f"{Path(filename).name} is missing column: {column}")
        rows = list(reader)
    if not rows:
        raise ValueError("The benchmark file is empty.")
    for row in rows:
        for field in NUMBER_FIELDS:
            row[field] = float(row[field])
    return rows


def found_rows(rows, experiment, algorithm):
    return [row for row in rows if row["experiment"] == experiment
            and row["algorithm"] == algorithm and row["found"] == 1]


def average_by(rows, experiment, algorithm, key, field):
    groups = {}
    for row in found_rows(rows, experiment, algorithm):
        groups.setdefault(row[key], []).append(row[field])
    return {name: mean(values) for name, values in groups.items()}


def ordered_ranges(rows, experiment):
    names = {row["distance_range"] for row in rows if row["experiment"] == experiment}
    return sorted(names, key=lambda name: int(name.split("-")[0]))


def short_number(value):
    if value >= 1000:
        return f"{value / 1000:.1f}k".replace(".0k", "k")
    return f"{value:.0f}"


def style_axis(axis, title, ylabel, xlabel):
    axis.set_facecolor(SURFACE)
    axis.set_title(title, fontsize=10, color=INK, loc="left")
    axis.set_ylabel(ylabel, fontsize=8, color=MUTED)
    axis.set_xlabel(xlabel, fontsize=8, color=MUTED)
    axis.tick_params(labelsize=8, colors=MUTED, length=0)
    for side in ("top", "right", "left"):
        axis.spines[side].set_visible(False)
    axis.spines["bottom"].set_color(GRID)
    axis.grid(axis="y", color=GRID, linewidth=0.8)
    axis.set_axisbelow(True)


def draw_bars(axis, labels, first, second, colors, log=False, number=None):
    positions = list(range(len(labels)))
    width = 0.36
    for offset, values, color in ((-width / 2, first, colors[0]), (width / 2, second, colors[1])):
        bars = axis.bar([p + offset for p in positions], values, width * 0.94, color=color,
                        edgecolor=SURFACE, linewidth=1.5, zorder=3)
        for bar, value in zip(bars, values):
            axis.annotate(number(value), (bar.get_x() + bar.get_width() / 2, value),
                          xytext=(0, 2), textcoords="offset points", ha="center",
                          fontsize=7, color=MUTED)
    axis.set_xticks(positions)
    axis.set_xticklabels(labels)
    if log:
        axis.set_yscale("log")
        axis.set_ylim(top=max(first + second) * 3)
    else:
        axis.set_ylim(top=max(first + second) * 1.15)


def draw_lines(axis, sizes, dijkstra, astar, log=False):
    for name, values, color in (("Dijkstra", dijkstra, DIJKSTRA), ("A*", astar, ASTAR)):
        axis.plot(sizes, values, color=color, linewidth=2, marker="o", markersize=7,
                  markeredgecolor=SURFACE, markeredgewidth=1.5, zorder=3)
        axis.annotate(name, (sizes[-1], values[-1]), xytext=(8, 0), textcoords="offset points",
                      va="center", fontsize=8, color=INK)
    axis.set_xticks(sizes)
    axis.set_xticklabels([f"{int(s)} x {int(s)}" for s in sizes])
    axis.set_xlim(sizes[0] - 20, sizes[-1] + 70)
    if log:
        axis.set_yscale("log")


def draw_charts(figure, rows):
    figure.clear()
    figure.set_facecolor(SURFACE)
    labels = ordered_ranges(rows, "distance_uniform")
    series = {}
    for algorithm in ("dijkstra", "astar"):
        for field in ("time_ms", "nodes_expanded"):
            averages = average_by(rows, "distance_uniform", algorithm, "distance_range", field)
            series[(algorithm, field)] = [averages[name] for name in labels]
    axes = figure.subplots(2, 2)
    figure.subplots_adjust(left=0.08, right=0.97, top=0.85, bottom=0.14, wspace=0.22, hspace=0.62)
    distance_label = "How far apart the wire's two ends are (cells)"
    colors = (DIJKSTRA, ASTAR)

    style_axis(axes[0][0], "1. Time per search\n(shorter bar = faster)", "milliseconds", distance_label)
    draw_bars(axes[0][0], labels, series[("dijkstra", "time_ms")], series[("astar", "time_ms")],
              colors, number=lambda v: f"{v:.2f}" if v < 10 else f"{v:.1f}")

    style_axis(axes[0][1], "2. Cells looked at per search\n(fewer = less work)", "cells", distance_label)
    draw_bars(axes[0][1], labels, series[("dijkstra", "nodes_expanded")],
              series[("astar", "nodes_expanded")], colors, number=short_number)

    ratio = {}
    for experiment in ("distance_uniform", "distance_weighted"):
        dijkstra_time = average_by(rows, experiment, "dijkstra", "distance_range", "time_ms")
        astar_time = average_by(rows, experiment, "astar", "distance_range", "time_ms")
        ratio[experiment] = [100 * astar_time[name] / dijkstra_time[name] for name in labels]
    style_axis(axes[1][0], "3. A* time compared with Dijkstra\n(below the dotted line = A* is faster)",
               "percent of Dijkstra's time", distance_label)
    draw_bars(axes[1][0], labels, ratio["distance_uniform"], ratio["distance_weighted"],
              (UNIFORM, WEIGHTED), number=lambda v: f"{v:.0f}%")
    axes[1][0].axhline(100, color=MUTED, linewidth=1, linestyle=(0, (4, 3)), zorder=2)
    axes[1][0].set_ylim(0, 130)
    axes[1][0].legend(handles=[axes[1][0].patches[0], axes[1][0].patches[len(labels)]],
                      labels=["Every cell costs 1", "Cell costs 1 to 9"], fontsize=7,
                      frameon=False, loc="upper left", ncol=2)

    sizes = sorted({row["board_size"] for row in rows if row["experiment"] == "size"})
    by_size = {}
    for algorithm in ("dijkstra", "astar"):
        averages = average_by(rows, "size", algorithm, "board_size", "time_ms")
        by_size[algorithm] = [averages[size] for size in sizes]
    style_axis(axes[1][1], "4. Bigger boards\n(lower line = faster)", "milliseconds per search",
               "Board size (long wires)")
    draw_lines(axes[1][1], sizes, by_size["dijkstra"], by_size["astar"])

    figure.suptitle("Dijkstra vs A*: same wires, same boards, 18.75% of cells blocked",
                    fontsize=12, color=INK, x=0.08, ha="left")
    figure.legend(handles=[figure.axes[0].patches[0], figure.axes[0].patches[len(labels)]],
                  labels=["Dijkstra", "A*"], loc="lower center", ncol=2, frameon=False,
                  fontsize=9, bbox_to_anchor=(0.5, 0.0))


def summary(rows):
    table = []
    for experiment in ("distance_uniform", "distance_weighted", "size"):
        key = "board_size" if experiment == "size" else "distance_range"
        groups = ordered_ranges(rows, experiment) if key == "distance_range" else \
            sorted({row["board_size"] for row in rows if row["experiment"] == experiment})
        for group in groups:
            entry = {"experiment": experiment, "group": group if key == "distance_range" else int(group)}
            for algorithm in ("dijkstra", "astar"):
                matching = [row for row in found_rows(rows, experiment, algorithm) if row[key] == group]
                entry["searches"] = len(matching)
                for field in ("time_ms", "nodes_expanded", "max_queue_size", "memory_bytes"):
                    entry[f"{algorithm}_{field}"] = mean(row[field] for row in matching)
            entry["speedup"] = entry["dijkstra_time_ms"] / entry["astar_time_ms"]
            table.append(entry)
    return table


def print_summary(table):
    print(f"{'experiment':<18}{'group':<9}{'n':>4}{'Dijkstra ms':>13}{'A* ms':>9}{'speedup':>9}"
          f"{'D cells':>10}{'A* cells':>10}{'D heap':>8}{'A* heap':>9}")
    for row in table:
        print(f"{row['experiment']:<18}{str(row['group']):<9}{row['searches']:>4}"
              f"{row['dijkstra_time_ms']:>13.2f}{row['astar_time_ms']:>9.2f}{row['speedup']:>8.1f}x"
              f"{row['dijkstra_nodes_expanded']:>10.0f}{row['astar_nodes_expanded']:>10.0f}"
              f"{row['dijkstra_max_queue_size']:>8.0f}{row['astar_max_queue_size']:>9.0f}")


def save_summary(table, filename):
    with open(filename, "w", newline="", encoding="utf-8") as file:
        writer = csv.DictWriter(file, fieldnames=list(table[0].keys()))
        writer.writeheader()
        writer.writerows(table)


def main():
    parser = argparse.ArgumentParser(description="Draw the Dijkstra versus A* performance results.")
    parser.add_argument("--data-dir", type=Path, default=Path.cwd())
    parser.add_argument("--output", type=Path)
    parser.add_argument("--show", action="store_true")
    args = parser.parse_args()

    try:
        import matplotlib
        if not args.show:
            matplotlib.use("Agg")
        from matplotlib.figure import Figure

        rows = load_rows(args.data_dir / "benchmark_results.csv")
        figure = Figure(figsize=(15, 8.5), dpi=100)
        draw_charts(figure, rows)
        table = summary(rows)
        print_summary(table)

        output = args.output or Path(__file__).resolve().parent / "output" / "performance.png"
        output.parent.mkdir(parents=True, exist_ok=True)
        figure.savefig(output, dpi=200, facecolor=SURFACE)
        save_summary(table, output.with_name(output.stem + "_summary.csv"))
        print(f"Saved: {output.resolve()}")
        if args.show:
            import matplotlib.pyplot as plt
            shown = plt.figure(figsize=(15, 8.5))
            draw_charts(shown, rows)
            plt.show()
        return 0
    except ImportError:
        print("Install Matplotlib first: python -m pip install -r visualization/requirements.txt",
              file=sys.stderr)
    except (OSError, ValueError, KeyError) as error:
        print(f"Could not draw the performance charts: {error}", file=sys.stderr)
        print("Run the benchmark from the program menu (option 8) first.", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
