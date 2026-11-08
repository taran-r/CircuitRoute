import argparse
import csv
import sys
from pathlib import Path


def read_csv(filename, required_columns):
    with open(filename, newline="", encoding="utf-8-sig") as file:
        reader = csv.DictReader(file)
        columns = reader.fieldnames or []
        for column in required_columns:
            if column not in columns:
                raise ValueError(f"{filename.name} is missing column: {column}")
        return list(reader)


def read_seed(filename):
    if not filename.exists():
        return "not supplied"
    with open(filename, encoding="utf-8") as file:
        version = file.readline().strip()
        settings = file.readline().split()
    if version != "CIRCUITROUTE_1" or len(settings) != 3:
        raise ValueError("The saved board metadata has an invalid format.")
    return settings[2]


def load_board(filename):
    records = read_csv(filename, ["row", "col", "blocked", "movement_cost", "cell"])
    if not records:
        raise ValueError("The board CSV is empty.")

    cells = {}
    for record in records:
        position = (int(record["row"]), int(record["col"]))
        if position in cells:
            raise ValueError("The board CSV contains a repeated position.")
        if position[0] < 0 or position[1] < 0:
            raise ValueError("Board coordinates cannot be negative.")
        cells[position] = {
            "blocked": int(record["blocked"]),
            "cost": int(record["movement_cost"]),
            "symbol": record["cell"],
        }
        if cells[position]["blocked"] not in (0, 1) or cells[position]["cost"] < 1:
            raise ValueError("Invalid obstacle flag or movement cost.")

    rows = max(position[0] for position in cells) + 1
    cols = max(position[1] for position in cells) + 1
    if rows > 1000 or cols > 1000 or len(cells) != rows * cols:
        raise ValueError("Expected a complete board, at most 1000 by 1000.")

    obstacles = []
    for row in range(rows):
        obstacle_row = []
        for col in range(cols):
            obstacle_row.append(cells[(row, col)]["blocked"])
        obstacles.append(obstacle_row)

    return {"rows": rows, "cols": cols, "cells": cells, "obstacles": obstacles}


def load_results(folder, algorithm):
    board = load_board(folder / f"{algorithm}_input_cells.csv")
    metrics = read_csv(folder / f"{algorithm}_metrics.csv", [
        "connection_id", "source_row", "source_col", "target_row", "target_col",
        "status", "search_time_ms", "path_cost",
    ])
    path_records = read_csv(folder / f"{algorithm}_paths.csv", [
        "connection_id", "step", "row", "col",
    ])

    paths = {}
    for record in path_records:
        connection_id = int(record["connection_id"])
        point = (int(record["step"]), int(record["row"]), int(record["col"]))
        paths.setdefault(connection_id, []).append(point)

    for connection_id, points in paths.items():
        points.sort()
        for step, point in enumerate(points):
            if point[0] != step:
                raise ValueError("A path has missing or repeated step numbers.")
        paths[connection_id] = [(point[1], point[2]) for point in points]

    occupied = set()
    known_ids = set()
    terminals = set()
    for result in metrics:
        for row_key, col_key in [("source_row", "source_col"), ("target_row", "target_col")]:
            terminal = (int(result[row_key]), int(result[col_key]))
            if terminal not in board["cells"] or terminal in terminals:
                raise ValueError("Connection endpoints are invalid or repeated.")
            terminals.add(terminal)

    for result in metrics:
        connection_id = int(result["connection_id"])
        if connection_id < 0 or connection_id in known_ids:
            raise ValueError("Connection IDs must be nonnegative and unique.")
        known_ids.add(connection_id)
        status = result["status"]
        if status not in ("success", "no_path", "invalid_path", "invalid_cost", "invalid_result"):
            raise ValueError(f"Unknown routing status: {status}")
        if float(result["search_time_ms"]) < 0:
            raise ValueError("Search times cannot be negative.")
        path = paths.get(connection_id, [])
        if status != "success":
            if path:
                raise ValueError("An unsuccessful connection has an exported path.")
            continue

        start = (int(result["source_row"]), int(result["source_col"]))
        target = (int(result["target_row"]), int(result["target_col"]))
        if not path or path[0] != start or path[-1] != target:
            raise ValueError("A successful path does not match its endpoints.")
        if len(set(path)) != len(path) or occupied.intersection(path):
            raise ValueError("A path repeats cells or overlaps another wire.")
        for point in path:
            if point not in board["cells"] or board["cells"][point]["blocked"]:
                raise ValueError("A path leaves the board or crosses an obstacle.")
        for previous, current in zip(path, path[1:]):
            distance = abs(current[0] - previous[0]) + abs(current[1] - previous[1])
            if distance != 1:
                raise ValueError("A path contains a diagonal move or a jump.")
        if terminals.intersection(path[1:-1]):
            raise ValueError("A path crosses another connection endpoint.")
        cost = sum(board["cells"][point]["cost"] for point in path[1:])
        if cost != int(result["path_cost"]):
            raise ValueError("An exported path cost does not match its cells.")
        occupied.update(path)

    if set(paths) - known_ids:
        raise ValueError("A path has no matching metrics record.")
    seed = read_seed(folder / f"{algorithm}_input.txt")
    return {"board": board, "metrics": metrics, "paths": paths, "seed": seed}


def draw_board(axis, board):
    from matplotlib.colors import ListedColormap

    colors = ListedColormap(["#fbfaf5", "#383e45"])
    axis.imshow(board["obstacles"], cmap=colors, vmin=0, vmax=1,
                origin="upper", interpolation="nearest")
    axis.set_xlabel("Column")
    axis.set_ylabel("Row")
    axis.set_xlim(-0.5, board["cols"] - 0.5)
    axis.set_ylim(board["rows"] - 0.5, -0.5)
    axis.set_aspect("equal")
    if board["rows"] <= 30 and board["cols"] <= 30:
        axis.set_xticks(range(board["cols"]))
        axis.set_yticks(range(board["rows"]))


