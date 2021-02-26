echo "static const char *const compute_cl = R\"(" > compute_cl.h
cat kernels/compute.cl >> compute_cl.h
echo ")\";" >> compute_cl.h