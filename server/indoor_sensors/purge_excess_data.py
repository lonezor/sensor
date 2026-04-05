#!/usr/bin/env python3

import os
import argparse
import math

BASE_DIRS = [
    "/var/www/html/sensor/wifi_sensors",
    "/var/www/html/sensor/lan_sensors",
]


def process_file(path, per_hour):
    with open(path, "r") as f:
        lines = [line.rstrip("\n") for line in f if line.strip()]
    if not lines:
        return 0

    rows = []
    for idx, line in enumerate(lines):
        parts = line.split(",", 1)
        try:
            ts = int(parts[0])
        except (ValueError, IndexError):
            continue
        rows.append((idx, ts, line))

    if not rows:
        return 0

    keep_indices = set()
    by_hour = {}
    for idx, ts, line in rows:
        hour = ts // 3600
        by_hour.setdefault(hour, []).append((ts, idx, line))

    for hour, items in by_hour.items():
        items.sort()
        if len(items) <= per_hour:
            for ts, idx, line in items:
                keep_indices.add(idx)
        else:
            step = len(items) / float(per_hour)
            for k in range(per_hour):
                pos = int(math.floor(k * step + step / 2.0))
                if pos >= len(items):
                    pos = len(items) - 1
                ts, idx, line = items[pos]
                keep_indices.add(idx)

    new_lines = [line for i, line in enumerate(lines) if i in keep_indices]

    backup_path = path + ".bak"
    if not os.path.exists(backup_path):
        os.rename(path, backup_path)

    with open(path, "w") as f:
        for line in new_lines:
            f.write(line + "\n")

    return len(lines) - len(new_lines)


def scan_tree(root, per_hour, dry_run=False):
    total_removed = 0
    for dirpath, dirnames, filenames in os.walk(root):
        for fname in filenames:
            if fname != "measurements.csv":
                continue
            path = os.path.join(dirpath, fname)
            if dry_run:
                with open(path, "r") as f:
                    count = sum(1 for _ in f)
                print(f"[DRY RUN] Would process {path}, rows={count}")
                continue
            removed = process_file(path, per_hour)
            print(f"Processed {path}, removed {removed} rows")
            total_removed += removed
    return total_removed


def scan_roots(roots, per_hour, dry_run=False):
    total_removed = 0
    for root in roots:
        if not os.path.isdir(root):
            print(f"Skipping missing directory: {root}")
            continue
        total_removed += scan_tree(root, per_hour, dry_run=dry_run)
    return total_removed


def main():
    parser = argparse.ArgumentParser(
        description="Downsample measurement CSV files by hour."
    )
    parser.add_argument(
        "roots",
        nargs="*",
        default=BASE_DIRS,
        help="Root directories to scan (default: both wifi_sensors and lan_sensors)",
    )
    parser.add_argument(
        "--per-hour",
        "-n",
        type=int,
        default=2,
        help="Number of entries to keep per hour (default: 2)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Scan and report without modifying any files",
    )

    args = parser.parse_args()

    total_removed = scan_roots(args.roots, args.per_hour, dry_run=args.dry_run)
    if not args.dry_run:
        print(f"Total removed rows: {total_removed}")


if __name__ == "__main__":
    main()

