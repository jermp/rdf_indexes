import sys, os

input_filename = sys.argv[1]
prefix = sys.argv[2]

os.system("sort -n -k1,1 -k2,2 -k3,3 -u " + input_filename + " > " + prefix + ".mapped.sorted.spo")
os.system("sort -n -k2,2 -k3,3 -k1,1 -u " + input_filename + " > " + prefix + ".mapped.sorted.pos")
os.system("sort -n -k3,3 -k1,1 -k2,2 -u " + input_filename + " > " + prefix + ".mapped.sorted.osp")
os.system("sort -n -k3,3 -k2,2 -k1,1 -u " + input_filename + " > " + prefix + ".mapped.sorted.ops")
os.system("sort -n -k2,2 -k1,1 -k3,3 -u " + input_filename + " > " + prefix + ".mapped.sorted.pso")
