"""
This script takes json and mpk input files and creates a PR comment in markdown format.

An mpk (msgpack) file contains the benchmark result history for a specific config.
Unpacked, this file will have the following format:
```json
[
    {
        "commit_hash": "<commit_hash>"
        "scenes": [
            {
                "scene_name": "",
                "avg_cpu": 0,
                "avg_fps": 0,
                "avg_time": 0,
                "render_time": 0,
                "flush_time": 0,
            },
            ...
        ] 
    },
    ...
]
```

A json file contains the benchmark result for a specific commit:

```json
[
    {
        "scene_name": "",
        "avg_cpu": 0,
        "avg_fps": 0,
        "avg_time": 0,
        "render_time": 0,
        "flush_time": 0,
    }
    ...
]
```

This script reads the previous and new benchmark results, compares them and creates a comment.

Example comment:

```
Hi :wave:, thank you for your PR!

We've run benchmarks in an emulated environment. Here are the results:

#### ARM Emulated 32b - lv_conf_perf32b

| Scene Name | Avg CPU (%) | Avg FPS | Avg Time (ms) | Render Time (ms) | Flush Time (ms) |
|------------|------------|---------|--------------|----------------|--------------|
| All scenes avg. | 20 | 24 | 7 | 7 | 0 |

<details>
<summary>
Detailed Results Per Scene
</summary>

| Scene Name | Avg CPU (%) | Avg FPS | Avg Time (ms) | Render Time (ms) | Flush Time (ms) |
|------------|------------|---------|--------------|----------------|--------------|
| Empty screen | 11 | 25 | 0 | 0 | 0 |
| Moving wallpaper | 1 | 25 | 0 | 0 | 0 |
| Single rectangle | 0 | 25 | 0 | 0 | 0 |
| Multiple rectangles | 0 | 25 | 0 | 0 | 0 |
| Multiple RGB images | 0 | 25 | 0 | 0 | 0 |
| Multiple ARGB images | 22 (-1)| 25 | 1 | 1 | 0 |
| Rotated ARGB images | 47 (-1)| 24 | 20 | 20 | 0 |
| Multiple labels | 2 (-2)| 25 | 0 | 0 | 0 |
| Screen sized text | 30 (+1)| 24 (-1)| 11 (-1)| 11 (-1)| 0 |
| Multiple arcs | 19 (+4)| 24 | 7 | 7 | 0 |
| Containers | 1 (-1)| 25 | 0 | 0 | 0 |
| Containers with overlay | 87 (-2)| 21 | 44 | 44 | 0 |
| Containers with opa | 23 (+1)| 25 | 4 | 4 | 0 |
| Containers with opa_layer | 22 (+1)| 25 | 8 | 8 | 0 |
| Containers with scrolling | 25 | 25 | 10 | 10 | 0 |
| Widgets demo | 34 | 24 (-1)| 13 | 13 | 0 |
| All scenes avg. | 20 | 24 | 7 | 7 | 0 |


</details>


Disclaimer: These benchmarks were run in an emulated environment using QEMU with instruction counting mode.
The timing values represent relative performance metrics within this specific virtualized setup and should
not be interpreted as absolute real-world performance measurements. Values are deterministic and useful for
comparing different LVGL features and configurations, but may not correlate directly with performance on
physical hardware. The measurements are intended for comparative analysis only.


---

:robot: This comment was automatically generated by a bot.
```
"""

import argparse
import json
import msgpack


DISCLAIMER = """
Disclaimer: These benchmarks were run in an emulated environment using QEMU with instruction counting mode.
The timing values represent relative performance metrics within this specific virtualized setup and should
not be interpreted as absolute real-world performance measurements. Values are deterministic and useful for
comparing different LVGL features and configurations, but may not correlate directly with performance on
physical hardware. The measurements are intended for comparative analysis only.
"""

DATA_KEYS = ["avg_cpu", "avg_fps", "avg_time", "render_time", "flush_time"]


def generate_table(results: list[dict], prev_results: list[dict]) -> list[dict]:
    return [
        {key: ((scene_results[key] - prev_scene_results[key])) for key in DATA_KEYS}
        for scene_results, prev_scene_results in zip(results, prev_results)
    ]


