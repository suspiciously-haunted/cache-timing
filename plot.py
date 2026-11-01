import csv, sys
import matplotlib.pyplot as plt

SERIES = {"chase": ("Pointer chase", "#2a78d6"),
          "reversed": ("Bubble sort, reversed", "#eb6834"),
          "random": ("Bubble sort, random", "#1baf7a")}
CACHES = [(128 * 1024, "L1 128 KB"), (16 * 1024**2, "L2 16 MB")]

fig, ax = plt.subplots(figsize=(10, 5.5), facecolor="#fcfcfb")
ax.set_facecolor("#fcfcfb")

for size, label in CACHES:
    ax.axvline(size, color="#999999", linewidth=1, linestyle="--")
    ax.annotate(label, (size, 1), xycoords=("data", "axes fraction"),
                xytext=(4, -12), textcoords="offset points", color="#555555")

for path in sys.argv[1:]:
    rows = list(csv.DictReader(open(path)))
    name, color = SERIES[rows[0]["test"]]
    x = [int(r["bytes"]) for r in rows]
    y = [float(r["ns_per_op"]) for r in rows]
    ax.plot(x, y, color=color, linewidth=2, marker="o", markersize=4, label=name)
    ax.annotate(name, (x[-1], y[-1]), xytext=(0, 8), textcoords="offset points",
                ha="right", color="#333333")

ax.set_xscale("log", base=2)
ax.set_yscale("log")
ax.set_xlabel("Memory used (bytes)")
ax.set_ylabel("Time per comparison or step (ns)")
ax.set_title("Time per memory operation as data grows, Apple M4")
ax.grid(True, which="major", color="#e6e6e6", linewidth=0.8)
ax.spines[["top", "right"]].set_visible(False)
ax.legend(frameon=False, loc="upper left", bbox_to_anchor=(0, 0.92))
fig.tight_layout()
fig.savefig("cache_plot.png", dpi=160)
