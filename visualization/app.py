import os
import random
import re
import shutil
import subprocess
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

import matplotlib

matplotlib.use("TkAgg")
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure

from interactive_viewer import RouteView, load_available
from plot_performance import draw_charts, load_rows, summary
from plot_routes import draw_generated_board, load_board, read_seed

PROJECT = Path(__file__).resolve().parent.parent
BUILD_DIR = Path(__file__).resolve().parent / "output"
RUN_DIR = BUILD_DIR / "run"
SOURCES = ["src/main.cpp", "src/pcb/board.cpp",
           "src/algorithms/dijkstra.cpp", "src/algorithms/astar.cpp", "src/benchmark/benchmark.cpp"]
ALGORITHMS = {"dijkstra": "Dijkstra", "astar": "A*"}
MENU_GENERATE, MENU_SAVE, MENU_LOAD = 1, 3, 4
MENU_DIJKSTRA, MENU_ASTAR, MENU_DEMO, MENU_BENCHMARK, MENU_EXIT = 5, 6, 7, 8, 0

FIELDS = [
    ("Rows", "rows", 25, 1, 1000, "How tall the board is"),
    ("Columns", "cols", 40, 1, 1000, "How wide the board is"),
    ("Obstacles", "obstacles", 150, 0, 1000000, "Blocked squares that wires cannot cross"),
    ("Connections", "connections", 8, 0, 500000, "Pairs of points to join with a wire (project: 20-50)"),
    ("Seed", "seed", 7, 0, 2000000000, "Same seed = same board every time"),
    ("Max cell cost", "cost", 1, 1, 1000, "1 = every square costs the same; higher = some squares cost more"),
]

EXPLAINER = (
    "How to read these charts: blue is Dijkstra and orange is A*, both solving the same wires on the same boards.\n"
    "1. Time per search: how long finding one wire takes. Shorter is better.\n"
    "2. Cells looked at: how many squares each one checked before finding the wire. Fewer means less work.\n"
    "3. A* time compared with Dijkstra: 100% means equal speed, lower means A* is faster. "
    "Dark bars are boards where every cell costs the same, light bars are boards where some cells cost more.\n"
    "4. Bigger boards: whether the gap holds as the board grows."
)


PRESETS = {
    "Small demo": [25, 40, 150, 8, 7, 1],
    "Project size (400 x 400)": [400, 400, 30000, 35, 42, 1],
}


def read_connections(filename):
    with open(filename, encoding="utf-8") as file:
        file.readline()
        rows, cols, _ = file.readline().split()
        for _ in range(int(rows) * int(cols)):
            file.readline()
        count = int(file.readline())
        return [tuple(int(x) for x in file.readline().split()) for _ in range(count)]


