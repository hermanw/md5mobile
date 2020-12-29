echo "static const char *const compute_cl = R\"(" > mm.h
cat kernels/compute.cl >> mm.h
echo ")\";" >> mm.h