def format_table(results: list[dict], delta: list[dict]) -> str:

    table = "| Scene Name | Avg CPU (%) | Avg FPS | Avg Time (ms) | Render Time (ms) | Flush Time (ms) |\n"
    table += "|------------|------------|---------|--------------|----------------|--------------|\n"

    for scene_results, delta_p in zip(results, delta):
        table += f"| {scene_results['scene_name']} |"
        for key in DATA_KEYS:
            if delta_p[key] == 0:
                table += f" {scene_results[key]} |"
            else:
                table += f" {scene_results[key]} ({delta_p[key]:+})|"
        table += "\n"

    return table


def main():
    parser = argparse.ArgumentParser(
        description="Process previous and new results, and output a comment file."
    )

    parser.add_argument(
        "--previous",
        type=str,
        nargs="+",
        required=False,
        help="Path to the previous results file (supports multiple, e.g., results*.mpk)",
    )
    parser.add_argument(
        "--new",
        type=str,
        nargs="+",
        required=True,
        help="Paths to new results files (supports multiple, e.g., results*.json)",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=str,
        required=True,
        help="Output file path (e.g., comment.md)",
    )

    args = parser.parse_args()
    previous_results_paths = args.previous
    results_paths = args.new
    output_path = args.output

    print(f"Previous results: {previous_results_paths}")
    print(f"New results: {results_paths}")

    previous_results_map: dict[str, list[dict]] = {}
    if previous_results_paths:
        for results_path in previous_results_paths:
            _, image_type, config = results_path.replace(".mpk", "").split("-")

            with open(results_path, "rb") as f:
                previousb = f.read()
                rs: list = msgpack.unpackb(previousb)
                previous_results_map[results_path] = rs

    new_results: dict[str, list[dict]] = {}
    for results_path in results_paths:
        with open(results_path, "r") as f:
            r: list[dict] = json.load(f)
            new_results[results_path] = r

    comment = "Hi :wave:, thank you for your PR!\n\n"
    comment += "We've run benchmarks in an emulated environment."
    comment += " Here are the results:\n\n"

    diff_found = False
    for result_path, result in new_results.items():
        mpk_path = result_path.replace(".json", ".mpk")

        new_all_scene_avg = [
            scene for scene in result if scene["scene_name"] == "All scenes avg."
        ]

        prev_results = previous_results_map.get(mpk_path, [])

        if len(prev_results) > 0:
            print(f"Found previous results for {result_path} in {mpk_path}")
            prev_scenes = prev_results[-1]["scenes"]
            prev_all_scene_avg = [
                [
                    scene
                    for scene in prev_scenes
                    if scene["scene_name"] == "All scenes avg."
                ][0]
            ]
            prev_results = prev_scenes
        else:
            # If there are no previous results, we use the current result as
            # the previous aswell
            # In this case, the difference will always be zero and we won't
            # add any new information to the result table
            print(f"Previous results not found for {result_path}")
            diff_found = True  # Generate a real comment since we got new results
            prev_results = result
            prev_all_scene_avg = new_all_scene_avg

        delta_all_scene_avg = generate_table(new_all_scene_avg, prev_all_scene_avg)
        delta_results_avg = generate_table(result, prev_results)

        diff_found = diff_found or any(
            value != 0 for dic in delta_results_avg for value in dic.values()
        )

        _, image_type, config = result_path.replace(".json", "").split("-")
        comment += f"#### ARM Emulated {image_type} - {config}\n\n"
        comment += format_table(new_all_scene_avg, delta_all_scene_avg)
        comment += "\n<details>"
        comment += "\n<summary>"
        comment += "\nDetailed Results Per Scene"
        comment += "\n</summary>\n\n"
        comment += format_table(result, delta_results_avg)
        comment += "\n\n</details>\n\n"

    comment += DISCLAIMER
    comment += "\n\n"
    comment += "---"
    comment += "\n\n"
    comment += ":robot: This comment was automatically generated by a bot."

    # Create the file but don't write anything to it if we haven't found a difference
    with open(output_path, "w") as f:
        if diff_found:
            f.write(comment)


if __name__ == "__main__":
    main()