class App:
    def __init__(self, root):
        self.root = root
        root.title("CircuitRoute")
        root.geometry(f"{min(1500, root.winfo_screenwidth() - 80)}x{min(950, root.winfo_screenheight() - 120)}+20+20")
        self.values = {name: tk.StringVar(value=str(default))
                       for _, name, default, _, _, _ in FIELDS}
        self.view_mode = tk.StringVar(value="both")
        self.status = tk.StringVar(value="Pick a board size and press \"Generate board\".")
        self.summary = tk.StringVar(value="")
        self.wire_label = tk.StringVar(value="")
        self.performance_note = tk.StringVar(
            value="Press \"Performance benchmark\" to compare the two algorithms on many boards.\n\n" + EXPLAINER)
        self.views = []
        self.axes = []
        self.loaded = []
        self.connections = []
        self.board_ready = False
        self.total = 0
        self.count = 0
        self.playing = False
        self.updating = False
        self.action_buttons = []
        self.build_form()
        self.build_tabs()
        self.build_controls()
        root.bind("<Left>", lambda event: self.arrow_key(event, -1))
        root.bind("<Right>", lambda event: self.arrow_key(event, 1))

    def build_form(self):
        style = ttk.Style()
        style.configure("TButton", padding=(8, 4))
        style.configure("Heading.TLabel", font=("TkDefaultFont", 10, "bold"))
        outer = ttk.Frame(self.root)
        outer.pack(side="left", fill="y")
        scroller = tk.Canvas(outer, highlightthickness=0)
        scrollbar = ttk.Scrollbar(outer, orient="vertical", command=scroller.yview)
        side = ttk.Frame(scroller, padding=(14, 12))
        side.bind("<Configure>", lambda event: scroller.configure(scrollregion=scroller.bbox("all")))
        scroller.create_window((0, 0), window=side, anchor="nw")
        scroller.configure(yscrollcommand=scrollbar.set)
        scroller.pack(side="left", fill="y")
        scrollbar.pack(side="right", fill="y")
        scroller.bind("<Enter>", lambda event: scroller.bind_all(
            "<MouseWheel>", lambda e: scroller.yview_scroll(int(-e.delta / 120), "units")))
        scroller.bind("<Leave>", lambda event: scroller.unbind_all("<MouseWheel>"))

        ttk.Label(side, text="1. Describe the board", style="Heading.TLabel").pack(anchor="w", pady=(0, 6))
        grid = ttk.Frame(side)
        grid.pack(fill="x")
        for row, (label, name, _, _, _, hint) in enumerate(FIELDS):
            ttk.Label(grid, text=label).grid(row=row, column=0, sticky="w", pady=4, padx=(0, 10))
            entry = ttk.Entry(grid, textvariable=self.values[name], width=10)
            entry.grid(row=row, column=1, sticky="w", pady=4)
            entry.bind("<FocusIn>", lambda event, text=hint: self.status.set(text))
            if name == "seed":
                ttk.Button(grid, text="Random", command=self.random_seed).grid(row=row, column=2, padx=(6, 0))

        ttk.Separator(side).pack(fill="x", pady=12)
        ttk.Label(side, text="Board", style="Heading.TLabel").pack(anchor="w", pady=(0, 4))
        for name, numbers in PRESETS.items():
            ttk.Button(side, text=name,
                       command=lambda n=numbers: self.use_preset(n)).pack(fill="x", pady=2)
        files = ttk.Frame(side)
        files.pack(fill="x", pady=2)
        self.add_button(files, "Save board...", self.save_board, expand=True)
        self.add_button(files, "Load board...", self.load_saved_board, expand=True)

        ttk.Separator(side).pack(fill="x", pady=12)
        ttk.Label(side, text="Route", style="Heading.TLabel").pack(anchor="w", pady=(0, 4))
        self.add_button(side, "Generate board", self.generate_board, fill=True)
        routes = ttk.Frame(side)
        routes.pack(fill="x", pady=2)
        self.add_button(routes, "Run Dijkstra", lambda: self.run_algorithms(["dijkstra"]), expand=True)
        self.add_button(routes, "Run A*", lambda: self.run_algorithms(["astar"]), expand=True)
        self.add_button(side, "Run both", lambda: self.run_algorithms(["dijkstra", "astar"]), fill=True)
        self.add_button(side, "Generate and route both", self.generate_and_route, fill=True)

        ttk.Separator(side).pack(fill="x", pady=12)
        ttk.Label(side, text="More", style="Heading.TLabel").pack(anchor="w", pady=(0, 4))
        self.add_button(side, "Collision demo", self.collision_demo, fill=True)
        self.add_button(side, "Performance benchmark", self.run_benchmark, fill=True)
        self.add_button(side, "Save picture...", self.save_picture, fill=True)

        ttk.Label(side, textvariable=self.status, foreground="#1a5fb4", wraplength=260,
                  justify="left").pack(anchor="w", pady=(14, 0))
        self.root.update_idletasks()
        scroller.configure(width=side.winfo_reqwidth())

    def add_button(self, parent, text, command, expand=False, fill=False):
        button = ttk.Button(parent, text=text, command=command)
        if expand:
            button.pack(side="left", fill="x", expand=True, padx=2)
        else:
            button.pack(fill="x" if fill else "none", pady=2)
        self.action_buttons.append(button)

    def build_tabs(self):
        self.right = ttk.Frame(self.root)
        self.right.pack(side="left", fill="both", expand=True)
        self.tabs = ttk.Notebook(self.right)
        self.tabs.pack(fill="both", expand=True, padx=(0, 12), pady=(12, 6))

        board_tab = ttk.Frame(self.tabs)
        self.tabs.add(board_tab, text="Board")
        self.figure = Figure(figsize=(12, 6.5), dpi=100)
        self.canvas = FigureCanvasTkAgg(self.figure, master=board_tab)
        self.canvas.get_tk_widget().pack(fill="both", expand=True)

        table_tab = ttk.Frame(self.tabs)
        self.tabs.add(table_tab, text="Connections")
        columns = ("id", "start", "target", "dijkstra", "astar")
        self.table = ttk.Treeview(table_tab, columns=columns, show="headings")
        for name, text, width in [("id", "ID", 60), ("start", "Start (row, col)", 140),
                                  ("target", "Target (row, col)", 140),
                                  ("dijkstra", "Dijkstra", 220), ("astar", "A*", 220)]:
            self.table.heading(name, text=text)
            self.table.column(name, width=width, anchor="w")
        scroll = ttk.Scrollbar(table_tab, orient="vertical", command=self.table.yview)
        self.table.configure(yscrollcommand=scroll.set)
        self.table.pack(side="left", fill="both", expand=True)
        scroll.pack(side="right", fill="y")

        performance_tab = ttk.Frame(self.tabs)
        self.tabs.add(performance_tab, text="Performance")
        self.performance_figure = Figure(figsize=(13, 6.2), dpi=100)
        self.performance_canvas = FigureCanvasTkAgg(self.performance_figure, master=performance_tab)
        ttk.Label(performance_tab, textvariable=self.performance_note, wraplength=1300,
                  justify="left").pack(side="bottom", anchor="w", padx=12, pady=10)
        self.performance_canvas.get_tk_widget().pack(fill="both", expand=True)

    def build_controls(self):
        controls = ttk.LabelFrame(self.right, text="2. Watch the routing", padding=(14, 10))
        controls.pack(fill="x", padx=(0, 12), pady=(6, 12))

        top = ttk.Frame(controls)
        top.pack(fill="x")
        ttk.Label(top, text="Show:").pack(side="left", padx=(0, 6))
        for text, value in [("Both", "both"), ("Dijkstra", "dijkstra"), ("A*", "astar")]:
            ttk.Radiobutton(top, text=text, value=value, variable=self.view_mode,
                            command=self.apply_view).pack(side="left", padx=4)
        ttk.Button(top, text="< Back", command=lambda: self.set_count(self.count - 1)).pack(
            side="left", padx=(30, 2))
        self.play_button = ttk.Button(top, text="Play", command=self.toggle_play)
        self.play_button.pack(side="left", padx=2)
        ttk.Button(top, text="Next >", command=lambda: self.set_count(self.count + 1)).pack(
            side="left", padx=2)
        ttk.Label(top, textvariable=self.wire_label).pack(side="left", padx=20)

        self.scale = tk.Scale(controls, from_=0, to=0, orient="horizontal", showvalue=False,
                              resolution=1,
                              command=lambda value: self.set_count(int(float(value))))
        self.scale.pack(fill="x", pady=(6, 2))
        ttk.Label(controls, textvariable=self.summary, justify="left").pack(anchor="w")

    def arrow_key(self, event, step):
        if not isinstance(event.widget, (ttk.Entry, ttk.Treeview)):
            self.set_count(self.count + step)

    def use_preset(self, numbers):
        for (_, name, *_), number in zip(FIELDS, numbers):
            self.values[name].set(str(number))

    def random_seed(self):
        self.values["seed"].set(str(random.randint(0, 99999)))

    def read_form(self):
        numbers = {}
        for label, name, _, low, high, _ in FIELDS:
            text = self.values[name].get().strip()
            try:
                number = int(text)
            except ValueError:
                raise ValueError(f"{label} must be a whole number (you typed \"{text}\").")
            if not low <= number <= high:
                raise ValueError(f"{label} must be between {low} and {high}.")
            numbers[name] = number
        if numbers["obstacles"] + 2 * numbers["connections"] > numbers["rows"] * numbers["cols"]:
            raise ValueError("That board is too crowded: obstacles plus two points per "
                             "connection must fit in rows x columns.")
        return numbers

    def ensure_program(self):
        program = BUILD_DIR / ("circuitroute.exe" if os.name == "nt" else "circuitroute")
        sources = [PROJECT / path for path in SOURCES] + list((PROJECT / "src").rglob("*.h"))
        if program.exists() and program.stat().st_mtime >= max(p.stat().st_mtime for p in sources):
            return program
        self.say("Building the C++ program (first time only)...")
        try:
            BUILD_DIR.mkdir(parents=True, exist_ok=True)
            built = subprocess.run(["g++", "-std=c++17", "-O2", *SOURCES, "-o", str(program)],
                                   cwd=PROJECT, capture_output=True, text=True, timeout=300)
        except FileNotFoundError:
            raise RuntimeError("g++ was not found. Install a C++ compiler (see the README).")
        if built.returncode != 0:
            raise RuntimeError("The C++ program failed to build:\n\n" + built.stderr[-1500:])
        return program

    def run_menu(self, choices):
        program = self.ensure_program()
        RUN_DIR.mkdir(parents=True, exist_ok=True)
        text = "\n".join(str(choice) for choice in choices) + "\n"
        done = subprocess.run([str(program)], input=text, cwd=RUN_DIR,
                              capture_output=True, text=True, timeout=600)
        return done.stdout

    def clear_results(self, names=("dijkstra", "astar")):
        for name in names:
            for old in RUN_DIR.glob(f"{name}_*"):
                old.unlink()

    def say(self, message):
        self.status.set(message)
        self.root.update_idletasks()

    def guarded(self, work):
        self.stop_play()
        for button in self.action_buttons:
            button.state(["disabled"])
        try:
            work()
        except (ValueError, RuntimeError, OSError, subprocess.SubprocessError) as error:
            self.status.set("Something went wrong.")
            messagebox.showerror("CircuitRoute", str(error))
        finally:
            for button in self.action_buttons:
                button.state(["!disabled"])

    def generate_board(self):
        self.guarded(self.do_generate)

    def do_generate(self):
        n = self.read_form()
        self.say("Generating the board...")
        RUN_DIR.mkdir(parents=True, exist_ok=True)
        self.clear_results()
        output = self.run_menu([MENU_GENERATE, n["rows"], n["cols"], n["obstacles"],
                                n["connections"], n["seed"], n["cost"], MENU_SAVE, MENU_EXIT])
        if "Generated." not in output:
            raise RuntimeError("The program could not generate that board. Try fewer "
                               "obstacles or connections.")
        self.after_new_board("Board ready. Now press Run Dijkstra, Run A* or Run both.")

    def after_new_board(self, message):
        self.loaded = []
        self.board_ready = True
        self.show_board_preview()
        self.refresh_table()
        self.status.set(message)

    def run_algorithms(self, names):
        self.guarded(lambda: self.do_run(names))

    def do_run(self, names):
        if not self.board_ready:
            raise RuntimeError("Generate or load a board first.")
        choices = [MENU_LOAD]
        for name in names:
            self.clear_results([name])
            choices.append(MENU_DIJKSTRA if name == "dijkstra" else MENU_ASTAR)
        self.say("Routing with " + " and ".join(ALGORITHMS[n] for n in names) + "...")
        output = self.run_menu(choices + [MENU_EXIT])
        if "Loaded dataset.txt." not in output:
            raise RuntimeError("The saved board could not be loaded. Generate it again.")
        self.loaded = load_available(RUN_DIR)
        self.show_results()
        self.refresh_table()
        self.status.set("Done. Drag the slider or press Play to watch the wires.")

    def generate_and_route(self):
        self.guarded(lambda: (self.do_generate(), self.do_run(["dijkstra", "astar"])))

    def save_board(self):
        if not self.board_ready:
            messagebox.showinfo("CircuitRoute", "Generate or load a board first.")
            return
        filename = filedialog.asksaveasfilename(
            title="Save board", defaultextension=".txt", initialfile="my_board.txt",
            filetypes=[("CircuitRoute board", "*.txt")])
        if filename:
            target = Path(filename)
            shutil.copyfile(RUN_DIR / "dataset.txt", target)
            shutil.copyfile(RUN_DIR / "dataset_cells.csv",
                            target.with_name(target.stem + "_cells.csv"))
            self.status.set(f"Saved {target.name} (and {target.stem}_cells.csv).")

    def load_saved_board(self):
        filename = filedialog.askopenfilename(title="Load a saved board",
                                              filetypes=[("CircuitRoute board", "*.txt")])
        if filename:
            self.guarded(lambda: self.do_load(Path(filename)))

    def do_load(self, filename):
        RUN_DIR.mkdir(parents=True, exist_ok=True)
        kept = {name: (RUN_DIR / name).read_bytes() for name in ("dataset.txt", "dataset_cells.csv")
                if (RUN_DIR / name).exists()}
        shutil.copyfile(filename, RUN_DIR / "dataset.txt")
        output = self.run_menu([MENU_LOAD, MENU_SAVE, MENU_EXIT])
        if "Loaded dataset.txt." not in output:
            for name, data in kept.items():
                (RUN_DIR / name).write_bytes(data)
            raise RuntimeError(f"{filename.name} is not a valid CircuitRoute board file. "
                               "Your current board was kept.")
        self.clear_results()
        board = load_board(RUN_DIR / "dataset_cells.csv")
        costs = [cell["cost"] for cell in board["cells"].values()]
        self.values["rows"].set(str(board["rows"]))
        self.values["cols"].set(str(board["cols"]))
        self.values["obstacles"].set(str(sum(cell["blocked"] for cell in board["cells"].values())))
        self.values["connections"].set(str(len(read_connections(RUN_DIR / "dataset.txt"))))
        self.values["seed"].set(read_seed(RUN_DIR / "dataset.txt"))
        self.values["cost"].set(str(max(costs)))
        self.after_new_board(f"Loaded {filename.name}. Now press Run Dijkstra, Run A* or Run both.")

    def collision_demo(self):
        self.guarded(self.do_demo)

    def do_demo(self):
        output = self.run_menu([MENU_DEMO, MENU_EXIT])
        start = output.find("First wire accepted")
        end = output.find("This demonstration")
        if start < 0:
            raise RuntimeError("The collision demonstration did not run.")
        window = tk.Toplevel(self.root)
        window.title("Collision demonstration")
        ttk.Label(window, wraplength=520, justify="left", text=(
            "Wires may not cross each other. Here a first wire is placed along row 4, then a "
            "second wire tries to cross it. The first is accepted (1) and the crossing one is "
            "rejected (0).\n# = obstacle, S = start, T = target, * = wire")).pack(padx=12, pady=(12, 6))
        text = tk.Text(window, width=60, height=18, font=("Courier New", 11))
        text.insert("1.0", output[start:end].strip())
        text.configure(state="disabled")
        text.pack(padx=12, pady=(0, 12))
        self.status.set("Collision demonstration opened in a new window.")

    def run_benchmark(self):
        self.guarded(self.do_benchmark)

    def do_benchmark(self):
        self.say("Running the performance benchmark. This takes about 20 seconds...")
        output = self.run_menu([MENU_BENCHMARK, MENU_EXIT])
        if "Saved benchmark_results.csv" not in output:
            raise RuntimeError("The benchmark did not finish.")
        rows = load_rows(RUN_DIR / "benchmark_results.csv")
        draw_charts(self.performance_figure, rows)
        self.performance_canvas.draw_idle()
        agreed = re.search(r"disagreed on the path cost: (\d+)", output)
        findings = self.describe_benchmark(summary(rows), agreed and agreed.group(1) == "0")
        self.performance_note.set(EXPLAINER + "\n\nWhat we found: " + findings)
        self.tabs.select(2)
        self.status.set("Benchmark finished. See the Performance tab.")

    def describe_benchmark(self, table, same_costs):
        def speedups(experiment):
            return [row["speedup"] for row in table if row["experiment"] == experiment]
        uniform = speedups("distance_uniform")
        weighted = speedups("distance_weighted")
        longest = [row for row in table if row["experiment"] == "distance_uniform"][-1]
        text = (f"With every cell costing 1, A* was {min(uniform):.1f}x to {max(uniform):.1f}x faster than "
                f"Dijkstra, and the longer the wire the bigger the win. When cell costs vary from 1 to 9 the gap "
                f"shrinks to {min(weighted):.1f}x to {max(weighted):.1f}x, because A* guesses the distance left "
                f"as if every cell cost 1. On the longest wires A* held more waiting cells in memory "
                f"({longest['astar_max_queue_size']:.0f} against {longest['dijkstra_max_queue_size']:.0f}).")
        if same_costs:
            text += " Both algorithms found paths of the same cost every time."
        return text

    def save_picture(self):
        on_performance = self.tabs.index(self.tabs.select()) == 2
        figure = self.performance_figure if on_performance else self.figure
        if not figure.axes:
            messagebox.showinfo("CircuitRoute", "Nothing to save yet.")
            return
        filename = filedialog.asksaveasfilename(title="Save picture", defaultextension=".png",
                                                initialfile="circuitroute.png",
                                                filetypes=[("PNG picture", "*.png")])
        if filename:
            figure.savefig(filename, dpi=200, facecolor=figure.get_facecolor())
            self.status.set(f"Saved picture {Path(filename).name}.")

    def show_board_preview(self):
        board = load_board(RUN_DIR / "dataset_cells.csv")
        self.connections = read_connections(RUN_DIR / "dataset.txt")
        self.figure.clear()
        axis = self.figure.add_axes([0.12, 0.06, 0.76, 0.78])
        draw_generated_board(axis, board, read_seed(RUN_DIR / "dataset.txt"))
        self.views, self.axes, self.total, self.count = [], [], 0, 0
        self.scale.configure(to=0)
        self.scale.set(0)
        self.wire_label.set("")
        self.summary.set("Green dots = wire starts, blue squares = targets, dark squares = obstacles.")
        self.canvas.draw_idle()
        self.tabs.select(0)

    def show_results(self):
        self.figure.clear()
        self.axes = [self.figure.add_axes([0.07, 0.06, 0.4, 0.78]) for _ in self.loaded]
        self.views = [RouteView(axis, results, name)
                      for axis, (name, results) in zip(self.axes, self.loaded)]
        self.total = len(self.loaded[0][1]["metrics"])
        self.scale.configure(to=self.total)
        lines = []
        for name, results in self.loaded:
            metrics = results["metrics"]
            good = [row for row in metrics if row["status"] == "success"]
            lines.append(f"{name}: {len(good)}/{len(metrics)} wires routed, "
                         f"search time {sum(float(r['search_time_ms']) for r in metrics):.2f} ms, "
                         f"total path cost {sum(int(r['path_cost']) for r in good)}")
        self.summary.set("\n".join(lines) + "\nGray dots = where wires start and end.  "
                         "Red cross = a connection that could not be routed.")
        self.apply_view()
        self.set_count(self.total)
        self.tabs.select(0)

    def apply_view(self):
        if not self.views:
            return
        mode = self.view_mode.get()
        wanted = [i for i, (name, _) in enumerate(self.loaded)
                  if mode == "both" or (mode == "dijkstra") == (name == "Dijkstra")]
        if not wanted:
            wanted = list(range(len(self.axes)))
        positions = [[0.12, 0.06, 0.76, 0.78]] if len(wanted) == 1 else \
            [[0.07, 0.06, 0.40, 0.78], [0.55, 0.06, 0.40, 0.78]]
        for index, axis in enumerate(self.axes):
            axis.set_visible(index in wanted)
        for index, position in zip(wanted, positions):
            self.axes[index].set_position(position)
        self.set_count(self.count)

    def set_count(self, number):
        if not self.views or self.updating:
            return
        self.updating = True
        self.count = max(0, min(self.total, int(number)))
        for view in self.views:
            view.show_first(self.count)
        self.scale.set(self.count)
        last = [view.last_wire_text(self.count)
                for view, axis in zip(self.views, self.axes) if axis.get_visible()]
        self.wire_label.set(f"Wires routed: {self.count} / {self.total}    " + "   |   ".join(last))
        self.canvas.draw_idle()
        self.updating = False

    def refresh_table(self):
        self.table.delete(*self.table.get_children())
        outcomes = {}
        for name, results in self.loaded:
            for row in results["metrics"]:
                text = row["status"]
                if text == "success":
                    text += f" (cost {row['path_cost']}, {float(row['search_time_ms']):.3f} ms)"
                outcomes[(name, int(row["connection_id"]))] = text
        for number, start_row, start_col, target_row, target_col in self.connections:
            self.table.insert("", "end", values=(
                number, f"({start_row}, {start_col})", f"({target_row}, {target_col})",
                outcomes.get(("Dijkstra", number), "not run"),
                outcomes.get(("A*", number), "not run")))

    def toggle_play(self):
        if self.playing:
            self.stop_play()
            return
        if not self.views:
            return
        if self.count >= self.total:
            self.set_count(0)
        self.playing = True
        self.play_button.configure(text="Pause")
        self.root.after(250, self.play_step)

    def play_step(self):
        if not self.playing:
            return
        if self.count >= self.total:
            self.stop_play()
            return
        self.set_count(self.count + 1)
        self.root.after(250, self.play_step)

    def stop_play(self):
        self.playing = False
        self.play_button.configure(text="Play")


def main():
    root = tk.Tk()
    App(root)
    root.mainloop()


if __name__ == "__main__":
    main()
