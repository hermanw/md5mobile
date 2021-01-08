./clspv kernels/compute.cl
./spirv-cross -V --output a.comp a.spv
./GlslangValidator -V a.comp -o compute.spv
