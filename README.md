# STORK — SIMD Topological Order RanK

Experimental code to find multiple "optimal" paths wrt to up to 8 linear combinations.

## Usage

Run `./compile` and then
```bash
>>> ./build/run -h
MultiCostDAG

Available parameters:

  -h	--help
   
   This parameter is optional. The default value is ''.

  -i	--input_graph	(required)
   Input graph in DIMACs format.

  -s	--show_stats
   Show statistics.
   This parameter is optional. The default value is '0'.
```

Reading a file and running the "find all optimal path between vertex 0 and 10" (see `main.cpp`) code:

```bash
>>> ./build/run -s -i examples/transit_network.dimacs 
Read graph from file ... done [2ms]
Reorder graph ... done [0ms]
Graph Statistics:
	Vertices: 12
	Edges:    22
	MinDeg:   0
	MaxDeg:   5
	AvgDeg:   1.83333
	Isolated: 1
[coeff 0 -> v9] 0 -> 2 -> 5 -> 9  (cost=21)
[coeff 1 -> v9] 0 -> 2 -> 5 -> 9  (cost=21)
[coeff 2 -> v9] 0 -> 3 -> 7 -> 9  (cost=5)
[coeff 3 -> v9] 0 -> 3 -> 7 -> 9  (cost=0)
[coeff 4 -> v9] 0 -> 2 -> 5 -> 9  (cost=45)
[coeff 5 -> v9] 0 -> 2 -> 5 -> 9  (cost=49)
[coeff 6 -> v9] 0 -> 2 -> 5 -> 9  (cost=81)
[coeff 7 -> v9] 0 -> 2 -> 5 -> 9  (cost=98)
```

Note: The path currently prints the reordered vertex ids.

## Graph Format

The input graph should look like this:

```txt
Format:
c  comment line (ignored)
p  sp <n_vertices> <n_edges> <k_weights>   (problem line)
a  <from> <to> <w0> <w1> ... <wk-1>        (arc line, 1-indexed)

// Example (k = 2):
c  Small example graph
p  sp  4  5  2
a  1  2   5  10
a  1  3   3  20
a  2  3   2   5
a  2  4  10   1
a  3  4   4   8
```
