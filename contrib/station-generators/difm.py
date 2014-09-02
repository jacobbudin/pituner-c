#!/usr/bin/env python3

# Convert Digitally Imported stations into Pituner-compatible stations.json

import json
import urllib.request

stations_endpoint = 'http://listen.di.fm/public3'
stations_response = urllib.request.urlopen(stations_endpoint)
stations_text = stations_response.read().decode('utf-8')
stations = json.loads(stations_text)

stations_converted = []

for station in stations:
	stations_converted.append({
		'name': station['name'],
		'url': station['playlist'],
	})

print(json.dumps(stations_converted))
