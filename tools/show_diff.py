#!/usr/bin/env python3

import datetime

for line in open("diff.txt"):
    ending, steps, duration_in_minutes, distance_in_km,calories,duration_in_seconds,foo = line.split(";")
    ending=datetime.datetime.strptime(ending, "%d-%m-%Y %H:%M:%S")
    beginning = ending - datetime.timedelta(minutes=int(duration_in_minutes))
    beginning=beginning.strftime("%d-%m-%Y %H:%M:%S")
    distance_in_km=distance_in_km.replace(".",",")
    print(f"{beginning};{steps};{duration_in_minutes};{distance_in_km};{calories};")

