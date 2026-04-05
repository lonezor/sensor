#!/bin/bash

mkdir -p /var/www/html/sensor
cp -rfv /var/www/ref/echarts-6.0.0/test/* /var/www/html/sensor/
cp -rv  /var/www/ref/echarts-6.0.0/dist /var/www/html/sensor/..
python3 /var/www/ref/purge_excess_data.py --per-hour 8 
python3 /var/www/ref/sync_timestamps.py
python3 /var/www/ref/generate_temp_page.py
