import sys, os

path_to_basename = sys.argv[1]
path_to_binaries = sys.argv[2]
prefix_name = sys.argv[3]

index_types = [
                  "compact_3t"
                , "ef_3t"
                , "vb_3t"
                , "pef_3t"
                , "pef_r_3t"
                , "pef_2to"
                ,"pef_2tp"
              ]

executables = [
                 "build"
               , "check_index"
               , "check_queries"
               ]

for type in index_types:
    output_filename = path_to_binaries + "/" + prefix_name + "." + type + ".bin"
    for program in executables:
        cmd = "./" + program + " " + type + " " + path_to_basename
        if program == "build":
            cmd += " -o"
        cmd += " " + output_filename
        os.system(cmd)
    os.system("rm " + output_filename)