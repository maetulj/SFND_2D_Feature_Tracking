#!/bin/bash python
import subprocess
import numpy as np
from matplotlib import pyplot as plt

vis = "false"

# Possible detectors.
detectors = ["SHITOMASI"] # ["SHITOMASI", "HARRIS", "FAST", "BRISK", "ORB", "AKAZE", "SIFT"]
descriptors = ["BRISK", "BRIEF", "ORB", "FREAK", "AKAZE", "SIFT"]

print("Computing...", end="", flush=True)

# Results.
results = []

for det in detectors:
    for desc in descriptors:
        try:
            p = subprocess.Popen("./2D_feature_tracking {} {} {}".format(vis, det, desc), cwd="./build", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
            out, err = p.communicate()
            if p.returncode == 0:
                out = out.split("\n")
                for line in out:
                    if line.startswith("Detector"):
                        results.append(line)
            else:
                results.append("Invalid combination|{}|{}".format(det, desc))
        except Exception as e:
            pass
        print(".", end="", flush=True)

print("Results:")
for line in results:
    print(line)

print("\nProcessing data:")

detector_results = {}

for line in results:
    # Discard invalid combinations.
    if line.startswith("Invalid"):
        combination = line.split("|")
        print("Detector {} and descriptor {} form an invalid combination. Discarding results".format(combination[1], combination[2]))
        continue

    # Join detectors.
    combination_results = line.split("|")
    print(combination_results)
    
    # First element is the detector.
    detector = combination_results[0].split(":")[1]
    # Second element is the descriptor.
    descriptor = combination_results[1].split(":")[1]
    # Third element is the matcher.
    matcher = combination_results[2].split(":")[1]
    # Fourth element is the number of points total.
    pts_total = combination_results[3].split(":")[1]
    # Fifth element is the number of points on the vehicle in front.
    pts_vehicle = combination_results[4].split(":")[1]
    # Sixth element is the number of matches.
    matches = combination_results[5].split(":")[1]
    # Seventh element is the time for detector.
    detector_time = combination_results[6].split(":")[1]
    # Eighth element is the time for descriptor.
    descriptor_time = combination_results[7].split(":")[1]

    # Join them together.
    if detector not in detector_results:
        detector_results[detector] = {}

    if descriptor not in detector_results[detector]:
        detector_results[detector][descriptor] = {}
        detector_results[detector][descriptor]["total"] = []
        detector_results[detector][descriptor]["vehicle"] = []
        detector_results[detector][descriptor]["matches"] = []
        detector_results[detector][descriptor]["time_detector"] = []
        detector_results[detector][descriptor]["time_descriptor"] = []

    detector_results[detector][descriptor]["total"].append(pts_total)
    detector_results[detector][descriptor]["vehicle"].append(pts_vehicle)
    detector_results[detector][descriptor]["matches"].append(matches)
    detector_results[detector][descriptor]["time_detector"].append(detector_time)
    detector_results[detector][descriptor]["time_descriptor"].append(descriptor_time)

print("\nPlotting fancy plots:")

# Average of total detected points.

for detector in detector_results:

    def autolabel(rects, ax):
        for rect in rects:
            height = rect.get_height()
            ax.annotate("{}".format(height), xy=(rect.get_x() + rect.get_width(), height), xytext=(0, 3), textcoords="offset points", ha="center", va="bottom")

    # Points total, points on vehicle and matches
    labels = []
    avgs_total = []
    avgs_vehicle = []
    avgs_matches = []
    avgs_detector_times = []
    avgs_descriptor_times = []

    for descriptor in detector_results[detector]:
        labels.append(descriptor)

        total = np.array(detector_results[detector][descriptor]["total"]).astype(np.float)
        avgs_total.append(np.mean(total))

        vehicle = np.array(detector_results[detector][descriptor]["vehicle"]).astype(np.float)
        avgs_vehicle.append(np.mean(vehicle))

        matches = np.array(detector_results[detector][descriptor]["matches"]).astype(np.float)
        avgs_matches.append(np.mean(matches))

        detector_times = np.array(detector_results[detector][descriptor]["time_detector"]).astype(np.float)
        avgs_detector_times.append(np.round(np.mean(detector_times), 2))

        descriptor_times = np.array(detector_results[detector][descriptor]["time_descriptor"]).astype(np.float)
        avgs_descriptor_times.append(np.round(np.mean(descriptor_times), 2))

    x = np.arange(len(labels))
    width = 0.3

    # Plot bar graph for points total, points on vehicle and matches
    fig, (ax, ax2) = plt.subplots(1, 2)
    # fig.suptitle("Results for the {} detector".format(detector))

    rects_total = ax.bar(x - width / 2, avgs_total, width / 3, label="Total")
    rects_vehicle = ax.bar(x, avgs_vehicle, width / 3, label="On Vehicle")
    rects_matches = ax.bar(x + width / 2, avgs_matches, width  / 3, label="Matches")

    ax.set_ylabel("Mean number of points")
    ax.set_title("{}: Mean of Points Detected".format(detector))
    ax.set_xticks(x)
    ax.set_xticklabels(labels)
    ax.legend()

    autolabel(rects_total, ax)
    autolabel(rects_vehicle, ax)
    autolabel(rects_matches, ax)

    fig.tight_layout()
    fig.canvas.set_window_title("{}".format(detector))
    plt.draw()

    # Save to file.
    plt.savefig("./results/mean_points_{}.png".format(detector))

    # Ploat bar graph for time for detector and descriptor
    # fig, ax = plt.subplots()
    rects_detector = ax2.bar(x - width / 2, avgs_detector_times, width, label="Detector")
    rects_descriptor = ax2.bar(x + width / 2, avgs_descriptor_times, width, label="Descriptor")

    ax2.set_ylabel("Mean time [ms]")
    ax2.set_title("{}: Detector and Descriptor Processing Times".format(detector))
    ax2.set_xticks(x)
    ax2.set_xticklabels(labels)
    ax2.legend()

    autolabel(rects_detector, ax2)
    autolabel(rects_descriptor, ax2)

    fig.tight_layout()
    fig.canvas.set_window_title("{}".format(detector))
    plt.draw()

    # Save to file.
    plt.savefig("./results/computing_times_{}.png".format(detector))

plt.show()

print("Finished")