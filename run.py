import argparse
import math
import matplotlib
import matplotlib.pyplot as plt
import os
import random
import shutil
import struct
import csv
import subprocess
import time
from mpl_toolkits.mplot3d import Axes3D
from typing import Dict, List, Tuple
import re
def get_run_time():
    program_path = "./executable/psrs"
    mpi_run = "mpirun -np {procnum} "
    len_flag = " -l {length}"
    program = mpi_run + program_path + len_flag

    procnum_range = tuple(2**e for e in range(4))
    size_range = tuple(2**e for e in range(16, 20, 1))

    data_result = ""
    header = "{:<20},{:<20},{:<20},{:<20}\n".format(
    "array_size", "Time(s)", "Speedup", "procnum")
    data_result += header

    argument_dict = dict()
    for size in size_range:
        sequential_time = 0
        for procnum in procnum_range:   
            argument_dict["length"] = size
            argument_dict["procnum"] = procnum
            cmd = program.format(**argument_dict).split()
            with subprocess.Popen(cmd, stdout=subprocess.PIPE) as proc:
                output_bytes = proc.communicate()[0]
                output_str = output_bytes.decode("utf-8")
                match = re.search(r'Mean Sorting Time\n(\d+\.\d+)', output_str)
                if match:
                    run_time = float(match.group(1))
                else:
                    print("Error: Unable to extract sorting time.")
                    raise ValueError
                proc.wait()
            if procnum == 1:
                sequential_time = run_time
                formatted_str = "{:<20d},{:<20f},{:<20f},{:<20d}\n".format(size, run_time, 1, 1)
            else:
                formatted_str = "{:<20d},{:<20f},{:<20f},{:<20d}\n".format(size, run_time, sequential_time/run_time, procnum)
            data_result += formatted_str
    csv_file_name = './data/run_time.csv'
    with open(csv_file_name, 'w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        for line in data_result.splitlines():
            csv_writer.writerow(line.split(','))
        print(f"数据已成功写入到 {csv_file_name}")

def get_run_time_per_phase():
    program_path = "./executable/psrs"
    mpi_run = "mpirun -np 8 "
    len_flag = " -l {length} -o "
    program = mpi_run + program_path + len_flag

    size_range = tuple(2**e for e in range(14, 17, 1))

    data_result = ""
    header = "{:<20},{:<20},{:<20},{:<20},{:<20}\n".format(
    "array_size", "phase1", "phase2","phase3","phase4")
    data_result += header

    argument_dict = dict()
    for size in size_range:
        phase = []
        argument_dict["length"] = size
        cmd = program.format(**argument_dict).split()
        with subprocess.Popen(cmd, stdout=subprocess.PIPE) as proc:
            output_bytes = proc.communicate()[0]
            output_str = output_bytes.decode("utf-8")
            match = re.search(r'Phase 1, Phase 2, Phase 3, Phase 4\n(\d*\.\d+),\s*(\d*\.\d+),\s*(\d*\.\d+),\s*(\d*\.\d+)', output_str)

            if match:
                for i in range(4):
                    phase.append(float(match.group(i+1))) 
            else:
                print("Error: Unable to extract sorting time.")

            formatted_str = "{:<20d},{:<20f},{:<20f},{:<20f},{:<20f}\n".format(size, phase[0], phase[1], phase[2], phase[3])
        proc.wait()
        data_result += formatted_str
    csv_file_name = './data/run_time_per_phase.csv'
    with open(csv_file_name, 'w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        for line in data_result.splitlines():
            csv_writer.writerow(line.split(','))
        print(f"数据已成功写入到 {csv_file_name}")




def _log2_exponent_get(number: float) -> str:
    """
    Returns a specially formatted string of the result log2(number).

    NOTE: The result log2(number) must be an integer.
    """
    result = math.log2(number)

    if not result.is_integer():
        raise ValueError("The result exponent must be an integer")

    result = int(result)
    return r"$\mathregular{2^{" + str(result) + r"}}$"

def test():
    program = "mpirun -np 2 ./executable/psrs -l 20 "
    cmd = program.split()
    print(cmd)
    process = subprocess.Popen(cmd,stdout=subprocess.PIPE)
    print("process started")
    # 获取标准输出和标准错误的输出
    stdout = process.communicate()[0]

    # 打印输出结果
    print("Standard Output:")
    print(stdout.decode('utf-8'))


    # 等待子进程结束
    process.wait()

    # 获取命令的返回码
    return_code = process.returncode
    print("\nReturn Code:", return_code)

def main():
    get_run_time()
    get_run_time_per_phase()



if __name__ == "__main__":
    main()
