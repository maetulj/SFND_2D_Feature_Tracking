#!/bin/bash python
import subprocess
import numpy as np
from matplotlib import pyplot as plt
import csv

vis = "false"

# Possible detectors.
detectors = ["SHITOMASI", "HARRIS", "FAST", "BRISK", "ORB", "AKAZE", "SIFT"]
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
        detector_results[detector][descriptor]["image"] = []

    detector_results[detector][descriptor]["total"].append(pts_total)
    detector_results[detector][descriptor]["vehicle"].append(pts_vehicle)
    detector_results[detector][descriptor]["matches"].append(matches)
    detector_results[detector][descriptor]["time_detector"].append(detector_time)
    detector_results[detector][descriptor]["time_descriptor"].append(descriptor_time)

    detector_results[detector][descriptor]["image"].append([detector, descriptor, pts_total, pts_vehicle, detector_time, descriptor_time, matches])

print("\nExporting data")

with open("./results/task8_9.csv", mode="w") as csv_file:
    csv_writer = csv.writer(csv_file, delimiter=",", quotechar='"', quoting=csv.QUOTE_MINIMAL)

    csv_writer.writerow(["Detector", "Descriptor", "Image", "Total Keypoints", "Keypoints on vehicle", "Detector Time", "Descriptor Time", "Matches"])

    for detector in detector_results:
        for descriptor in detector_results[detector]:
            for idx, image in enumerate(detector_results[detector][descriptor]["image"]):
                csv_writer.writerow([
                    image[0], 
                    image[1],
                    idx,
                    image[2],
                    image[3],
                    image[4],
                    image[5],
                    image[6]
                ])
            csv_writer.writerow([None])


print("\nPlotting fancy plots:")

# Average of total detected points.

for detector in detector_results:
    # Points detected in images for this detector.
    pts_total = []
    pts_vehicle = []

    descriptors = []
    detector_time = []
    descriptor_time = []
    matches = []

    # Points are equal for each descriptor.
    for descriptor in detector_results[detector]:
        detector_image_time = []
        for image in detector_results[detector][descriptor]:
            pts_total = detector_results[detector][descriptor]["total"]
            pts_vehicle = detector_results[detector][descriptor]["vehicle"]

        if descriptor not in descriptors:
            descriptors.append(descriptor)

        detector_time = detector_results[detector][descriptor]["time_detector"]

        time = np.array(detector_results[detector][descriptor]["time_descriptor"]).astype(np.float)
        descriptor_time.append(np.mean(time))

        matches.append(detector_results[detector][descriptor]["matches"])
    
    # Points in image.
    fig, ((ax, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(14, 8))

    # Total points.
    color = "tab:red"
    ax.set_xlabel("Image")
    ax.set_ylabel("Number of points detected in image", color=color)
    ax.plot(np.array(pts_total).astype(np.float), color=color)
    ax.tick_params(axis="y", labelcolor=color)

    ax.set_xticks(range(0, 10))

    # Number of points on vehicle.
    color = "tab:blue"
    ax_right = ax.twinx()
    ax_right.set_ylabel("Number of points detected on vehicle", color=color)
    ax_right.plot(pts_vehicle, color=color)
    ax_right.tick_params(axis="y", labelcolor=color)
    ax.set_title("{} detector".format(detector))
    plt.draw()

    # Detector time
    ax2.set_title("{} detector time".format(detector))
    ax2.set_xticks(range(0, 10))
    ax2.set_xlabel("Image")
    ax2.set_ylabel("Average time of the detector [ms]")
    ax2.plot(detector_time)
    plt.draw()

    # Descriptor time
    ax3.set_title("Average descriptor time with {} detector".format(detector))
    ax3.set_xticks(np.arange(len(descriptors)))
    ax3.set_xticklabels(descriptors)
    ax3.set_ylabel("Average time of the descriptor [ms]")
    ax3.plot(descriptor_time)
    plt.draw()

    ax4.set_title("Matched points with {} detector\nNote: First image skipped".format(detector))
    ax4.set_xticks(range(0, 10))
    ax4.set_xticklabels(range(1, 10))
    ax4.set_ylabel("Matched points")
    for match in matches:
        ax4.plot(np.array(match).astype(np.float)[1:])

    ax4.legend(descriptors)
    plt.draw()

    fig.tight_layout()
    fig.canvas.set_window_title("{}".format(detector))
    plt.savefig("./results/{}.png".format(detector))
plt.show()


print("Finished")