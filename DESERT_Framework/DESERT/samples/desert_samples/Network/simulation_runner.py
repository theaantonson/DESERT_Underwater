import subprocess

node_counts = [3, 5]
# "line", "circle", "grid", "random"
topologies = ["line", "circle", "grid"]
node_distances = [2000.0]

for nn in node_counts:
    for topo in topologies:
        for distance in node_distances:
            print(f"Testing {nn} nodes in a {topo} formation with node distance {distance} m ...")
            
            cmd = f"ns test_uwguwmanet_uwguwal.tcl 125 60 1 {nn} {topo} {distance}"
            
            output_file = f"results/guwmanet_{nn}nodes_{topo}distance{distance}.txt"
            with open(output_file, "w") as f:
                subprocess.run(cmd, shell=True, stdout=f, stderr=subprocess.STDOUT)