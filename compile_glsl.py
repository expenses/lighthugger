import sys
import re
import copy
import subprocess
import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("filenames", nargs="+")
parser.add_argument("--flags")
parser.add_argument("--shader-stage")
parser.add_argument("--out-dir", default=".")
args = parser.parse_args()

if not os.path.exists(args.out_dir):
    os.mkdir(args.out_dir)

entry_point_regex = re.compile("^void (\w+)\(\)")
layout_regex = re.compile("^layout\((location|local_size)")


# Find a block range. For functions and layouts (like uniforms).
def find_block_range(lines, line_start):
    balance = 0

    if "{" not in lines[line_start]:
        return range(line_start, line_start + 1)

    for i, line in enumerate(lines[line_start:]):
        for char in line:
            if char == "{":
                balance += 1
            elif char == "}":
                balance -= 1
                if balance == 0:
                    return range(line_start, i + line_start + 1)


def comment_out(lines, line_nums):
    for line_num in line_nums:
        lines[line_num] = "//" + lines[line_num]


for filename in args.filenames:
    contents = open(filename).read()
    parent_dir = os.path.dirname(filename)

    basename = os.path.basename(filename).split(".")[0]

    fallback_shader_stage = args.shader_stage

    if not fallback_shader_stage:
        if filename.endswith(".comp"):
            fallback_shader_stage = "comp"

    lines = contents.split("\n")

    entry_points = [
        (line_num, match)
        for (line_num, match) in (
            (line_num, entry_point_regex.match(line))
            for (line_num, line) in enumerate(lines)
        )
        if match != None
    ]

    if len(entry_points) == 0:
        print(f"No entry points found in {filename}")
        sys.exit(1)

    prev_line_num = 0

    entry_point_ranges = []

    for entry_point_line_num, entry_point in entry_points:
        entry_point_ranges.append(find_block_range(lines, entry_point_line_num))

    for i, (entry_point_line_num, entry_point) in enumerate(entry_points):
        shader_stage = None

        # Find the last shader stage tag.
        for line_num in range(prev_line_num, entry_point_line_num):
            if lines[line_num] == "//vert":
                shader_stage = "vert"
                break
            elif lines[line_num] == "//frag":
                shader_stage = "frag"
                break
            elif lines[line_num] == "//comp":
                shader_stage = "comp"
                break

        shader_stage = shader_stage or fallback_shader_stage

        if not shader_stage:
            print(f"shader stage missing for {filename} {entry_point.group(1)}")
            sys.exit(1)

        entry_point_lines = copy.copy(lines)

        # Comment out the other entry points
        for j, entry_point_range in enumerate(entry_point_ranges):
            if i == j:
                continue
            comment_out(entry_point_lines, entry_point_ranges[j])

        # Comment out layouts belonging to entry points above this one
        comment_out(
            entry_point_lines,
            (
                line_num
                for (line_num, line) in enumerate(lines[:prev_line_num])
                if layout_regex.match(line)
            ),
        )

        # Comment out layouts belonging to entry points below this one
        comment_out(
            entry_point_lines,
            (
                entry_point_line_num + line_num
                for (line_num, line) in enumerate(lines[entry_point_line_num:])
                if layout_regex.match(line)
            ),
        )

        # Rename the entry point
        entry_point_lines[entry_point_line_num] = entry_point_lines[
            entry_point_line_num
        ].replace(entry_point.group(0), "void main()")

        # Name the file afte the entry point, or use the basename
        # of the file if the entry point is 'main
        entry_point_name = entry_point.group(1)

        output_filename = None
        if entry_point_name == "main":
            output_filename = basename + ".spv"
        else:
            output_filename = entry_point_name + ".spv"

        file_contents = "\n".join(entry_point_lines)

        try:
            subprocess.run(
                f"glslc {args.flags} -I {parent_dir} -fshader-stage={shader_stage} - -o {os.path.join(args.out_dir, output_filename)}".split(
                    " "
                ),
                input=file_contents,
                text=True,
                check=True,
            )
        except subprocess.CalledProcessError:
            # for (i, line) in enumerate(entry_point_lines):
            #    print(f"{i+1}: {line}")
            print(f"Error occured in {filename}: {entry_point_name}")
            sys.exit(1)

        prev_line_num = entry_point_ranges[i][-1] + 1
