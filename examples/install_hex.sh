#
#  usage:  ./install_hex.sh  ./bin/<target-hex-file>
#
nrfjprog -f nRF52 -e
nrfjprog -f nRF52 --program $1
nrfjprog -f nRF52 -r
