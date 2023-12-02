import sys
import re
import copy
import subprocess
import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("filenames", nargs='+')
parser.add_argument("--flags")
args = parser.parse_args()

entry_point_regex = re.compile("^void (\w+)\(\)")

def find_function_range(lines, line_start):
    balance = 0
    for (i, line) in enumerate(lines[line_start:]):
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

for file in args.filenames:
    contents = open(file).read()
    parent_dir = os.path.dirname(file)

    lines = contents.split("\n")

    entry_points = [
        (line_num, match) for (line_num, match) in (
            (line_num, entry_point_regex.match(line)) for (line_num, line) in enumerate(lines)
        ) if match != None
    ]

    prev_line_num = 0

    entry_point_ranges = []

    for (entry_point_line_num, entry_point) in entry_points:
        entry_point_ranges.append(find_function_range(lines, entry_point_line_num))

    for (i, (entry_point_line_num, entry_point)) in enumerate(entry_points):
        shader_stage = None

        # Find the last shader stage tag.
        for line_num in range(entry_point_line_num - 1, -1, -1):
            if lines[line_num] == "//vert":
                shader_stage = "vert"
                break
            elif lines[line_num] == "//frag":
                shader_stage = "frag"
                break
            elif lines[line_num] == "//comp":
                shader_stage = "comp"
                break

        if not shader_stage:
            print(f"entry point missing for {entry_point.group(1)}")
            sys.exit(1)

        entry_point_lines = copy.copy(lines)

        # Comment out the other entry points
        for (j, entry_point_range) in enumerate(entry_point_ranges):
            if i == j:
                continue
            comment_out(entry_point_lines, entry_point_ranges[j])

        # Comment out layouts belonging to entry points above this one
        comment_out(entry_point_lines, (line_num for (line_num, line) in enumerate(lines[:prev_line_num]) if line.startswith("layout(")))

        # Comment out layouts belonging to entry points below this one
        comment_out(entry_point_lines, (entry_point_line_num + line_num for (line_num, line) in enumerate(lines[entry_point_line_num:]) if line.startswith("layout(")))

        # Rename the entry point
        entry_point_lines[entry_point_line_num] = entry_point_lines[entry_point_line_num].replace(entry_point.group(0), "void main()")

        filename = entry_point.group(1) + ".spv"

        file_contents = "\n".join(entry_point_lines)

        subprocess.run(f"glslc {args.flags} -I {parent_dir} -fshader-stage={shader_stage} - -o compiled_shaders/{filename}".split(" "), input=file_contents, text = True, check = True)

        prev_line_num = entry_point_line_num
