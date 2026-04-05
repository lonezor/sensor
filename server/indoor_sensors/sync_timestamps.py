#!/usr/bin/env python3

import os
import glob
import json
import re
import pandas as pd
import numpy as np

SOURCES = {
    "wifi": "/var/www/html/sensor/wifi_sensors",
    "lan": "/var/www/html/sensor/lan_sensors",
}
OUTPUT_FILE = "/var/www/ref/sensor_data.js"
UNIT_RE = re.compile(r'^(?:UNIT_)?(\d+)$')


def find_measurement_files(sources):
    files = []
    for source_name, base_dir in sources.items():
        if not os.path.isdir(base_dir):
            continue
        pattern = os.path.join(base_dir, "**", "measurements.csv")
        for path in glob.glob(pattern, recursive=True):
            files.append((source_name, base_dir, path))
    return files


def extract_unit_name(path, base_dir):
    norm_path = os.path.normpath(path)
    norm_base = os.path.normpath(base_dir)
    rel = os.path.relpath(norm_path, norm_base)
    if rel.startswith(".."):
        return None
    parts = rel.split(os.sep)
    if len(parts) < 4:
        return None

    first = parts[0]
    match = UNIT_RE.match(first)
    if not match:
        return None
    return f"unit_{int(match.group(1)):02d}"


def process_unit(paths):
    if not paths:
        return []

    dfs = []
    for path in paths:
        try:
            df = pd.read_csv(path, header=None, names=['ts', 'temp', 'humid'])
            if len(df) > 0:
                df['ts'] = pd.to_numeric(df['ts'], errors='coerce').astype('Int64')
                df['temp'] = pd.to_numeric(df['temp'], errors='coerce')
                df = df.dropna(subset=['ts', 'temp'])
                if len(df) > 0:
                    df['ts'] = df['ts'].astype(np.int64)
                    dfs.append(df[['ts', 'temp']])
        except Exception:
            pass

    if not dfs:
        return []

    df_all = pd.concat(dfs, ignore_index=True)
    df_all['ts_grid_sec'] = (df_all['ts'] // 900) * 900
    df_grouped = df_all.groupby('ts_grid_sec', as_index=False)['temp'].mean()
    df_grouped = df_grouped.sort_values('ts_grid_sec')

    return [
        [int(row.ts_grid_sec) * 1000, round(float(row.temp), 2)]
        for row in df_grouped.itertuples(index=False)
    ]


def sync_sensor_data():
    discovered = find_measurement_files(SOURCES)

    grouped = {source_name: {} for source_name in SOURCES}
    for source_name, base_dir, path in discovered:
        unit_name = extract_unit_name(path, base_dir)
        if unit_name is None:
            continue
        grouped.setdefault(source_name, {}).setdefault(unit_name, []).append(path)

    output = {}
    for source_name in sorted(grouped.keys()):
        output[source_name] = {}
        for unit_name in sorted(grouped[source_name].keys()):
            output[source_name][unit_name] = process_unit(grouped[source_name][unit_name])

    js_content = "var synced_data = " + json.dumps(output, separators=(",", ":")) + ";\n"

    os.makedirs(os.path.dirname(OUTPUT_FILE), exist_ok=True)
    with open(OUTPUT_FILE, 'w') as f:
        f.write(js_content)

    print(f"Wrote {OUTPUT_FILE}")


if __name__ == "__main__":
    sync_sensor_data()


