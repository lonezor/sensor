#!/usr/bin/env python3
import os
import json

TEMPLATE_HTML = "/var/www/ref/temperature_base.html"
OUTPUT_HTML = "/var/www/html/sensor/index.html"
SYNCED_JS = "/var/www/ref/sensor_data.js"
PLACEHOLDER = "__SENSOR_DATA_PLACEHOLDER__"


def load_synced_data(js_path):
    with open(js_path, "r", encoding="utf-8") as f:
        js_str = f.read().strip()

    prefixes = [
        "var synced_data =",
        "var syncedData =",
    ]

    payload = None
    for prefix in prefixes:
        if js_str.startswith(prefix):
            payload = js_str[len(prefix):].strip()
            break

    if payload is None:
        raise ValueError(f"Unexpected format in {js_path}")

    if payload.endswith(";"):
        payload = payload[:-1].strip()

    return json.loads(payload)


def build_sensor_data(synced_data):
    sensor_data = {}

    source_prefix_map = {
        "wifi": "wifi",
        "lan": "lan",
    }

    for source_name in sorted(synced_data.keys()):
        units = synced_data[source_name]
        source_prefix = source_prefix_map.get(source_name.lower(), source_name.lower())

        for unit_name in sorted(units.keys()):
            digits = "".join(ch for ch in unit_name if ch.isdigit())
            if digits:
                sensor_key = f"{source_prefix}_{digits.zfill(2)}"
            else:
                sensor_key = f"{source_prefix}_{unit_name.lower()}"

            sensor_data[sensor_key] = units[unit_name]

    return sensor_data


def replace_sensor_data_placeholder(html, replacement_json):
    if PLACEHOLDER not in html:
        raise ValueError(f"Could not find placeholder {PLACEHOLDER} in template")
    return html.replace(PLACEHOLDER, replacement_json)


def main():
    print(f"Loading synced data from {SYNCED_JS}...")
    synced_data = load_synced_data(SYNCED_JS)

    print("Building sensorData object...")
    sensor_data = build_sensor_data(synced_data)
    replacement_json = json.dumps(sensor_data, separators=(",", ":"))

    print(f"Applying to template {TEMPLATE_HTML}...")
    with open(TEMPLATE_HTML, "r", encoding="utf-8") as f:
        html = f.read()

    new_html = replace_sensor_data_placeholder(html, replacement_json)

    os.makedirs(os.path.dirname(OUTPUT_HTML), exist_ok=True)
    with open(OUTPUT_HTML, "w", encoding="utf-8") as f:
        f.write(new_html)

    print(f"Wrote {OUTPUT_HTML}")
    print("Success!")


if __name__ == "__main__":
    main()

