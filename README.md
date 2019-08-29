# md5mobile

md5mobile builds a formula in memory to decode md5 hash value back to the original mobile number. It supports Chinese mobile number in the format of 13812345678.

## Usage:
  md5mobile [Flags] [Filename]

### Flags:
  -d    decode the hashes in given file.
  -t    generate random mobiles and test
  -b    build formula and write to file "formula.dat"
  -f    load from "formula.dat" at first. usage: -df, -tf
### Filename:
  csv file which contains hashes to be decoded.