def draw_endpoints(axis, points, marker, color):
    if points:
        rows = [point[0] for point in points]
        cols = [point[1] for point in points]
        axis.scatter(cols, rows, marker=marker, color=color, s=30, zorder=4)


def draw_generated_board(axis, board, seed):
    draw_board(axis, board)
    starts = []
    targets = []
    for position, cell in board["cells"].items():
        if cell["symbol"] == "S":
            starts.append(position)
        elif cell["symbol"] == "T":
            targets.append(position)
    draw_endpoints(axis, starts, "o", "#15734b")
    draw_endpoints(axis, targets, "s", "#2359a1")
    axis.set_title(f"Generated PCB | {board['rows']} x {board['cols']}\n"
                   f"{len(starts)} connection pairs | Seed: {seed}")


def draw_results(axis, results, title):
    from matplotlib.colors import hsv_to_rgb

    board = results["board"]
    draw_board(axis, board)
    successful = 0
    total_time = 0.0
    invalid = 0
    for result in results["metrics"]:
        connection_id = int(result["connection_id"])
        start = (int(result["source_row"]), int(result["source_col"]))
        target = (int(result["target_row"]), int(result["target_col"]))
        total_time += float(result["search_time_ms"])
        if result["status"].startswith("invalid"):
            invalid += 1
        if result["status"] == "success":
            successful += 1
            hue = (connection_id * 0.61803398875) % 1
            color = hsv_to_rgb((hue, 0.75, 0.75))
            path = results["paths"][connection_id]
            rows = [position[0] for position in path]
            cols = [position[1] for position in path]
            axis.plot(cols, rows, color=color, linewidth=1.4, zorder=3)
            draw_endpoints(axis, [start], "o", color)
            draw_endpoints(axis, [target], "s", color)
        else:
            draw_endpoints(axis, [start, target], "x", "#d12e36")

    attempted = len(results["metrics"])
    rate = 100 * successful / attempted if attempted else 0
    axis.set_title(f"{title} | {successful}/{attempted} routed ({rate:.1f}%) | Seed: {results['seed']}\n"
                   f"Search time: {total_time:.2f} ms | Invalid results: {invalid}")


def check_same_inputs(first, second):
    if first["board"]["cells"] != second["board"]["cells"]:
        raise ValueError("The algorithms used different input boards or costs.")
    columns = ["connection_id", "source_row", "source_col", "target_row", "target_col"]
    first_connections = [[row[column] for column in columns] for row in first["metrics"]]
    second_connections = [[row[column] for column in columns] for row in second["metrics"]]
    if first_connections != second_connections:
        raise ValueError("The algorithms used different connections or routing orders.")


def main():
    parser = argparse.ArgumentParser(description="Draw CircuitRoute boards and routing results.")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--algorithm", choices=["dijkstra", "astar"])
    mode.add_argument("--compare", action="store_true")
    parser.add_argument("--data-dir", type=Path, default=Path.cwd())
    parser.add_argument("--output", type=Path)
    parser.add_argument("--show", action="store_true")
    args = parser.parse_args()

    try:
        import matplotlib
        if not args.show:
            matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        from matplotlib.lines import Line2D
        from matplotlib.patches import Patch

        if args.compare:
            first = load_results(args.data_dir, "dijkstra")
            second = load_results(args.data_dir, "astar")
            check_same_inputs(first, second)
            figure, axes = plt.subplots(1, 2, figsize=(15, 8), layout="constrained")
            draw_results(axes[0], first, "Dijkstra")
            draw_results(axes[1], second, "A*")
            name = "routing_comparison.png"
        else:
            figure, axis = plt.subplots(figsize=(9, 9), layout="constrained")
            if args.algorithm:
                results = load_results(args.data_dir, args.algorithm)
                title = "Dijkstra" if args.algorithm == "dijkstra" else "A*"
                draw_results(axis, results, title)
                name = f"{args.algorithm}_routes.png"
            else:
                board = load_board(args.data_dir / "dataset_cells.csv")
                draw_generated_board(axis, board, read_seed(args.data_dir / "dataset.txt"))
                name = "generated_board.png"

        legend = [
            Patch(facecolor="#383e45", label="Obstacle"),
            Line2D([], [], marker="o", color="none", markerfacecolor="#15734b", label="Start"),
            Line2D([], [], marker="s", color="none", markerfacecolor="#2359a1", label="Target"),
        ]
        if args.algorithm or args.compare:
            legend.append(Line2D([], [], color="#3863ad", label="Wire: color follows connection ID"))
            legend.append(Line2D([], [], marker="x", color="#d12e36", linestyle="none",
                                 label="Unsuccessful endpoint"))
            for item in legend[1:3]:
                item.set_markerfacecolor("#555555")
        figure.legend(handles=legend, loc="outside lower center", ncol=2, frameon=False)
        output = args.output or Path(__file__).resolve().parent / "output" / name
        output.parent.mkdir(parents=True, exist_ok=True)
        figure.savefig(output, dpi=200)
        print(f"Saved: {output.resolve()}")
        if args.show:
            plt.show()
        plt.close(figure)
        return 0
    except ImportError:
        print("Install Matplotlib first: python -m pip install matplotlib", file=sys.stderr)
    except (OSError, ValueError, KeyError) as error:
        print(f"Could not draw the board: {error}", file=sys.stderr)
        print("Run the C++ program and export the matching files first.", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
