import argparse
import sys
from pathlib import Path

from plot_routes import check_same_inputs, draw_board, load_results

ALGORITHMS = [("dijkstra", "Dijkstra"), ("astar", "A*")]
GRAY = "#9aa0a6"
RED = "#d12e36"


def wire_color(connection_id):
    from matplotlib.colors import hsv_to_rgb

    return hsv_to_rgb(((connection_id * 0.61803398875) % 1, 0.75, 0.75))


class RouteView:

    def __init__(self, axis, results, name):
        self.axis = axis
        self.results = results
        self.name = name
        self.steps = []
        self.routed_so_far = [0]
        self.time_so_far = [0.0]

        draw_board(axis, results["board"])
        for result in results["metrics"]:
            connection_id = int(result["connection_id"])
            start = (int(result["source_row"]), int(result["source_col"]))
            target = (int(result["target_row"]), int(result["target_col"]))

            axis.scatter([start[1], target[1]], [start[0], target[0]],
                         color=GRAY, s=18, zorder=3)

            if result["status"] == "success":
                color = wire_color(connection_id)
                path = results["paths"][connection_id]
                line, = axis.plot([p[1] for p in path], [p[0] for p in path],
                                  color=color, linewidth=1.6, zorder=4)
                dot = axis.scatter([start[1]], [start[0]], marker="o", color=color, s=34, zorder=5)
                square = axis.scatter([target[1]], [target[0]], marker="s", color=color, s=34, zorder=5)
                drawings = [line, dot, square]
            else:
                cross = axis.scatter([start[1], target[1]], [start[0], target[0]],
                                     marker="x", color=RED, s=44, zorder=5)
                drawings = [cross]

            for drawing in drawings:
                drawing.set_visible(False)
            self.steps.append(drawings)
            self.routed_so_far.append(self.routed_so_far[-1] + (result["status"] == "success"))
            self.time_so_far.append(self.time_so_far[-1] + float(result["search_time_ms"]))

    def show_first(self, count):
        for index, drawings in enumerate(self.steps):
            for drawing in drawings:
                drawing.set_visible(index < count)
        self.axis.set_title(
            f"{self.name} | {self.routed_so_far[count]}/{count} routed | "
            f"Seed: {self.results['seed']}\n"
            f"Search time so far: {self.time_so_far[count]:.2f} ms")

    def last_wire_text(self, count):
        if count == 0:
            return f"{self.name}: no wires routed yet"
        result = self.results["metrics"][count - 1]
        text = f"{self.name}: connection {result['connection_id']} {result['status']}"
        if result["status"] == "success":
            text += f" (path cost {result['path_cost']})"
        return text


def load_available(data_dir):
    loaded = []
    for key, name in ALGORITHMS:
        if (data_dir / f"{key}_metrics.csv").exists():
            loaded.append((name, load_results(data_dir, key)))
    if not loaded:
        raise ValueError("No results found. Run Dijkstra and/or A* from the menu first.")
    if len(loaded) == 2:
        check_same_inputs(loaded[0][1], loaded[1][1])
    return loaded


def main():
    parser = argparse.ArgumentParser(description="Step through CircuitRoute results one wire at a time.")
    parser.add_argument("--data-dir", type=Path, default=Path.cwd())
    parser.add_argument("--wires", type=int, help="wires to show at the start (default: all)")
    parser.add_argument("--view", choices=["both", "dijkstra", "astar"], default="both")
    parser.add_argument("--save", type=Path, help="save a picture instead of opening a window")
    args = parser.parse_args()

    try:
        import matplotlib
        if args.save:
            matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        from matplotlib.widgets import RadioButtons, Slider

        loaded = load_available(args.data_dir)
        total = len(loaded[0][1]["metrics"])
        figure = plt.figure(figsize=(15, 9))
        axes = [figure.add_axes([0.05, 0.22, 0.4, 0.68]) for _ in loaded]
        views = [RouteView(axis, results, name) for axis, (name, results) in zip(axes, loaded)]
        info = figure.text(0.05, 0.035, "", fontsize=10)
        state = {"count": total, "view": "both"}

        def update_layout(choice):
            state["view"] = choice
            wanted = [index for index, (name, _) in enumerate(loaded)
                      if choice == "both" or name.lower().startswith(choice[0])]
            for index, axis in enumerate(axes):
                axis.set_visible(index in wanted)
            positions = {1: [[0.15, 0.22, 0.7, 0.68]],
                         2: [[0.04, 0.22, 0.44, 0.68], [0.52, 0.22, 0.44, 0.68]]}[len(wanted)]
            for index, position in zip(wanted, positions):
                axes[index].set_position(position)
            update_wires(state["count"])

        def update_wires(count):
            state["count"] = count = int(count)
            for view in views:
                view.show_first(count)
            shown = [view.last_wire_text(count) for view, axis in zip(views, axes) if axis.get_visible()]
            info.set_text("     ".join(shown))
            figure.canvas.draw_idle()

        slider_axis = figure.add_axes([0.15, 0.115, 0.5, 0.03])
        slider = Slider(slider_axis, "Wires routed", 0, total, valinit=total, valstep=1)
        slider.on_changed(update_wires)

        radio = None
        if len(loaded) == 2:
            radio_axis = figure.add_axes([0.75, 0.04, 0.2, 0.13])
            radio = RadioButtons(radio_axis, ["Both", "Dijkstra", "A*"])
            radio.on_clicked(lambda label: update_layout(
                {"Both": "both", "Dijkstra": "dijkstra", "A*": "astar"}[label]))

        def on_key(event):
            if event.key in ("left", "right"):
                step = -1 if event.key == "left" else 1
                slider.set_val(min(total, max(0, state["count"] + step)))
        figure.canvas.mpl_connect("key_press_event", on_key)

        start = total if args.wires is None else min(total, max(0, args.wires))
        choice = args.view if len(loaded) == 2 else "both"
        state["count"] = start
        update_layout(choice)
        if radio is not None:
            radio.eventson = False
            radio.set_active({"both": 0, "dijkstra": 1, "astar": 2}[choice])
            radio.eventson = True
        if start != total:
            slider.set_val(start)

        if args.save:
            args.save.parent.mkdir(parents=True, exist_ok=True)
            figure.savefig(args.save, dpi=150)
            print(f"Saved: {args.save.resolve()}")
        else:
            plt.show()
        plt.close(figure)
        return 0
    except ImportError:
        print("Install Matplotlib first: python -m pip install -r visualization/requirements.txt",
              file=sys.stderr)
    except (OSError, ValueError, KeyError) as error:
        print(f"Could not open the viewer: {error}", file=sys.stderr)
        print("Run the C++ program and export the matching files first.", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
