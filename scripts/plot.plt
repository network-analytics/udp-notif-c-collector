set term postscript eps enhanced "Helvetica" 20
set output "graph.eps"
plot "stats.txt" u 1:2 w p
