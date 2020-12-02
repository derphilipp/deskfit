#!/usr/bin/env bash
cp walking.csv ./logs/$(date +"%Y%m%d-%H%M").csv
comm -23 walking.csv added.csv  > diff.txt
python3 deskfit/tools/show_diff.py
cp walking.csv added.csv
