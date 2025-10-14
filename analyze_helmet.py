import re
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

# ---------------------------
# CONFIGURATION
# ---------------------------
files = [
    "helmet_data_1.csv",
    "helmet_data_2.csv",
    "helmet_data_3.csv"
]

section_names = [
    "Helmet worn Not buckled",
    "Helmet worn and buckled",
    "Helmet remove and Not Bucked"
]

# ---------------------------
# HELPER FUNCTION
# ---------------------------
def parse_file(filename):
    """Parse a single CSV file into structured section data."""
    data = {name: [] for name in section_names}
    current_section = None

    # FIX: UTF-8 decode with ignore for special chars (✅⚠️)
    with open(filename, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            line = line.strip()
            # Detect section header
            if line.startswith("##"):
                for name in section_names:
                    if name in line:
                        current_section = name
                        break
                continue

            # Match data lines
            match = re.search(r"helmetTouched:\s*(\d+),\s*fsrValue:\s*(\d+),\s*buckled:\s*(\d+)", line)
            if match and current_section:
                helmet, fsr, buckle = map(int, match.groups())
                data[current_section].append({
                    "helmetTouched": helmet,
                    "fsrValue": fsr,
                    "buckled": buckle
                })
    return data

# ---------------------------
# AGGREGATE ALL FILES
# ---------------------------
all_data = {name: [] for name in section_names}

for file in files:
    file_path = Path(file)
    parsed = parse_file(file_path)
    for section, readings in parsed.items():
        if readings:
            df = pd.DataFrame(readings)
            df["trial"] = file_path.stem
            all_data[section].append(df)

# ---------------------------
# COMPUTE AVERAGES PER SECTION
# ---------------------------
averages = []
for section, dfs in all_data.items():
    combined = pd.concat(dfs)
    avg = combined.mean(numeric_only=True)
    averages.append({
        "Section": section,
        "FSR Avg": avg["fsrValue"],
        "HelmetTouch Avg": avg["helmetTouched"],
        "Buckle Avg": avg["buckled"]
    })

avg_df = pd.DataFrame(averages)
print("\n=== Experimental Results Summary ===")
print(avg_df)

# ---------------------------
# CHART 1: Average Comparison Across Sections
# ---------------------------
plt.figure(figsize=(10,6))
plt.plot(avg_df["Section"], avg_df["FSR Avg"], marker='o', label="FSR Value (Pressure)")
plt.plot(avg_df["Section"], avg_df["HelmetTouch Avg"], marker='s', label="Helmet Detection")
plt.plot(avg_df["Section"], avg_df["Buckle Avg"], marker='^', label="Buckle Detection")

plt.title("Average Sensor Readings Across Helmet Conditions")
plt.xlabel("Helmet Condition Section")
plt.ylabel("Average Sensor Value")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()

# ---------------------------
# CHART 2: Section-wise Data Comparison (All Trials)
# ---------------------------
fig, axes = plt.subplots(len(section_names), 1, figsize=(10, 10), sharex=False)
fig.suptitle("Sensor Data Variation per Section (All Trials)", fontsize=14)

for idx, section in enumerate(section_names):
    ax = axes[idx]
    for df in all_data[section]:
        ax.plot(df["fsrValue"], label=f"{df['trial'][0]} - FSR", alpha=0.7)
    ax.set_title(section)
    ax.set_ylabel("FSR Value")
    ax.grid(True)
    ax.legend()

axes[-1].set_xlabel("Sample Index")
plt.tight_layout(rect=[0, 0, 1, 0.97])
plt.show()
